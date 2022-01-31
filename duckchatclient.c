#include "csapp.h"

int clientfd;
char *host, *port, buf[MAXLINE];
rio_t rio;
char name[50];
char prompt[100];
int cnt = 0;

void Chat_send()
{
    // Fputs(prompt, stdout);
    if (Fgets(buf, MAXLINE, stdin) == NULL){
        return;
    }
    Rio_writen(clientfd, buf, strlen(buf));
}
void Chat_receive()
{
    //Receive and print message
    // Fputs("Receiving messages...\n", stdout);
    Rio_writen(clientfd, "re\n", 3);
    Rio_readlineb(&rio, buf, MAXLINE);
    int num = atoi(buf); // Lines received from server
    while(num--) {
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    usleep(100000);
}


int main(int argc, char **argv){

    if(argc != 3){
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    if (clientfd < 0){
        printf("Connection failed\n");
        exit(0);
    }
    Rio_readinitb(&rio, clientfd);

    //init message
    while(Rio_readlineb(&rio, buf, MAXLINE) > 0) {
        Fputs(buf, stdout);
        cnt++;
        if (cnt >= 4)
            break;
    }

    // create name
    while(Fgets(buf, MAXLINE, stdin)) {
        Rio_writen(clientfd, buf, strlen(buf));
        strcpy(name, buf);
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
        if(strcmp(buf,"Named successfully\n") == 0){ // Check if succeeded
            name[strlen(name)-1] = 0;
            break;
        }
    }

    pid_t child_pid = Fork();
    if(child_pid) // Main process is used to send message
    {
        sprintf(prompt, "%s@duckchat >>> ", name);
        while(1) Chat_send();
        Close(clientfd);
        exit(0);
    }
    else
    {
        while(1) Chat_receive();
        Close(clientfd);
        exit(0);
    }
}
