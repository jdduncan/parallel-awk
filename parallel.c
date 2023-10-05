/*
Parallel Awk

Copyright (C) John David Duncan, 2004
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation. JOHN DAVID DUNCAN DISCLAIMS ALL WARRANTIES 
WITH REGARD TO THIS SOFTWARE.
*/

#define DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "awk.h"
#include "ytab.h"
#include "mpi.h"
#include "messages.h"

int		mpi = 0;		/* Running in Parallel */
int		mpi_rank;       /* Our node number */
int		mpi_size;       /* Total number of nodes */
int		skiplines = 0;
char	skip_set = 0; 

#define NREDUCEOPS 6
Array   *reduceops;		/* symbol table */
MPI_Op  optable[NREDUCEOPS] = 
  { MPI_MIN, MPI_MAX, MPI_SUM, MPI_PROD, MPI_LAND, MPI_LOR };

MPI_Aint SIZE_OF_INT;
MPI_Aint SIZE_OF_FVAL;
MPI_Aint SIZE_OF_CHAR;

/* Should comm.list be a dynamic array? */
#define COMM_ARRAY_SIZE 128
struct communicators {
	int id;   /* index of the current active communicator */
	int max;  /* max used communicator */
	int size; /* always COMM_ARRAY_SIZE, for now */
	MPI_Comm list[COMM_ARRAY_SIZE];  /* The list of usable communicators */
} comm;

MPI_Datatype MPIType_Msg_Header;

MPI_Op Op_str_least;
MPI_Op Op_str_greatest;

/* Borrowed from run.c */
static Cell	truecell	={ OBOOL, BTRUE,  0, 0, 1.0, NUM };
static Cell falsecell   ={ OBOOL, BFALSE, 0, 0, 0.0, NUM };
#define tempfree(x)	if (istemp(x)) tfree(x); else


/*       prototypes, private to this file: */
   /* MPI functions: */
Cell *  assign_parallel(Node *a);
Cell *  barrier();
Cell *  comm_split(Node *a);
Cell *  comm_set(Node *a);
Cell *	mpi_hash(Node *a);
   /* Functions for user-defined MPI operations: */
void strcmp_least(char *in, char *inout, int *len, MPI_Datatype *t);
void strcmp_greatest(char *in, char *inout, int *len, MPI_Datatype *t);
   /* Misc. functions: */
Cell *  parallel_skip_lines(int batch_size);
void	Set_RANK_SIZE();
int		node_rank(Node *a);
Cell *  unpack(struct _send_recv *sr);
void	dbug_header(int header[MSG_HEADER_N_ELEMENTS]);
void	too_many(Node *a, char *f);
void	send_cell(int cmd, Cell *x, struct _send_recv *sr);
int     big_packet(struct _send_recv *sr);


/*** startup and shutdown functions ***/


void init_parallel(int *argc, char ***argv) {
    if (MPI_Init(argc,argv) == MPI_SUCCESS)
        mpi=1;
}

void begin_parallel() {
    MPI_Aint sz;
    
	if(mpi == 0) return;

 /* Initialize the list of communicators */
	comm.id = comm.max = 0;
	comm.size = COMM_ARRAY_SIZE;
	comm.list[0] = MPI_COMM_WORLD;

 /* Set PARALLEL, RANK, and SIZE: */
	setfval(PARloc, 1.0);
	Set_RANK_SIZE();

 /* Create datatypes: */
	MPI_Type_contiguous(MSG_HEADER_N_ELEMENTS,MPI_INT,&MPIType_Msg_Header);
	MPI_Type_commit(&MPIType_Msg_Header);

 /* Create operations: */
    MPI_Op_create((MPI_User_function *) strcmp_least,   1, &Op_str_least);
    MPI_Op_create((MPI_User_function *) strcmp_greatest,1, &Op_str_greatest);	
    
 /* Calculate datatype strides in packed buffers: */
    MPI_Type_extent(MPI_CHAR,&SIZE_OF_CHAR);
    MPI_Type_extent(MPI_INT,&SIZE_OF_INT);  
    MPI_Type_extent(MPI_DOUBLE,&SIZE_OF_FVAL);

 /* Build a symbol table for collective operations */
    reduceops = makesymtab(SMALLSYMTAB);
	setsymtab("min", "", 0.0, STR, reduceops);
	setsymtab("max", "", 1.0, STR, reduceops);
	setsymtab("sum", "", 2.0, STR, reduceops);
	setsymtab("prod","", 3.0, STR, reduceops);
	setsymtab("and", "", 4.0, STR, reduceops);
	setsymtab("or",  "", 5.0, STR, reduceops);
}

