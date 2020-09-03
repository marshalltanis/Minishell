#    $Id: Makefile,v 1.10 2017/06/01 16:08:55 tanism3 Exp $



FILES=arg_parse.c builtin.c msh.c expand.c strmode.c findComment.c findChar.c sigHandler.c findRedir.c handlePipes.c stmts.c
OBFILES=${FILES:.c=.o}
CC=gcc
CFLAGS=-g -Wall

msh:$(OBFILES)

#dependencies list
${OBFILES} : proto.h
clean:
	rm -f $(OBFILES) msh
