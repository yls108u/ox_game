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
#define PORT 8888
#define MAX_BUF 1024
#define MAX_CLIENT 1024


/*global varibles for normal operations*/
int listen_fd, new_socket;
int activity, valread, max_sd, sd;
int client_socket[MAX_CLIENT];
char client_list[MAX_CLIENT][MAX_BUF];
char buffer_c_to_c[MAX_BUF + 1] = {0};


/*record sign up users*/
char user_name[MAX_CLIENT][MAX_BUF];
char user_passwd[MAX_CLIENT][MAX_BUF];
int checking_sign_in = -1;
int checking_sign_up = -1;


/*global varibles for chat functions*/
int chatter1 = -1, chatter2 = -1, chatter2_confirming = -1;


/*global varibles for ox game and other appending functions*/
char **game = NULL;
int playing = -1, player1 = -1, player2 = -1, player2_confirming = -1;
int watch_battle[MAX_CLIENT][1];
int saver = -1, saving = -1;


/*sign in & up functions*/
int check_record(char *name)
{
    for(int i = 0; i < MAX_CLIENT; i++){
        if(strcmp(user_name[i], name) == 0){
            return 1;
        }
    }
    return 0;
}
int check_passwd(char *name, char* passwd)
{
    for(int i = 0; i < MAX_CLIENT; i++){
        if(strcmp(user_name[i], name) == 0 && strcmp(user_passwd[i], passwd) == 0){
            return 1;
        }
    }
    return 0;
}
int sign_up(char *name, char* passwd)
{
    for(int i = 0; i < MAX_CLIENT; i++){
        if(user_name[i][0] == '\0' && user_passwd[i][0] == '\0'){
            strcpy(user_name[i], name);
            strcpy(user_passwd[i], passwd);
            return 1;
        }
    }
    return 0;
}