void end_parallel() {
    MPI_Finalize();       
}    


/* Generic MPI function call */
/* a[0] is subtype, a[1] is list of linked nodes */
Cell *parallel(Node **a, int n) {
	int f;
 
	f = ptoi(a[0]);
	switch (f) {
		case ASSIGN_PRLL:
			return assign_parallel(a[1]);
		case HASH:
			return mpi_hash(a[1]);
		case BARRIER:
			return barrier();
		case COMM_SPLIT:
			return comm_split(a[1]);
		case COMM_SET:
			return comm_set(a[1]);
		default:	/* can't happen */
			FATAL("illegal MPI function call type %d", f);
	}
	return &truecell;
}


/**** Code to implement the parallel functions.
	  If you execute a node, 
      tempfree() the returned cell
      when you're done with it 
*****/

/* assign(NR,10) */
/* TO DO: support FNR as well */
Cell *assign_parallel(Node *a) {
    Cell *x, *y;
    int i;

    if(mpi == 0) return &truecell;
    x=execute(a);
	if(x != nrloc) 
		FATAL("First paramater of assign(NR,x) must be NR");
	if((a=a->nnext) == NULL) FATAL("not enough args in assign()");
	y=execute(a);
	i = (int) getfval(y);
	tempfree(x);
	tempfree(y);
	if(a->nnext != NULL) too_many(a->nnext,"assign");
	return parallel_skip_lines(i);
}


/* hash("array key") 
   orthogonal to the usual awk hash, as it uses
   multiplier 37 rather than 31.  It may be important
   to this algorithm that the number of buckets, 
   mpi_size, is usually even and is often a 
   power of 2
*/
Cell *mpi_hash(Node *a) {
    Cell *x, *y;
    int i;
	unsigned hashval;
	char *s;
	
    x=execute(a);
	if (a->nnext != NULL) too_many(a->nnext,"hash");
	s=getsval(x);

	for (hashval = 0; *s != '\0'; s++)
		hashval = (*s + 37 * hashval);
    i = hashval % mpi_size;

    tempfree(x); 

    y=gettemp();
    y->tval = NUM;
    setfval(y,(Awkfloat) i);
	return y;
}


Cell *barrier() {
    if(MPI_Barrier(comm.list[comm.id]) == MPI_SUCCESS)
        return ( &truecell );
    
    return( &falsecell );
}


/* x = comm_split(color,key,intercom_color) */
Cell *comm_split(Node *a) {
	Cell *x, *y;
	int color, key;
	char *s;
	
	if (!mpi) return &falsecell;
	if (comm.max++ == comm.size) 
		FATAL("Out of communicators\n");
	
	x = execute(a);
	a = a->nnext;
	y = execute(a);
	s=getsval(x);
	if(*s == NULL) color = MPI_UNDEFINED;
	else color = (int) getfval(x);
	key = (int) getfval(y);
	tempfree(x);
	tempfree(y);
	
	x = gettemp();
	MPI_Comm_split(comm.list[comm.id],color,key,&comm.list[comm.max]);
	if ((a = a->nnext) == NULL) {
		comm.id = comm.max;
		setfval(x, (Awkfloat) comm.id);
		return x;
	}	
	/* intercom_color is set, so get intercommunicators: */
	
}


/* comm_set(x) */
Cell *comm_set(Node *a) {
	Cell *x;
	int c;

	if (!mpi) return &truecell;
	x = execute(a);
	c = (int) getfval(x);
	if(c < 0 || c > comm.max) 
		FATAL("comm_set(%d): %d is not a valid communicator\n",c,c);
	comm.id = c;
	Set_RANK_SIZE();
	if (a->nnext != NULL) too_many(a->nnext,"comm_set");
	setfval(x,c);
	return x;
}


/*** public functions, seen in awkgram.y:  ***/


