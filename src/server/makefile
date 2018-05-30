project_server : protocol.o project_server.o sqlite3.c
	gcc -o project_server protocol.o project_server.o sqlite3.c -lpthread -ldl

test_client : protocol.o
	gcc -o test_client test_client.c protocol.o

protocol.o : protocol.c
	gcc -c protocol.c

project_server.o : project_server.c
	gcc -c project_server.c

clean :
	rm *.o