/*ox game function*/
void initial_ox_game()
{
    game = malloc(sizeof(char *) * 3);
    for(int i = 0; i < 3; i++) *(game + i) = malloc(sizeof(char) * 4);
    for(int i = 0; i < 3; i++) memset(*(game + i), 0, 4);
    return ;
}
void insert(char *buffer)
{
    int i, j;
    char *ptr = malloc(2);

    *ptr = buffer[0];
    i = atoi(ptr);

    *ptr = buffer[1];
    j = atoi(ptr);

    game[i][j] = buffer[2];

    return ;
}
void show_game()
{
    memset(buffer_c_to_c, 0, MAX_BUF + 1);
    strcat(buffer_c_to_c, "\n");

    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            if(*( *(game + i) + j) == '\0'){
                strcat(buffer_c_to_c, "  ");
                // fprintf(stderr, "  ");
            }
            else{
                strncat(buffer_c_to_c, ( *(game + i) + j), 1);
                strcat(buffer_c_to_c, " ");
                // fprintf(stderr, "%c ", *( *(game + i) + j) );
            }
        }
        strcat(buffer_c_to_c, "\n");
    }

    return ;
}
int check_winner()
{
    for(int i = 0; i < 3; i++){
        if(*( *(game + i) + 0) != '\0' && \
           *( *(game + i) + 0) == *( *(game + i) + 1) && \
           *( *(game + i) + 0) == *( *(game + i) + 2)){
               return 1;
        }
    }

    for(int i = 0; i < 3; i++){
        if(*( *(game + 0) + i) != '\0' && \
           *( *(game + 0) + i) == *( *(game + 1) + i) && \
           *( *(game + 0) + i) == *( *(game + 2) + i)){
               return 1;
        }
    }

    if(*( *(game + 1) + 1) != '\0' && \
       *( *(game + 1) + 1) == *( *(game + 0) + 0) && \
       *( *(game + 1) + 1) == *( *(game + 2) + 2)){
           return 1;
    }

    if(*( *(game + 1) + 1) != '\0' && \
       *( *(game + 1) + 1) == *( *(game + 0) + 2) && \
       *( *(game + 1) + 1) == *( *(game + 2) + 0)){
           return 1;
    }

    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            if(*( *(game + i) + j) == '\0'){
                return 0;
            }
        }
    }
    return 2; //draw
}
int ox_game_handler(char *ptr)
{
    insert(ptr);

    //send result to player1 & 2, if game is ended, return 1 or 2
    show_game();
    int end_game = check_winner();
    if(end_game > 0) return end_game;

    //change playing player and send notify
    if(playing == player1){
        send(player1, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
        strcat(buffer_c_to_c, ">now, it's your turn.\n");
        send(player2, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
        playing = player2;
    }
    else{
        send(player2, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
        strcat(buffer_c_to_c, ">now, it's your turn.\n");
        send(player1, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
        playing = player1;
    }
    memset(buffer_c_to_c, 0, MAX_BUF + 1);

    return 0;
}


/*main function*/
int main(int argc, char *argv[])
{
    struct sockaddr_in address;
	socklen_t addr_len;
	char buffer[MAX_BUF + 1] = {0};
    fd_set readfds;
    FILE *file;
    char filename[MAX_BUF + 1] = {0};

    //initialize client_socket
    for(int i = 0; i < MAX_CLIENT; i++){
        client_socket[i] = 0;
        memset(client_list[i], 0, MAX_BUF);
        memset(user_name[i], 0, MAX_BUF);
        memset(user_passwd[i], 0, MAX_BUF);
    }

    //create server fd
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }

    //type of socket created
    address.sin_family = AF_INET;
	address.sin_port = htons(PORT);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind the socket to localhost port 8888
    if(bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0){  
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //start listening
    printf("Listener on port %d\n", PORT);
    if(listen(listen_fd, 5) < 0){  
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addr_len = sizeof(address);
    printf("Waiting for connection\n");

    while(1){
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(listen_fd, &readfds);
        max_sd = listen_fd;
             
        //add child socket
        for(int i = 0; i < MAX_CLIENT; i++){
            sd = client_socket[i];
            if(sd > 0) FD_SET(sd, &readfds);  
            if(sd > max_sd) max_sd = sd;  
        }  

        //waiting for an activity
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if((activity < 0) && (errno != EINTR)) printf("select error\n");

        //new connection
        if(FD_ISSET(listen_fd, &readfds)){
            if((new_socket = accept(listen_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len)) < 0){
                perror("accept");
                exit(EXIT_FAILURE);
            }
            // printf("New connection.\nsocket fd: %d\tip: %s\tport: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //add new socket
            for (int i = 0; i < MAX_CLIENT; i++){
                if(client_socket[i] == 0){
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);

                    valread = read(new_socket, buffer, MAX_BUF);
                    buffer[valread] = '\0';
                    strcpy(client_list[i], buffer);
                    memset(buffer, 0, MAX_BUF + 1);
                    printf("user name: %s\n", client_list[i]);

                    if(check_record(client_list[i])){ //type passwd(sign in)
                        checking_sign_in = 1;
                        send(client_socket[i], "passwd:\n", 10, 0);
                    }
                    else{ //sign up
                        checking_sign_up = 1;
                        send(client_socket[i], "please sign up, passwd:\n", 26, 0);
                    }

                    break;
                }
            }
        }

        if(FD_ISSET(0, &readfds)){
            valread = read(listen_fd, buffer, MAX_BUF);
            buffer[valread] = '\0';
            // if(strncmp(buffer, "/quit", 5) == 0){
            //     fprintf(stderr, "closing...\n");
            //     close(listen_fd);
            //     exit(0);
            // }
        }

        //activity from other socket
        for(int i = 0; i < MAX_CLIENT; i++){
            memset(buffer, 0, MAX_BUF + 1);
            memset(buffer_c_to_c, 0, MAX_BUF + 1);
            sd = client_socket[i];

            if(FD_ISSET(sd, &readfds)){
                //disconnect, clear all information
                if((valread = read(sd, buffer, MAX_BUF)) == 0){
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addr_len);  
                    // printf("Host disconnected.\nip: %s\tport: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    fprintf(stderr, "user %s is leaving...\n", client_list[i]);
                    close(sd);  
                    client_socket[i] = 0;
                    memset(client_list[i], 0, MAX_BUF);
                }
                //recv message
                else{
                    buffer[valread] = '\0';
                    // fprintf(stderr, "---buffer:\n%s\n---\n", buffer);

                    //check sign in or sign up
                    if(check_record(client_list[i]) && checking_sign_in == 1){ //sign in
                        if(check_passwd(client_list[i], buffer)){
                            checking_sign_in = -1;
                            send(client_socket[i], "passwd confirm, welcome back\n", 31, 0);
                        }
                        else send(client_socket[i], "incorrect passwd, please type again:\n", 39, 0);
                    }
                    else if(!check_record(client_list[i]) && checking_sign_up == 1){ //sign up
                        if(sign_up(client_list[i], buffer)){
                            checking_sign_up = -1;
                            send(client_socket[i], "sign up success, welcome\n", 27, 0);
                        }
                        else{
                            checking_sign_up = -1;
                            send(client_socket[i], "sign up failed, you can just be a visiter\n", 44, 0);
                        }
                    }

                    //offerred functions
                    if(strncmp(buffer, "/list", 5) == 0){
                        // fprintf(stderr, "sec 0\n");
                        strcat(buffer_c_to_c, "online users: ");

                        for(int i = 0; i < MAX_CLIENT; i++){
                            if(client_socket[i] != 0){
                                strcat(buffer_c_to_c, client_list[i]);
                                strcat(buffer_c_to_c, " ");
                            }
                        }
                        strcat(buffer_c_to_c, "\n");
                        send(sd, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                    }
                    else if(strncmp(buffer, "/play", 5) == 0 || player1 == sd || player2 == sd){ //only one ox game can be accepted
                        if(player1 == -1 && player2 == -1){
                            // fprintf(stderr, "sec 1-0\n");
                            player1 = sd;
                            send(sd, "choose an online user you want to play with\n", 46, 0);
                        }
                        else if(player1 == sd && player2 == -1){
                            // fprintf(stderr, "sec 1-1\n");
                            for(int j = 0; j < MAX_CLIENT; j++){
                                if(client_socket[j] != 0 && strncmp(client_list[j], buffer, strlen(client_list[j])) == 0){
                                    player2 = client_socket[j];
                                    player2_confirming = 0;
                                    strcat(buffer_c_to_c, "user ");
                                    strcat(buffer_c_to_c, client_list[i]);
                                    strcat(buffer_c_to_c, " want to play ox game with you, confirm?(y/n)\n");
                                    send(client_socket[j], buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                                    // memset(buffer_c_to_c, 0, MAX_BUF + 1);
                                }
                            }
                        }
                        else if(player2 == sd && player2_confirming == 0){
                            // fprintf(stderr, "sec 1-2\n");
                            if(!strncmp(buffer, "y", 1)){
                                player2_confirming = 1;
                                playing = player1;
                                strcat(buffer_c_to_c, "user ");
                                strcat(buffer_c_to_c, client_list[i]);
                                strcat(buffer_c_to_c, " accept your invitation\n");
                                strcat(buffer_c_to_c, ">now, it's your turn.\n");
                                send(player1, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                                // memset(buffer_c_to_c, 0, MAX_BUF + 1);
                                initial_ox_game();
                            }
                            else if(!strncmp(buffer, "n", 1)){
                                // fprintf(stderr, "sec 1-3\n");
                                strcat(buffer_c_to_c, "user ");
                                strcat(buffer_c_to_c, client_list[i]);
                                strcat(buffer_c_to_c, " reject your invitation\n");
                                send(player1, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                                // memset(buffer_c_to_c, 0, MAX_BUF + 1);
                                
                                player1 = -1;
                                player2 = -1,
                                player2_confirming = -1;
                            }
                            else{
                                // fprintf(stderr, "sec 1-4\n");
                                send(sd, "please retype your answer(y/n).\n", 33, 0);
                            }
                        }
                        else if(player2_confirming == 1){
                            // fprintf(stderr, "sec 2\n");
                            int end_game = ox_game_handler(buffer);
                            if(end_game > 0){ //return 1 means game is end, reset. return 2表示平手(draw)
                                if(end_game == 1){
                                    strcat(buffer_c_to_c, ">player ");
                                    for(int j = 0; j < MAX_CLIENT; j++){
                                        if(playing == client_socket[j]){
                                            strcat(buffer_c_to_c, client_list[j]);
                                            break;
                                        }
                                    }
                                    strcat(buffer_c_to_c, " wins!\n>game is over.\n");
                                }
                                else{
                                    strcat(buffer_c_to_c, ">draw!\n>game is over.\n");
                                }
                                send(player1, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                                send(player2, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                                player1 = -1;
                                player2 = -1;
                                playing = -1;
                                player2_confirming = -1;
                            }
                        }
                    }
                    //
                    else if(strncmp(buffer, "/chat", 5) == 0 || chatter1 == sd || chatter2 == sd || strncmp(buffer, "/end", 4) == 0){
                        if(chatter1 == -1 && chatter2 == -1){
                            chatter1 = sd;
                            send(sd, "choose an online user you want to chat\n", 41, 0);
                        }
                        else if(chatter1 == sd && chatter2 == -1){
                            for(int j = 0; j < MAX_CLIENT; j++){
                                if(client_socket[j] != 0 && strncmp(client_list[j], buffer, strlen(client_list[j])) == 0){
                                    chatter2 = client_socket[j];
                                    chatter2_confirming = 0;
                                    strcat(buffer_c_to_c, "user ");
                                    strcat(buffer_c_to_c, client_list[i]);
                                    strcat(buffer_c_to_c, " want to chat, confirm?(y/n)\n");
                                    send(client_socket[j], buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                                    // memset(buffer_c_to_c, 0, MAX_BUF + 1);
                                }
                            }
                        }
                        else if(chatter2 == sd && chatter2_confirming == 0){
                            if(!strncmp(buffer, "y", 1)){
                                chatter2_confirming = 1;
                                strcat(buffer_c_to_c, "user ");
                                strcat(buffer_c_to_c, client_list[i]);
                                strcat(buffer_c_to_c, " accept your invitation\n");
                                strcat(buffer_c_to_c, ">if you want to end the chat, type \"/end\"\n");
                                send(chatter1, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                                send(chatter2, ">if you want to end the chat, type \"/end\"\n", 44, 0);
                                // memset(buffer_c_to_c, 0, MAX_BUF + 1);
                            }
                            else if(!strncmp(buffer, "n", 1)){
                                strcat(buffer_c_to_c, "user ");
                                strcat(buffer_c_to_c, client_list[i]);
                                strcat(buffer_c_to_c, " reject your invitation\n");
                                send(chatter1, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                                // memset(buffer_c_to_c, 0, MAX_BUF + 1);
                                
                                chatter1 = -1;
                                chatter2 = -1,
                                chatter2_confirming = -1;
                            }
                            else{
                                send(sd, "please retype your answer(y/n).\n", 33, 0);
                            }
                        }
                        else if(chatter2_confirming == 1 && strncmp(buffer, "/end", 4) == 0){
                            strcat(buffer_c_to_c, "user ");
                            strcat(buffer_c_to_c, client_list[i]);
                            strcat(buffer_c_to_c, " end the chat\n");
                            send(chatter1, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                            send(chatter2, buffer_c_to_c, strlen(buffer_c_to_c) + 1, 0);
                            chatter1 = -1;
                            chatter2 = -1;
                            chatter2_confirming = -1;
                        }
                        else if(chatter2_confirming == 1){
                            if(sd == chatter1){
                                send(chatter2, buffer, strlen(buffer) + 1, 0);
                            }
                            else{
                                send(chatter1, buffer, strlen(buffer) + 1, 0);
                            }
                        }
                    }
                    //
                    else if(strncmp(buffer, "/save", 5) == 0 || saver == sd || saving == sd || (saving == sd && strncmp(buffer, "/stop", 5) == 0)){
                        if(saver == -1){
                            saver = sd;
                            send(sd, "type your [file.txt] name:\n", 29, 0);
                        }
                        else if(saver == sd && saving == -1){
                            saving = sd;
                            char *ptr = NULL;
                            if((ptr = strstr(buffer, ".txt")) == NULL){
                                send(sd, "please type [file.txt]\n", 25, 0);
                                // memset(buffer, 0, MAX_BUF + 1);
                                // memset(buffer_c_to_c, 0, MAX_BUF + 1);
                                // continue;
                            }
                            strncpy(filename, buffer, ptr - buffer + 4);
                            send(sd, "type your content\n>if you want to stop, type \"/stop\"\n", 55, 0);
                        }
                        else if(strncmp(buffer, "/stop", 5) == 0){
                            saver = -1;
                            saving = -1;
                            memset(filename, 0, MAX_BUF + 1);
                            send(sd, "stop successfully\n", 20, 0);
                        }
                        else if(saver == sd && saving == sd){
                            file = fopen(filename, "a+");
                            buffer[strlen(buffer)] = '\0';
                            fwrite(buffer, sizeof(char), strlen(buffer), file);
                            fclose(file);
                        }
                    }
                    else if(strncmp(buffer, "/quit", 5) == 0){
                        fprintf(stderr, "user %s leaving...\n", client_list[i]);
                        close(sd);  
                        client_socket[i] = 0;
                        memset(client_list[i], 0, MAX_BUF);
                    }

                    memset(buffer, 0, MAX_BUF + 1);
                    memset(buffer_c_to_c, 0, MAX_BUF + 1);
                }
            }
        }
    }

    return 0;
}