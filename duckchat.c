#include "csapp.h"
#include <time.h>
#define MAXNAME 50
#define MAXINF 502
#define FRONT 0
#define REAR 1

struct information{
    int connfd;
    int pos;
};

int namenum = 0;
const int helpnum = 5;
char names[MAXNAME][MAXNAME];
char help[MAXNAME][200] = {"Hints:\n",">>>help: show this table\n",">>>names: show all the names of the users\n",
">>>send <\"...\"> (to <name>...): Send message(...) (to user(s)) if the command includes \"to\", otherwise it has the same effect as echo does.\n",
">>>refresh or >>>re: Updating messages from your message queue\n"};
char *uc = "Undefined command\n";
char *nonews = "No news feed\n";

/* queue */
char messages[MAXNAME][MAXINF][MAXINF] = {0};
int messagenum[MAXNAME][2] = {0}; //0: front, 1: rear
void gettime(char *timestr);

sem_t mutex;

void strlwr(char *str);
int readsentence(char *start, char *message);
void addmessage(int pos, char *sentence);

int parse(char *buf, char **lines, int pos){
    int i = 0, len = (int)strlen(buf);
    while((buf[i] == ' ' || buf[i] == '\n') && i < len)
        i++;
    int j = i;
    while(!(buf[j] == ' ' || buf[j] == '\n') && j < len)
    j++;
    buf[j] = 0;
    if(strcmp(buf+i, "help") == 0){
        int k = 0;
        for(; k < helpnum; k++){
            lines[k] = help[k];
        }
        return helpnum;
    }
    else if(strcmp(buf+i, "names") == 0){
        int k = 0;
        for(; k < namenum; k++){
            lines[k] = names[k];
        }
        return namenum;
    }
    else if(strcmp(buf+i, "send") == 0){
        i = j+1;
        while((buf[i] == ' ' || buf[i] == '\n') && i < len)
            i++;
        char message[MAXLINE];
        int tmp = i;
        // parse sentence and copy it to message
        i = readsentence(buf+i, message) + i;
        // sentence error(return -1)
        if(i < tmp){
            lines[0] = message;
            return 1;
        }
    
        while((buf[i] == ' ' || buf[i] == '\n') && i < len)
            i++;
        // check whether the command includes "to" or not
        if(buf[i]){
            if(buf[i] == 't' && buf[i+1] == 'o'){
                i += 2;
                // including to, then getting names
                while((buf[i] == ' ' || buf[i] == '\n') && i < len)
                    i++;
                if(!buf[i]){ // including "to" but missing names
                    strcpy(message, "Error: names not found. Format: send <\"...\"> to <name>...\n");
                    lines[0] = message;
                    return 1;
                }
                // parsing names
                char curname[MAXNAME];
                j = i;
                while(!(buf[j] == ' ' || buf[j] == '\n') && j < len)
                    j++;
                buf[j] = 0;
                //first name
                strcpy(curname, buf+i);
                curname[strlen(curname)+1] = 0;
                curname[strlen(curname)] = '\n';
                printf("Got target name: %s", curname);
                int k = 0, flag = 0;
                P(&mutex);
                for(; k<namenum; k++){
                    if(strcmp(curname, names[k]) == 0){
                        if(k == pos){
                            flag = -1;
                            break;
                        }
                        flag = 1;
                        // too many unread messages(Queue overflows)
                        if((messagenum[k][REAR] + 1) % MAXINF == messagenum[k][FRONT]){
                            V(&mutex);
                            sprintf(message, "Overflow error: Too many unread messages for user %s", curname);
                            lines[0] = message;
                            return 1;
                        }
                        message[strlen(message)-1] = 0;
                        // adding tail information
                        char tail[MAXINF];
                        char timestr[30];
                        gettime(timestr);
                        char tmpname[MAXNAME];
                        strcpy(tmpname, names[pos]);
                        tmpname[strlen(tmpname)-1] = 0;
                        sprintf(tail, " --From user %s %s\n", tmpname, timestr);
                        strcat(message, tail);
                        strcpy(messages[k][messagenum[k][REAR]], message);
                        messagenum[k][REAR] = (messagenum[k][REAR] + 1) % MAXINF;
                        V(&mutex);
                        strcpy(message, "Message conveyed successfully\n");
                    }
                }
                V(&mutex);
                // user not found
                if(flag == 0){
                    strcpy(message, "Error: user not found\n");
                }
                else if(flag == -1){
                    strcpy(message, "Error: you can't send a message to yourself!\n");
                }
            }
            // missing "to"
            else{
                strcpy(message, "Syntax error. Format: send <\"...\"> (to <name>...)\n");
            }
        }
        // without "to": echo
        lines[0] = message;
        return 1;
    }
    else if(strcmp(buf+i, "refresh") == 0 || strcmp(buf+i, "re")== 0){
        int k = 0;

        P(&mutex);
        while(messagenum[pos][FRONT] != messagenum[pos][REAR]){
            lines[k] = messages[pos][messagenum[pos][FRONT]];
            k++;
            messagenum[pos][FRONT] = (messagenum[pos][FRONT] + 1) % MAXINF;
        }
        V(&mutex);
        if(k > 0)
            return k;
        else{
            return 0;
        }
    }
    // undefined command
    lines[0] = uc;
    return 1;
}

