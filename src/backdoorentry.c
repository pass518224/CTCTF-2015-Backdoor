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
#include <arpa/inet.h>
#include "aes.h"

#define bufsize 1024

//#define port 5566
//char in_key[17]="asdfzxcvqwer1234";
//char out_key[17]="4321rewqfdsavcxz";
char in_key[17]=IN_KEY;
char out_key[17]=OUT_KEY;

int main(int argc, char* argv[]){
	char buf[bufsize];
	FILE* f;
	int sockfd;
	struct sockaddr_in serv_addr;

	if (argc!=2) {
		printf("usage: %s <IPaddress>\n", argv[0]);
		exit(1);
	}

	// initialize and connect to the catcher
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
	serv_addr.sin_port = htons(port);
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0){
		printf("error: failed to connect %s %s\n", argv[1], argv[2]);
		exit(1);
	}

	fd_set fds;
	int input_not_eof = 1;
	int output_not_eof = 1;

	while (/*input_not_eof ||*/ output_not_eof) {
		// set fd
		FD_ZERO(&fds);
		if (input_not_eof) FD_SET(0, &fds);
		if (output_not_eof) FD_SET(sockfd, &fds);

		// maxfd+1, read_set, write_set, err_set, timeout
		select(sockfd+1, &fds, NULL, NULL, NULL);

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
				for (i=0; i<n; i+=16) AES128_ECB_encrypt(buf+i, in_key, buf2+i);
				write(sockfd, buf2, ((n-1)/16+1)*16);
			}
			else {
				input_not_eof = 0;
				close(0);
			}
		}

		// read from service output
		if (output_not_eof && FD_ISSET(sockfd, &fds)) {
			memset(buf, 0, bufsize);
			memset(buf2, 0, bufsize);
			n = read(sockfd, buf, bufsize);
			if (n > 0){	
				int i;
				for (i=0; i<n; i+=16) AES128_ECB_decrypt(buf+i, out_key, buf2+i);
				write(1, buf2, ((n-1)/16+1)*16);
			}
			else {
				output_not_eof = 0;
				close(1);
			}
		}
	}
	close(sockfd);
}