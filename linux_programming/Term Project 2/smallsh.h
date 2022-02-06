//
// Created by changhyeonnam on 2021/11/19.
//

#ifndef LINUX_PROGRAMMING_SMALLSH_H
#define LINUX_PROGRAMMING_SMALLSH_H
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#define EOL             1
#define ARG             2
#define AMPERSAND       3
#define SEMICOLON       4
#define MAXARG          512
#define MAXBUF          512
#define FOREGROUND      0
#define BACKGROUND      1


int userin(char* p);
void procline();
int gettok(char** outptr);
int inarg(char c);
int runcommand(char** cline, int where,int argc,int out, char* output, int check_pipe);
int cmd_cd(int argc, char* argv[]);
void handle_sigint(int sig);
void handle_sigint_foreground(int sig);
void  handle_sigchld(int sig);

#endif //LINUX_PROGRAMMING_SMALLSH_H