/* reduce(func(term)) 
   a[0] is the name of an operator;
   a[1] is the value to reduce 
*/
Cell *reduce(Node **a, int n) {    
    Cell *x, *y; 
	char *s;
	Awkfloat f, f_reduced;
    int i, i_reduced;
	int dsize = 1;
	void *v_in, *v_out;
    MPI_Op op;
    MPI_Datatype dtype = MPI_DOUBLE;

	if(!mpi) return x;

	s = (execute(a[0]))->nval;				/* get the name of the operator; */
	x = lookup(s,reduceops);				/* look it up in the table of collective operations, */
	if(x) op = optable[(int) x->fval];		/* which provides an index into an array. */
	/* else ... { user-defined operation } */
	else FATAL("Unknown collective operation %s \n",s);
	
    x = execute(a[1]);
	if(isarr(x)) funnyvar(x,"reduce()");
	s = getsval(x);

 /* If the op is MIN or MAX, everyone must agree as to whether we're reducing 
    a string or a number (otherwise, there will be deadlock).  If -anyone-
	has a value that's absolutely not a number, we'll have to reduce as strings.
	So take a poll, and piggyback on it to get the needed string length.
 */
	if(op == MPI_MIN || op == MPI_MAX) {
		int poll_in[2], poll_out[2]; 
		poll_in[0] = isnum(x) ? 1 : 2;
		poll_in[1] = strlen(s) + 1;
		MPI_Allreduce(poll_in,poll_out,2,MPI_INT,MPI_MAX,comm.list[comm.id]);
		if(poll_out[0] == 2) {  
			dprintf( ("reducing with string sort\n") );
			dtype = MPI_CHAR;	
			dsize = poll_out[1];
			if(op == MPI_MIN) op = Op_str_least;
			else op = Op_str_greatest;
			v_in = malloc(dsize);
			v_out = malloc(dsize);
			strncpy(v_in,s,dsize - 1);
		}
	}
	if(dtype != MPI_CHAR) { /* not a string sort */
		if(isnum(x)) {
			f = getfval(x);
			i = (int) f;
			dprintf( ("reducing as number\n") );
		}
		else {
			i = strlen(s);
			f = (Awkfloat) i;
			dprintf( ("reducing as string length\n") );
		}
		if(op == MPI_LOR || op == MPI_LAND) {
			dtype = MPI_INT;
			v_in = &i;
			v_out = &i_reduced;
		}
		else {
			v_in = &f;
			v_out = &f_reduced;
		}
	}
    
    MPI_Allreduce(v_in,v_out,dsize,dtype,op,comm.list[comm.id]);

    tempfree(x);
	y=gettemp();
	if(dtype == MPI_CHAR) {
        y->tval = STR;
        y->sval = v_out;
        free(v_in);
	}
	else { 
		y->tval = NUM;
		if(dtype == MPI_DOUBLE) setfval(y,f_reduced);
		else setfval(y,(Awkfloat) i_reduced);
	}
    return y;
}


/* send mesg > n */
/* recv resp < m */
/* sendrecv (mesg,n,resp) [implicit m=n] */
/* sendrecv (mesg,n,resp,m) */

Cell *SendRecv(Node **a, int n) {    
    Cell *x, *y, *resp;
	Array *symtab;
	register struct { /* bitfield */
		unsigned int sending : 1 ;
		unsigned int recving : 1 ;
	} mode = { 0 , 0 };
	struct _send_recv sr;
    MPI_Status status;
	int i;

	if(!mpi) return &falsecell;  /* is that right? */
	if(a[0] != NULL) mode.sending=1;
	if(a[2] != NULL) mode.recving=1;
	sr.commu = comm.id;
	sr.tag = AWK_MSG_TAG;
	
	if(mode.sending) {
		x = execute(a[0]);
		sr.dst = node_rank(a[1]);
		if(mode.recving) send_cell(CMD_init_sendrecv,x,&sr);
		else send_cell(CMD_init_send,x,&sr);
		if(isarr(x)) {
			symtab = (Array *) x->sval;
			for (i = 0; i < symtab->size; i++)
                for (y = symtab->tab[i]; y != NULL; y = y->cnext)
					send_cell(CMD_pack,y,&sr);
		}
		else send_cell(CMD_pack,x,&sr);
		tempfree(x);
	}
	if(mode.recving) {
        sr.recv_buff_size = MSG_MAX_BUFF;
		if((sr.recv_packet = malloc(sr.recv_buff_size)) == NULL) 
			FATAL("out of memory in recv");
		sr.src = node_rank(a[2]);
	}
	
	dprintf(("ready to send/recv ... \n"));
	if (mode.recving && !mode.sending) 
		MPI_Recv(sr.recv_packet, sr.recv_buff_size, MPI_PACKED, 
				 sr.src, sr.tag, comm.list[sr.commu], &status);
	else send_cell(CMD_finalize,NULL,&sr);
	dprintf(("send/recv completed.\n"));

	if (mode.recving) {
        /* WHAT IF the recv argument is an array element, 
           or $0, or $1, etc, but the thing received is an array? */
		y = unpack(&sr);
        resp = execute(a[3]);
        /* if isarr(resp) ... then copy y into it */
        resp->sval = y->sval;
        resp->tval = y->tval;
        resp->fval = y->fval;
        dprintf(("received node: type %d, sval %s, fval %f\n",y->tval,y->sval,y->fval));
		free(sr.recv_packet);
	}

    return &truecell;
}


