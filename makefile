# /****************************************************************
# Copyright (C) Lucent Technologies 1997
# All Rights Reserved
# 
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose and without fee is hereby
# granted, provided that the above copyright notice appear in all
# copies and that both that the copyright notice and this
# permission notice and warranty disclaimer appear in supporting
# documentation, and that the name Lucent Technologies or any of
# its entities not be used in advertising or publicity pertaining
# to distribution of the software without specific, written prior
# permission.
# 
# LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
# IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
# SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
# IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
# ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
# THIS SOFTWARE.
# ****************************************************************/

CFLAGS = -g
CFLAGS = -O2
# CFLAGS = -O

CC = mpicc

YACC = bison -y
YACC = yacc
YFLAGS = -d

OFILES = b.o main.o parse.o proctab.o tran.o lib.o run.o lex.o

SOURCE = awk.h ytab.c ytab.h proto.h awkgram.y lex.c b.c main.c \
	maketab.c parse.c lib.c run.c tran.c proctab.c missing95.c \
	parallel.c messages.h

LISTING = awk.h proto.h awkgram.y lex.c b.c main.c maketab.c parse.c \
	lib.c run.c tran.c missing95.c parallel.c messages.h 

SHIP = README FIXES $(SOURCE) ytab[ch].bak makefile makefile.win \
	vcvars32.bat buildwin.bat awk.1

mpawk:	ytab.o parallel.o $(OFILES)
	$(CC) $(CFLAGS) -o mpawk ytab.o parallel.o $(OFILES) $(ALLOC)  -lm

parallel.o: parallel.c messages.h

$(OFILES):	awk.h ytab.h proto.h

ytab.o:	awk.h proto.h awkgram.y
	$(YACC) $(YFLAGS) awkgram.y
	mv y.tab.c ytab.c
	mv y.tab.h ytab.h
	$(CC) $(CFLAGS) -c ytab.c

proctab.c:	maketab
	./maketab >proctab.c

maketab:	ytab.h maketab.c
	$(CC) $(CFLAGS) maketab.c -o maketab

bundle:
	@cp ytab.h ytabh.bak
	@cp ytab.c ytabc.bak
	@bundle $(SHIP)

tar:
	@cp ytab.h ytabh.bak
	@cp ytab.c ytabc.bak
	@bundle $(SHIP) >awk.shar
	@tar cf awk.tar $(SHIP)
	gzip awk.tar
	ls -l awk.tar.gz
	@zip awk.zip $(SHIP)
	ls -l awk.zip

names:
	@echo $(LISTING)

clean:
	rm -f a.out *.o *.obj maketab maketab.exe # proctab.c
