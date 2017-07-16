# makefile
# This makes "dblist"

dblist: dblist.o
	$(CC) dblist.o -o dblist -lisam $(INFORMIXDIR)/lib/libsql.a -s

dblist.o: dblist.c
	$(CC) -O -I$(INFORMIXDIR)/incl -c dblist.c