/*** User-defined functions for MPI operations ***/

void strcmp_least(char *in, char *inout, int *len, MPI_Datatype *t) {
	dprintf((" .. sorting \"%s\" < \"%s\" ?\n",in,inout));
	if(strcmp(in,inout) < 0)
		strncpy(inout,in,*len);
}


void strcmp_greatest(char *in, char *inout, int *len, MPI_Datatype *t) {
	dprintf((" .. sorting \"%s\" > \"%s\" ?\n",in,inout));
	if(strcmp(in,inout) > 0)
		strncpy(inout,in,*len);
}


/*** private utility functions:  ***/


Cell *parallel_skip_lines(int batch_size) {
    int nr, round_size, nrounds, next;
    int nskip;

    nr = (int) getfval(nrloc);
    if((nr/batch_size) % mpi_size == mpi_rank) 
        return(&truecell);
    else {
        /* set the global "skiplines" optimization */
        round_size = mpi_size * batch_size;
        nrounds = nr / round_size;
        next = (nrounds * round_size) + (mpi_rank * batch_size);
        if(next < nr) next += round_size;
        nskip = next - nr - 1;
        /* If multiple statements all want to set skiplines, 
           then the smallest skip wins:
        */
        if(skip_set == 0 || skiplines > nskip) {
            skiplines=nskip;
			skip_set = 1;
		}
        return(&falsecell);
    }
}


void Set_RANK_SIZE() { 
	MPI_Comm_rank(comm.list[comm.id], &mpi_rank);
	MPI_Comm_size(comm.list[comm.id], &mpi_size);
	setfval(RANKloc, (Awkfloat) mpi_rank);
	setfval(SIZEloc, (Awkfloat) mpi_size);
}


int node_rank(Node *a) {
	int i;
	Cell *x;
    char *v;
	
	x = execute(a);
	i = (int) getfval(x);
	if(isstr(x)) {
		v = getsval(x);
		if(!strncmp(v,"*",1)) return MPI_ANY_SOURCE;
		if(*v == NULL) return MPI_PROC_NULL;
	}
	if((!isnum(x)) || i < 0  || i > (mpi_size - 1))
		FATAL("Invalid process rank %d\n",i);
	tempfree(x);
	return i;
}


/* To do: the static variables
   could be moved into struct _send_recv */

