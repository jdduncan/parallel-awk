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


/********       Message Format
	       
         +-----------+------------+    
 header: |   type    |   length   |     MPI_INT[2]
         +-----------+------------+

         +-----------+------------+
  cell:  |          tval          |     MPI_INT
         +-----------+------------+
         |          fval          |     MPI_DOUBLE
         +------------------------+
         |          sval          |     MPI_CHAR
         |          name          |    
         +------------------------+
	   
*********/

#define MSG_MAX_BUFF        8192
#define AWK_MSG_TAG         77

struct _send_recv {
    int commu;		/* index in table of communicators */
    int src;		/* rank of src node (for recv) */
    int dst;		/* rank of dest node (for send) */
    int tag;                /* MPI tag */
    int send_size;          /* actual size of message */
    int send_buff_size;     /* size of buffers ... */
    int recv_buff_size;
    char *send_packet;      /* buffers ... */
    char *recv_packet;      
    char *recv_prefix;      /* for unpacking array slices ... */
    char *recv_suffix;
};


enum Awk_Message_Header {
	MSG_type = 0,
	MSG_length,
	MSG_HEADER_N_ELEMENTS   
};

enum Message_Types {
	MSGT_SINGLE = 1,     /* MSG_length = size of strings in this cell */
	MSGT_MULTI,          /* MSG_length = number of cells in this packet */
	MSGT_TOO_LONG        /* MSG_length = size of buffer to allocate for real message */
};

enum send_cell_commands {
	CMD_init_send = 1,
	CMD_init_sendrecv,
	CMD_init,
	CMD_is_array,
	CMD_pack,
	CMD_finalize
};
