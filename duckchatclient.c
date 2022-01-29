#include "csapp.h"

int main(int argc, char **argv){
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

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
    int cnt = 0;
    //init message
    while(Rio_readlineb(&rio, buf, MAXLINE) > 0) {
        Fputs(buf, stdout);
        cnt++;
        if (cnt >= 4)
            break;
    }

    // create name
    char name[50];
    while(Fgets(buf, MAXLINE, stdin)) {
        Rio_writen(clientfd, buf, strlen(buf));
        strcpy(name, buf);
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
        if(strcmp(buf,"Named successfully\n") == 0){
            name[strlen(name)-1] = 0;
            break;
        }
    }

    //prompt message
    cnt = 0;
    while(Rio_readlineb(&rio, buf, MAXLINE) > 0) {
        Fputs(buf, stdout);
        cnt++;
        if (cnt >= 1)
            break;
    }
    
    char prompt[100];
    sprintf(prompt, "%s@duckchat>>>", name);
    while(1){
        Fputs(prompt, stdout);
        if (Fgets(buf, MAXLINE, stdin) == NULL){
            break;
        }
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        int num = atoi(buf);
        while(num--) {
            Rio_readlineb(&rio, buf, MAXLINE);
            Fputs(buf, stdout);
        }
    }
    Close(clientfd);
    exit(0);
}
