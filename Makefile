all: file_server file_client

file_client:
	gcc -Wall  -o file_client file_client.c

file_server:
	gcc -Wall  -o file_server file_server.c

clean:
	-rm -rf *.o all file_client file_server