int readsentence(char *start, char *message){
    int i = 0;
    if(start[i] != '\"'){
        strcpy(message, "Error type: syntax error(missing double quotation).Format: send <\"...\"> (to <name>...)\n");
        return -1;
    }
    i++;
    char sentence[MAXLINE];
    int len = 0;
    while(start[i] != '\"' && start[i]){
        if(start[i] == '\\'){
            sentence[len++] = start[++i];
        }
        else{
            sentence[len++] = start[i];
        }
        i++;
    }
    if(start[i] != '\"'){
        strcpy(message, "Syntax error: missing double quotation. Format: send <\"...\"> (to <name>...)\n");
        return -1;
    }
    int j = 0, flag = 0;
    sentence[len++] = '\n';
    sentence[len] = 0;
    for(;j<len-1;j++){
        if(sentence[j] != ' '){
            flag = 1;
            break;
        }
    }
        if (flag == 0){
        strcpy(message, "Error: empty message\n");
        return -1;
    }
    if (len >= MAXINF){
        sprintf(message, "Error: the message mustn't be longer than %d bytes(or %d bytes including '\\n')\n", MAXINF-2, MAXINF-1);
        return -1;
    }
    printf("Got sentence of %d bytes: %s", len, sentence);
    strcpy(message, sentence);
    return i+1;
}

