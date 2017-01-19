//name Kieran Boyle
//student ID: 1265888
//https://blog.udemy.com/fread-c/
//http://beej.us/guide/bgipc/output/html/multipage/flocking.html
/*
 * Copyright (c) 2008 Bob Beck <beck@obtuse.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <err.h>
#include <errno.h>
#include <sys/file.h>
#define MAX_STR_LENGTH 2000
#define IP 2130706433  /* 127.0.0.1 */
#define BUFFLEN 1025

char file_buffer[BUFFLEN];
static void kidhandler(int signum);

static void kidhandler(int signum) {
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}

int main(int argc, char *argv[]){
	FILE *reading_fp;
	FILE *log_fp;
	DIR *target_dir;
	time_t rawtime;
	struct tm * timeinfo;
	struct tm * sendtime;
	struct sockaddr_in si_me, si_other;
	struct sigaction sa;
	struct flock lock = {F_WRLCK, SEEK_SET,   0,      0,     0 };
	int fd;
	log_fp = fopen(argv[3],"a");

	if (log_fp == 0){
		fprintf(stderr,"The logfile that you are looking for does not exist.\n");
		exit(1);
	}

	if ((fd = open(argv[3], F_WRLCK)) == -1) {
        	fprintf(stderr,"There was an error when opening the lock");
        	exit(1);
   	}
	if (argc != 4){
		fprintf(stderr,"You have not enetered the required parameters\n");
		exit(1);
	}
	int s, i, slen=sizeof(si_other);
	char buf[BUFFLEN];
	int port_no = atoi(argv[1]);
	int res;
	//char buffer[BUFFLEN];
	char path[MAX_STR_LENGTH];
	int flag = 0;
	char time_string_recieved[MAX_STR_LENGTH];
	char time_string_return[MAX_STR_LENGTH];
	pid_t pid;
	strncpy(path, argv[2], strlen(argv[2]));
	//Binding to the socket and doing error handling
	if ( ( s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) == -1 )
	{ 
		printf("Error in creating socket");
		return 1;
	}	
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port_no);
	si_me.sin_addr.s_addr = htonl(IP);
	if ( bind(s, (const struct sockaddr *) &si_me, sizeof(si_me)) == -1 )
	{
		printf("Error in binding the socket");
		return 2;
	}	

	//printf("\n\nServer listening to %s:%d\n\n", inet_ntoa(si_me.sin_addr), ntohs(si_me.sin_port));
	

	target_dir = opendir(argv[2]);
	//printf("%s\n",argv[2]);
	if(target_dir == 0){
		fprintf(stderr,"The directory that you have supplied does not exist\n");
		exit(1);
	}
	closedir(target_dir);
	//below is code form lab 6 and explanations as to what it does

	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 */


	/*
	 * first, let's make sure we can have children without leaving
	 * zombies around when they die - we can do this by catching
	 * SIGCHLD.
	 */
	sa.sa_handler = kidhandler;
        sigemptyset(&sa.sa_mask);
	/*
	 * we want to allow system calls like accept to be restarted if they
	 * get interrupted by a SIGCHLD
	 */
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1)
                err(1, "sigaction failed");
	
	
	//Daemonizing
	if(daemon(1,0) == -1){
		fprintf(stderr,"Daemonization has failed\n");
		exit(1);
	}

	while (1) {
		if ( recvfrom(s, buf, BUFFLEN, 0, (struct sockaddr *) &si_other, (socklen_t *)&slen) != -1){
			//Making the child to serrve a request from them client(s)
			pid = fork();
			
			if (pid == -1){
			    fprintf(stderr, "fork failed");
			}
			if (pid == 0){
				lock.l_pid = getpid();
		
				//printf("\nReceived request from %s:%d  Data: %s\n\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);
				time(&rawtime);
				timeinfo = localtime (&rawtime);
				strftime(time_string_recieved,MAX_STR_LENGTH,"%c",timeinfo);
				strcat(path,"/");
				strcat(path,buf);
				reading_fp = fopen(path,"r");
				printf("%s\n",path);
				//Reading from the file into a buffer 1kb at a time and sending it
				if(reading_fp!= 0){
					while(!feof(reading_fp)){
						res = fread(file_buffer, 1, (sizeof file_buffer)-1, reading_fp);
						if(res == 1024){
							if(sendto(s, file_buffer, strlen(file_buffer) + 1, 0, (const struct sockaddr *) &si_other, sizeof(si_other)) != -1){
								time(&rawtime);
								sendtime = localtime (&rawtime);
								strftime(time_string_return,MAX_STR_LENGTH,"%c",sendtime);
								flag = 1;

							}else{
								flag = 2;
								
							}
						
						 
						}
						//emptying the buffer
						file_buffer[res] = 0;
						

					
					}
				
					//printf("made it out of the main loop\n");
					if (res > 0){
						//printf("do i get in here\n");
						//printf("\nSending: %s to %s:%d\n",file_buffer, inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
						if(sendto(s, file_buffer, strlen(file_buffer) + 1, 0, (const struct sockaddr *) &si_other, sizeof(si_other)) != -1){
							time(&rawtime);
							sendtime = localtime (&rawtime);
							strftime(time_string_return,MAX_STR_LENGTH,"%c",sendtime);
							flag = 1;
						}else{
							flag = 2;

							
						}						
						file_buffer[res] = 0; 
						//break;
					}
					fclose(reading_fp);
				
				}else{
					flag = 3;
				}
				//printf("%s",file_buffer);
				//printf("out of while loop file is getting closed\n");
				//This is where I do all my printing to file
				while(fcntl(fd,F_SETLKW,&lock)== -1){
				}
				if (flag == 1){
					fprintf(log_fp,"<%s> <%d> <%s> <%s> <%s>\n",inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port),buf,time_string_recieved,time_string_return);
				}else if (flag == 2){
					fprintf(log_fp,"<%s> <%d> <%s> <%s> <%s>\n",inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port),buf,time_string_recieved,"transmission not completed");

				}else if (flag == 3){
					
					fprintf(log_fp,"<%s> <%d> <%s> <%s> <%s>\n",inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port),buf,time_string_recieved,"file not found");

				}
				//sleep(5);
				flag = 0;
				lock.l_type = F_UNLCK;
				exit(0);
	
			}
			
			
		}
		

	}
	//closing up the file
	fclose(log_fp);
	close(s);
	return 0;
}
