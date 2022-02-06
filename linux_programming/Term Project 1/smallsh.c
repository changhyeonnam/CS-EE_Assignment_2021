//
// Created by changhyeonnam on 2021/11/19.
//

#include "smallsh.h"
static char inpbuf[MAXBUF];
static char tokbuf[2*MAXBUF];
static char *ptr = inpbuf;
static char *tok = tokbuf;
static char special[] = {' ', '\t', '&', ';', '\n', '\0'};

int userin(char* p){
    int c,count;
    ptr = inpbuf;
    tok = tokbuf;
    printf("%s",p);
    count=0;

    while(1){
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
                    }
                    if(strcmp(arg[0],"cd")==0)
                    {
                        cmd_cd(narg,arg);
                    }
                    else if(strcmp(arg[0],"exit")==0)
                    {
                        exit(0);
                    }

                    else
                        runcommand(arg, type,narg,out,output);
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
int runcommand(char** cline, int where,int argc,int out, char* output) {
    pid_t pid;
    int status;
    int fd;
    switch(pid=fork()){
        case -1:
            perror("smallsh");
            return -1;
        case 0:
            if (out)
            {
                int fd1 = creat(output , 0644) ;
                dup2(fd1, STDOUT_FILENO);
                close(fd1);
            }
            execvp(*cline,cline);
            perror(*cline);
            exit(1);
    }
    if(where==BACKGROUND){
        printf("[Process id] %d\n",pid);
        return 0;
    }
    if(waitpid(pid,&status,0)==-1)
        return -1;
    else
        return status;
}