void login(int connfd, int pos){
    size_t n;
    char buf[MAXLINE];
    rio_t rio;
    Rio_readinitb(&rio, connfd);
    char *words[4] = {"Hello there! Welcome to duckchat. Here you can chat with others that are online.\n",
    "Now please enter your favorite name to create your temporary account, which will be destroyed after you quit.\n",
    "Warning: please do use English or other characters that are encoded by ASCII, or it may causes some unexpected errors.（其实是可以用中文等uniode字符的，至少macos不会把客户端卡掉）\n",
    "Enter your name:\n"};
    int i;
    for(i = 0; i < 4; i++){
        strcpy(buf, words[i]);;
        Rio_writen(connfd, buf, strlen(buf));
    }

    /* name part */
    char name[MAXLINE];
    while(Rio_readlineb(&rio, name, MAXLINE) != 0){
        int err = 0;
        if(strcmp(name,"\n") == 0){
            err = 1;
        }
        int j = 0, len = strlen(name);
        for(;j<len;j++){
            if(name[j] == ' '){
                err = 1;
                break;
            }
        }

        if (err == 1){
            char *message = "Error: The name can't be empty and it mustn't include space(\" \").Try again.\n";
            Rio_writen(connfd, message, strlen(message));
            continue;
        }
        // check length
        if (len > 31){
            char *message = "Error: The name can't be longer than 30 bytes.Try again.\n";
            Rio_writen(connfd, message, strlen(message));
            continue;
        }
        // check DANGEROUS words
        char tmp[MAXNAME];
        strcpy(tmp, name);
        strlwr(tmp);
        if(strcmp(tmp, "cen\n") == 0 || strcmp(tmp, "yyac\n") == 0 || strcmp(tmp, "chenyixin\n") == 0 || strcmp(tmp, "cenyixin\n") == 0 || strcmp(tmp, "aicen\n") == 0){
            char *message = "AICEN error: Dangerous name caught.Try again!\n";
            Rio_writen(connfd, message, strlen(message));
            continue;
        }
        // check uniqueness
        int k = 0, flag = 0;
        P(&mutex);
        printf("namenum: %d\n", namenum);
        for(; k<namenum;k++){
            printf("Comparing: %s", names[k]);
            int ret;
            if((ret=strcmp(name, names[k])) == 0){
                char *message = "Error: The name has been created.Try again.\n";
                Rio_writen(connfd, message, strlen(message));
                flag = 1;
            }
            printf("strcmp returns %d\n", ret);
        }
        if(flag == 1){
            V(&mutex);
            continue;
        }

        char *message = "Named successfully\n";
        Rio_writen(connfd, message, strlen(message));
        goto flag;
    }
    P(&mutex);
    strcpy(names[pos], "ctrl + c interrupt\n");
    namenum++;
    V(&mutex);
    return;
    flag:
    strcpy(names[pos], name);
    namenum++;
    V(&mutex);
    printf("Got name: %s", name);

    char *message = "Now you can communicate with others with your terminal.Enter \"help\" to get some hints.\n";
    Rio_writen(connfd, message, strlen(message));
    name[strlen(name)-1] = 0;


    /* interpreter part */
    while(Rio_readlineb(&rio, buf, MAXLINE) != 0){
        if(strcmp(buf, "re\n")) // If received "re", do not show it in log. 
        {
            printf("server received %ld bytes from %s: %s", strlen(buf), name, buf);
        }
        char **lines = (char **) Malloc(sizeof(char *) * MAXNAME);
        int num = parse(buf, lines, pos);
        char numstring[16] = {0};
        sprintf(numstring, "%d\n", num);
        Rio_writen(connfd, numstring, strlen(numstring)); //header
        int j = 0;
        for(; j < num; j++){ // text(lines)
            Rio_writen(connfd, lines[j], strlen(lines[j]));
        }
        free(lines);
    }
}

void BlockSigno(int signo)
{
    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, signo);
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
}

void *thread(void *vargp){
    int connfd = ((struct information *)vargp)->connfd;
    int pos = ((struct information *)vargp)->pos;
    Pthread_detach(pthread_self());
    Free(vargp);
    BlockSigno(SIGPIPE);

    login(connfd, pos);
    Close(connfd);
    printf("User: TID %ld exited\n", pthread_self());

    P(&mutex);
    names[pos][strlen(names[pos])-1] = 0;
    strcat(names[pos], "(log out)\n");
    V(&mutex);

    return NULL;
}

int main(int argc, char **argv){
    int listenfd,  i = 0;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid[MAXLINE];
    struct information *inf;

    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    Sem_init(&mutex, 0, 1);

    listenfd = Open_listenfd(argv[1]);
    if (listenfd > 0 )
        printf("listenfd bound successfully\n");
    else{
        printf("cannot bind this port to listenfd\n");
        exit(0);
    }
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        inf = Malloc(sizeof(struct information));
        inf->connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        P(&mutex);
        inf->pos = i;
        i++;
        V(&mutex);

        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        pthread_create(tid+inf->pos, NULL, thread, inf);

        char timestr[MAXINF];
        gettime(timestr);
        printf("Connected to (%s, %s) %s with tid %ld\n", 
        client_hostname, client_port, timestr, tid[i-1]);
    }
    exit(0);
}

void strlwr(char *str){
    int i = 0;
    while(str[i]){
        if('A' <= str[i] && str[i] <= 'Z'){
            str[i] = str[i] - ('A' - 'a');
        }
        i++;
    }
}

void gettime(char *timestr){
    time_t now;
    struct tm *t;
    time(&now);
    t = gmtime(&now);
    char tmp[MAXINF];
    sprintf(tmp, "at %.2d:%.2d:%.2d, in %d-%d-%d", 8 + t->tm_hour, t->tm_min, t->tm_sec, 1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday);
    strcpy(timestr, tmp);
}