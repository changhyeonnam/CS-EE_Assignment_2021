//
// Created by changhyeonnam on 2021/11/19.
//

#include "smallsh.h"
#define NPROC 10

static char inpbuf[MAXBUF];
static char tokbuf[2*MAXBUF];
static char *ptr = inpbuf;
static char *tok = tokbuf;
static char special[] = {' ', '\t', '&', ';', '\n', '\0'};
static int children[NPROC];
static int nproc = 0;
static int flag_SIGINT_foregorund;
int userin(char* p){
    int c,count;
    ptr = inpbuf;
    tok = tokbuf;
    char buf[MAXBUF];
    getcwd(buf,MAXBUF);
    printf("%s$ ",buf);
    count=0;

    while(1){
        signal(SIGINT,handle_sigint);

        if((c=getchar())==EOF)
            return EOF;
        if(count < MAXBUF)
            inpbuf[count++]=c;
        if(c=='\n' &&count<MAXBUF){
            inpbuf[count]='\0';
            return count;
        }
        if(c=='\n' || count>=MAXBUF){
            printf("smallsh: input line too long\n");
            count = 0;
            printf("%s",p);
        }
    }
}
int gettok(char** outptr){
    int type;
    *outptr = tok;
    while(*ptr== ' '|| *ptr=='\t')
        ptr++;
    *tok++ = *ptr;
    switch(*ptr++){
        case '\n':
            type = EOL;
            break;
        case '&':
            type = AMPERSAND;
            break;
        case ';':
            type = SEMICOLON;
            break;
        default:
            type = ARG;
            while(inarg(*ptr))
                *tok++ = *ptr++;
    }
    *tok++ = '\0';
    return type;
}

int inarg (char c){
    char* wrk;
    for(wrk=special;*wrk;wrk++){
        if(c==*wrk)
            return 0;
    }
    return 1;
}


void procline() {
    char *arg[MAXARG + 1];
    int toktype, type;
    int narg = 0;
    for (;;) {
        int out = 0;
        int check_pipe = 0;
        char* output=NULL;
        switch (toktype = gettok(&arg[narg])) {
            case ARG:
                if (narg < MAXARG)
                    narg++;
                break;
            case EOL:
            case SEMICOLON:
            case AMPERSAND:
                if (toktype == AMPERSAND) type = BACKGROUND;
                else                      type = FOREGROUND;
                if (narg != 0) {
                    arg[narg] = NULL;
                    for(int i=0;i<narg;i++){
                        if(strcmp(arg[i],">")==0) {
                            out = 1;
                            output = arg[i+1];
                            arg[i]=NULL;
                        }
                        if(strcmp(arg[i],"|")==0) {
                            check_pipe = 1;
                        }
                    }

                    if(strcmp(arg[0],"cd")==0)
                    {
                        cmd_cd(narg,arg);
                    }
                    else if(strcmp(arg[0],"exit")==0)
                    {
                        exit(0);
                    }

                    else {
                        signal(SIGINT,handle_sigint_foreground);
                        if(flag_SIGINT_foregorund){
                            flag_SIGINT_foregorund=0;
                            return;
                        }
                        runcommand(arg, type, narg, out, output,check_pipe);
                    }
                }
                if (toktype == EOL)       return;
                narg = 0;
                break;
        }
    }
}

int cmd_cd(int argc, char* argv[]){
    if( argc == 1 )
        chdir( getenv( "HOME" ) );
    else if( argc == 2 ){
        if( chdir( argv[1] ) )
            printf( "No directory\n" );
    }
    else

        printf( "You should use cd like this. ->  cd [dir]\n" );

    return 0;
}

void handle_sigchld(int sig) {
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}
void handle_sigint(int sig) {
    return;
}
void handle_sigint_foreground(int sig) {
    int i;
    for (i = 0; i < nproc; i++) {
        if (kill(children[i], SIGTERM) < 0) {
            perror("kill");
        }
    }
    printf("\n");
    nproc=0;
    flag_SIGINT_foregorund = 1;
}
int run_pipe(char **cline){
    pid_t pid1,pid2;
    int fd[2];
    int i;
    int status;
    char* command1[MAXBUF];
    char* command2[MAXBUF];

    for(i=0;cline[i]!=NULL;i++){
        if(!strcmp(cline[i],"|")){
            command1[i]= NULL;
            break;
        }
        command1[i]= cline[i];
    }
    int k=0;
    for(i=i+1;cline[i]!=NULL;i++){
        command2[k]= cline[i];
        k++;
    }
    pipe(fd);

    switch(pid1 = fork()){
        case -1:
            perror("smallsh");
            return -1;
        case 0:
            dup2(fd[1],STDOUT_FILENO);
            close(fd[1]);
            close(fd[0]);
            execvp(command1[0],command1);
            exit(0);
        default:
            wait(&status);
            dup2(fd[0],STDIN_FILENO);
            close(fd[0]);
            close(fd[1]);
            execvp(command2[0],command2);
            exit(0);
    }

    return 0;
}
int runcommand(char** cline, int where,int argc,int out, char* output,int check_pipe) {
        pid_t pid;
        int status;
        int fd;
        switch(pid=fork()){
            case -1:
                perror("smallsh");
                return -1;
            case 0:
                if(check_pipe){
                    return run_pipe(cline);
                }a
                children[nproc]=pid;
                nproc++;
                if (out)
                {
                    int fd1 = creat(output , 0644) ;
                    dup2(fd1, STDOUT_FILENO);
                    close(fd1);
                }

                execvp(*cline, cline);
                perror(*cline);
                exit(1);
        }
        if(where==BACKGROUND){
            printf("[Process id] %d\n",pid);
            struct sigaction sa;
            sa.sa_handler = &handle_sigchld;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
            if (sigaction(SIGCHLD, &sa, 0) == -1) {
                perror(0);
                exit(1);
            }
            return 0;
        }
        if(waitpid(pid,&status,0)==-1)
            return -1;
        else
            return status;
}