void send_cell(int cmd, Cell *x, struct _send_recv *sr) {
	static int state=0;
	static int position;
	static int mode;

	int header[MSG_HEADER_N_ELEMENTS] = {0 , 0};
	register char *r;
	char *s = NULL;
    char *s1;
    char *warning_packet = NULL;
	Awkfloat v;
	int tval;
    int warpos=0, recvs=0;
    MPI_Status status;

	if(cmd < CMD_init) {
		assert(state == 0);
		mode = cmd;
		state = CMD_init;
        sr->send_size = 2 * SIZE_OF_INT;  /* header */
        sr->send_buff_size = MSG_MAX_BUFF;
		if((sr->send_packet = malloc(sr->send_buff_size)) == NULL) 
			FATAL("out of memory in send_cell");
		position = 0;
		if(isarr(x)) {
			state = CMD_is_array;
			header[MSG_type] = MSGT_MULTI;
			header[MSG_length] = ((Array *) x->sval)->nelem;
			MPI_Pack(header, 1, MPIType_Msg_Header,
			   sr->send_packet, sr->send_buff_size, &position, comm.list[sr->commu]);
            dprintf(("Node %d packed header for array of length %d \n",mpi_rank,header[MSG_length]));
		}
	}
	if(cmd == CMD_pack) {
		assert(state > 0);
		assert(!isarr(x));
		header[MSG_type] = MSGT_SINGLE;
		v = getfval(x);
		s = getsval(x);
		if(s) header[MSG_length] = strlen(s) + 1;
		tval = x->tval & ~(FLD|REC);
		if (state == CMD_is_array) {
			header[MSG_length] += strlen(x->nval) + 1;			
			s = malloc(header[MSG_length]);
            for(s1=s, r=x->sval; (*s1++ = *r++); );
            for(r=x->nval; (*s1++ = *r++); );			
		}
        /* 3 ints = two for header plus one for tval: */
        sr->send_size += (3 * SIZE_OF_INT) + (header[MSG_length] * SIZE_OF_CHAR);
        if(tval & NUM) sr->send_size += SIZE_OF_FVAL;

        adjbuf(&(sr->send_packet), &(sr->send_buff_size), sr->send_size,
               (sr->send_buff_size * 2), 0, "expanding send buffer");
        
		MPI_Pack(header, 1, MPIType_Msg_Header,
			   sr->send_packet, sr->send_buff_size, &position, comm.list[sr->commu]);
		MPI_Pack(&tval, 1, MPI_INT,
			   sr->send_packet, sr->send_buff_size, &position, comm.list[sr->commu]);
		if(tval & NUM) MPI_Pack(&v, 1, MPI_DOUBLE,
			   sr->send_packet, sr->send_buff_size, &position, comm.list[sr->commu]);
		if(s) MPI_Pack(s, header[MSG_length], MPI_CHAR, 
			   sr->send_packet, sr->send_buff_size, &position, comm.list[sr->commu]);
		if (state == CMD_is_array) free(s);
	}
	if(cmd == CMD_finalize) {
		assert(state > 0);
		state = 0;
		goto SEND_A_PACKET;
	}
	return;
	
	SEND_A_PACKET:

    if(sr->send_size > MSG_MAX_BUFF) {
        dprintf(("sending a warning packet for message length %d (%d)\n", sr->send_size,position));
        /* send a warning packet */
        header[MSG_type] = MSGT_TOO_LONG;
        header[MSG_length] = sr->send_size; 
        if((warning_packet = malloc(128)) == NULL)
            FATAL("out of memory in send_cell");
        MPI_Pack(header, 1, MPIType_Msg_Header,
            warning_packet, 128, &warpos, comm.list[sr->commu]);
    }
    
    if(mode == CMD_init_sendrecv) recvs = 1;
    
    if(warning_packet) {   /* send the warning packet */
        if(recvs) {
            MPI_Sendrecv(warning_packet, warpos, MPI_PACKED, sr->dst, sr->tag,
                sr->recv_packet, sr->recv_buff_size, MPI_PACKED, sr->src, sr->tag,
                comm.list[sr->commu], &status);
            if(!big_packet(sr)) recvs--;
        }
        else {
            MPI_Send(warning_packet, warpos, MPI_PACKED, 
                     sr->dst, sr->tag, comm.list[sr->commu]);
        }
        free(warning_packet);
    }

    if(recvs) {  /* send the real packet with MPI_Sendrecv() */
        MPI_Sendrecv(     
            sr->send_packet, position, MPI_PACKED, sr->dst, sr->tag,
            sr->recv_packet, sr->recv_buff_size, MPI_PACKED, sr->src, sr->tag, 
            comm.list[sr->commu], &status);
        if(!big_packet(sr)) recvs--;
    }
    else {      /* send the real packet with MPI_Send() */
        MPI_Send(sr->send_packet, position, MPI_PACKED, 
                 sr->dst, sr->tag, comm.list[sr->commu]);
    }

    if(recvs) {
        /* you are here if you sent a short message but got a long one. */
        assert(warning_packet == NULL);
        MPI_Recv(sr->recv_packet, sr->recv_buff_size, MPI_PACKED, 
            sr->src, sr->tag, comm.list[sr->commu], &status);
        recvs--;
    }
    assert(recvs == 0);

    free(sr->send_packet);    
}


