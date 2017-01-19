#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <unistd.h>
#include<signal.h>
#include <setjmp.h>
#define MAX_STR_LENGTH 2000
#define BUFFLEN 1024
#define IP 2130706433 /* 127.0.0.1 */
void  ALARMhandler(int sig);
//Signal handler that ends the process after 5 seconds has elapsed
void  ALARMhandler(int sig){
	fprintf(stderr,"Transmission appears to have been aborted.\n");
	exit(1);
}
int main(int argc, char *argv[]){
	//declaring variables and handling errors
	struct sockaddr_in si_me, si_other;
	if (argc != 4){
		fprintf(stderr,"You have not enetered the required parameters\n");
		exit(1);
	}
	int s, i, slen = sizeof(si_other);
	char server_ip[MAX_STR_LENGTH];
	signal(SIGALRM, ALARMhandler);
	strncpy(server_ip, argv[1],strlen(argv[1]));
	int port_no = atoi(argv[2]);
	char filename[MAX_STR_LENGTH];
	char buff[BUFFLEN];
	strncpy(filename, argv[3],strlen(argv[3]));
	//Setting up socket and more error handling
	if ( ( s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) == -1 ){
		printf("Error in creating socket\n");
		return 1;
	}
	
	memset((char *) &si_me, 0, sizeof(si_me));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port_no);		//sends stuff
	si_other.sin_addr.s_addr = inet_addr(argv[1]);    /* htonl(INADDR_ANY) for any interface on this machine */

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port_no + 1);	//listening to this port number
	si_me.sin_addr.s_addr = htonl(IP); 

	//printf("\n\nClient listening to %s:%d\n\n", inet_ntoa(si_me.sin_addr), ntohs(si_me.sin_port));
	//printf("\nSending %s to %s:%d\n", argv[3] , inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
	//Binding the socket
	if ( bind(s, (const struct sockaddr *) &si_me, sizeof(si_me)) == -1 ){
		printf("Error in binding the socket\n");
		return 2;
	}

	//printf("\nSending %s to %s:%d\n", argv[3] , inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
	//sending request to server
	sendto(s, filename, strlen(filename) + 1, 0, (const struct sockaddr *)&si_other, sizeof(si_other));
	//Waiting on response form the server and printing the returned file chunks
	while(1){
		
		if ( recvfrom(s, buff, BUFFLEN + 1, 0, (struct sockaddr *) &si_other, (socklen_t *)&slen) != -1){
			alarm(5);
			//printf("\nReceived chunk from %s:%d : %s\n\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buff);
			//sleep(5);
			if(strncmp(buff,"$",strlen("$")) != 0){
				printf("%s",buff);
			}
			//printf("\n######################This should only ever be 1024 size: %d\n",(int)strlen(buff));
			if((strncmp(buff,"$",strlen("$")) == 0) || (strlen(buff) < BUFFLEN)){
				if(strncmp(buff,"$",strlen("$")) != 0){
					printf("\n");
				}
				//printf("\n##########wtf############This should only ever be 1024 size: %d\n",(int)strlen(buff));
				break;
			}
		}
	}
	close(s);
	
	return 0;
}
