#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define MAX_BUF 1024

int main(int argc, char *argv[])
{
	long port = strtol(argv[2], NULL, 10);
	struct sockaddr_in address, cl_addr;
	char *server_address;
	int socket_fd, response;
	int read_tail = 0;
	char prompt[MAX_BUF + 4];
	char username[MAX_BUF];
	char buffer[MAX_BUF + 1];
	fd_set readfds;

	//檢查arguments
	if(argc < 3){
		printf("Usage: ./cc_thread [IP] [PORT]\n");
		exit(1);
	}

	//進入大廳
	printf("Enter your name: ");
	fgets(username, MAX_BUF, stdin);
	username[strlen(username) - 1] = 0;
	strcpy(prompt, username);
	strcat(prompt, "> ");

	//連線設定
	server_address = argv[1];
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(server_address);
	address.sin_port = htons(port);
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	// connect_to_server(socket_fd, &address);
    response = connect(socket_fd, (struct sockaddr *)&address, sizeof(address));
	if(response < 0){
		fprintf(stderr, "connect() failed: %s\n", strerror(errno));
		exit(1);
	}
    else{
		send(socket_fd, username, strlen(username) ,0);
		fprintf(stderr, "Connected\n");
	}

    while(1){
		fprintf(stderr, ">");
		
		//clear the socket set 
        FD_ZERO(&readfds);

        //add master socket to set
		FD_SET(STDIN_FILENO, &readfds);
        FD_SET(socket_fd, &readfds);

        int max_sd = socket_fd;

		//wait for an activity on one of the sockets , timeout is NULL ,so wait indefinitely 
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if((activity < 0) && (errno != EINTR)) printf("select error\n");

		//msg from terminal
		if(FD_ISSET(0, &readfds)){
			if((read_tail = read(STDIN_FILENO, buffer, BUFSIZ)) == 0){
				perror("read()");
				return -1;
			}
			else{
				buffer[read_tail] = '\0';
				send(socket_fd, buffer ,strlen(buffer) ,0);
            }
        }

		//msg from server
		if(FD_ISSET(socket_fd, &readfds)){
			if((read_tail = read(socket_fd, buffer, MAX_BUF)) == 0){
				perror("read()");
				return -1;
			}
			else{
				buffer[read_tail] = '\0';
				fprintf(stderr, "%s", buffer);
            }
        }
    }

	// //建thread data
	// thread_data data;
	// data.prompt = prompt;
	// data.socket = socket_fd;

	// //建thread接收訊息
	// pthread_create(&thread, NULL, (void *)receive, (void *)&data);

	//傳送訊息
	// send_message(prompt, socket_fd, &address,username);

	//關閉
	close(socket_fd);
	//pthread_exit(NULL);

	return 0;
}