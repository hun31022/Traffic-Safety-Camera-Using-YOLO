CC = gcc
CFLAGS = -g -Wall

LIBS = -lpthread
SQL = -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient
CV1 = `pkg-config --cflags opencv`
CV2 = `pkg-config --libs opencv`

all: server dbclient

server: 
		$(CC) $(CFLAGS) $(CV1) -o server th_main_server.c $(CV2) $(LIBS) $(SQL)

dbclient:
		$(CC) $(CFLAGS) -o dbclient db_client.c

		
clean:
		-rm -f server dbclient