/* If a message header is MSGT_TOO_LONG, big_packet()
   will resize the receive buffer and return 1;
   otherwise it returns 0 
*/
int big_packet(struct _send_recv *sr) {
    int header[MSG_HEADER_N_ELEMENTS];
    int position = 0;
    
	MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position,
				header, 1, MPIType_Msg_Header, comm.list[sr->commu]);

    if(header[MSG_type] == MSGT_TOO_LONG) {
        dprintf(("Got a warning packet; preparing for long (%d) message \n",header[MSG_length]));
        if(header[MSG_length] > sr->recv_buff_size) {
            /* Discard the warning packet and fetch the actual message */
            sr->recv_buff_size = header[MSG_length];            
            free(sr->recv_packet);
            sr->recv_packet = malloc(sr->recv_buff_size);
            if(! sr->recv_buff_size && sr->recv_packet) 
                FATAL("out of memory resizing recv buffer");
        }
        return 1;
    }
    return 0;
}



Cell *unpack(struct _send_recv *sr) {
	int header[MSG_HEADER_N_ELEMENTS], cell_header[MSG_HEADER_N_ELEMENTS];
	int tval, i, position = 0;
	Cell *x;
	char *s = NULL, *s1 = NULL;
	char *sval = NULL, *name = NULL;
	Array *a = NULL;
	Awkfloat fv = 0.0;
    MPI_Status status;

	x=gettemp();

	MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position,     /* header */
				header, 1, MPIType_Msg_Header, comm.list[sr->commu]);

    if(big_packet(sr)) {
        position = 0;
        MPI_Recv(sr->recv_packet, sr->recv_buff_size, MPI_PACKED, 
            sr->src, sr->tag, comm.list[sr->commu], &status);
        MPI_Unpack(sr->recv_packet, sr->recv_buff_size, &position,
			header, 1, MPIType_Msg_Header, comm.list[sr->commu]);
    }

    assert(header[MSG_type] != MSGT_TOO_LONG);
    
	if(header[MSG_type] == MSGT_MULTI) {
        dprintf(("Node %d unpacking array of size %d \n",mpi_rank,header[MSG_length]));
		x->sval = (char *) (a = makesymtab(BIGSYMTAB));
        x->tval = ARR;
		for(i = 0 ; i < header[MSG_length] ; i++) {
			MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position,     /* header */
				cell_header, 1, MPIType_Msg_Header, comm.list[sr->commu]);

			MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position,     /* tval */
						&tval, 1, MPI_INT, comm.list[sr->commu]);
            dprintf(("unpacked type: %d\n",tval));
			if(tval & NUM) {							             /* float */
				MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position, 
							&fv, 1, MPI_DOUBLE, comm.list[sr->commu]);
			}
            dprintf((" .. %d string characters in this cell.\n",cell_header[MSG_length]));
			s = malloc(cell_header[MSG_length]);
			MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position,    /* strings */
						s, cell_header[MSG_length], MPI_CHAR, comm.list[sr->commu]);
			if(tval & STR) {
				sval = s1 = s;
				while((*s1++));  /* skip to the first null */
			}
			name = s1;
            dprintf((" .. name: %s  sval: %s \n",name,sval));

			setsymtab(name,sval,fv,tval,a);

			free(s);  /* setsymtab has made copies of name and sval */
		}
	}
	else {
		MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position,     /* tval */
					&tval, 1, MPI_INT, comm.list[sr->commu]);

		if(tval & NUM) {							            /* float */
			MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position, 
						&fv, 1, MPI_DOUBLE, comm.list[sr->commu]);
			setfval(x,fv);
		}
		if(tval & STR) {
			s = malloc(header[MSG_length]);
			MPI_Unpack( sr->recv_packet, sr->recv_buff_size, &position, /* string */
						s, header[MSG_length], MPI_CHAR, comm.list[sr->commu]);
			x->sval = s;
		}
	}
    return x;
}


void dbug_header(int header[MSG_HEADER_N_ELEMENTS]) {
	dprintf(("Header type: %d, length: %d \n",
			  header[MSG_type],header[MSG_length]));
}




void too_many(Node *a, char *f) {
	WARNING("warning: too many arguments to %s()", f);
	for ( ; a ; a = a->nnext)
		execute(a);
}


