# Term Project 2: Improving smallsh

### 1. 개발 환경

- 운영체제 : Mac OS X (Ubuntu에서도 정상  작동)

- IDE : Clion

- 언어 : C

- Compiler : gcc

### 2. Term Project 2: Improving smallsh 설명

Term Project1에서 확장된 프로젝트였습니다. Term project1에서의 '&' (background process) 문제점을 해결하고,  prompt에 “Command>” 대신 “현재 디렉토리” 표시, SIGINT 처리 (Ctrl+C), ‘|’ (pipe) 처리에 대해 구현하는 문제였습니다.

### 3. '&' 처리 (background process) 문제점 해결

```c
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
```

background로 실행했을때, child가 종료되어 좀비프로세스되는것이 문제였습니다. child process들이 종료되어, SIGCHLD 시그널을 보내게 될때 handle_sigchld이라는 시그널 핸들러함수를 실행시켜 좀비프로세스가 생성되지 않게 해결했습니다. 이때, 핸들러의 플래그로 SA_RESTART, SA_NOCLDSTOP를 masking하여 sigchld가 왔을때, child process가 continue, restart와 같은 동작을 방지하도록 구현했습니다.

```c
void handle_sigchld(int sig) {
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}
```

다음은 SIGCHLD에 대한 시그널 해들러 함수입니다.

### 4. 입력 prompt에 “Command>” 대신 “현재 디렉토리” 표시

```c
int userin(char* p){
    int c,count;
    ptr = inpbuf;
    tok = tokbuf;
    char buf[MAXBUF];
    getcwd(buf,MAXBUF);
    printf("%s$ ",buf);
...
}
```

다음과 같이 userin 앞부분에 getcwd(get current directory)함수를 이용하여 간단하게 구현할 수 있었습니다.(prompt를 인자로  받아오는 부분은 제거하지 않았습니다.)

### 5. SIGINT 처리 (Ctrl+C)

추가적으로 여러 process를 foreground로 받았을때, smallsh 로 돌아가게 구현했습니다.

```c
$ sleep 10; sleep 5
```

만약 이를 고려하지 않으면 위와 같은  command에 대해서 ctrl+c를 두번 해야 smallsh로 돌아가게 됩니다.

먼저 smallsh로 돌아가는 code입니다.

```c
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
...
}
void handle_sigint(int sig) {
    return;
}
```

token을 받기 위해 for문을 받는 곳에 SIGINT에 대해 handle_sigint 핸들러함수가 호출되게 구현했습니다. smallsh에서 foreground에 어떠한 프로새스도 실행시키지 않을경우엔 바로 입력  prompt가 바로 나오게 해야합니다.

두번째로 foreground에 대한 모든 프로세스가 종료되게 하는 code입니다.

```c
int userin(char*p{
...
signal(SIGINT,handle_sigint_foreground);
  if(flag_SIGINT_foregorund){
      flag_SIGINT_foregorund=0;
      return;
}
...
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
```

foreground에 대해 실행되고 있는 모든 프로세스를 종료하게 구현했습니다. 그리고 마지막에  newline을 출력하여 smallsh에 대한 입력 promptr가 출력되게 해습니다.

### 6. ‘|’ (pipe) 처리

```c
void procline(){
...
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
...
}
```

먼저 입력으로 주어진 cmd에 대해 pipe 연산자가 있는지 flag를 설정했습니다. 만약 pipe에 대한 flag가 설정되었다면 다음 함수를 실행시켰습니다.

```c
int runcommand(char** cline, int where,int argc,int out, char* output,int check_pipe) {
...

    if(check_pipe){
        return run_pipe(cline);
    }
...
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
```

입력 command에 대해 “|”를 경계로 Standard output으로 redirection할 첫번째 command와 Standard input으로 redirection할 두번째 command를 따로 나눴습니다. fork()를 통해 자식 프로세스는 stdout으로 redirection하고, 부모프로세스는 stdin으로 redirection하여 pipe를 구현했습니다.

### 7. Build & Run

```c
$gcc -o a.out smallsh.c main.c
$ ./a.out
```

### 8. Result

1. '&' 처리 (background process) 문제점 해결

    ```c
    /Users/changhyeon/Documents/linux_programming$ ./sleep_proc&
    [Process id] 15379
    /Users/changhyeon/Documents/linux_programming$ ps
      PID TTY           TIME CMD
    15332 ttys000    0:00.03 /bin/zsh --login -i
    15346 ttys000    0:00.01 ./a.out
    15379 ttys000    0:00.00 ./sleep_proc
    /Users/changhyeon/Documents/linux_programming$ wake up
    ps
      PID TTY           TIME CMD
    15332 ttys000    0:00.03 /bin/zsh --login -i
    15346 ttys000    0:00.01 ./a.out
    ```

    좀비프로세스가 생성되지 않은것을 확인할 수 있습니다.


2. 입력 prompt에 “Command>” 대신 “현재 디렉토리” 표시

```c
/Users/changhyeon/Documents/linux_programming$
```

1. SIGINT 처리 (Ctrl+C)
    1. Foreground process들 종료

        1) ;을 사용하지 않은 프로세스 실행

        ```c
        /Users/changhyeon/Documents/linux_programming$ sleep 10; sleep 5
        ^C
        ```

        2) ;을 사용했을 경우

        ```c
        /Users/changhyeon/Documents/linux_programming$ sleep 10
        ^C
        /Users/changhyeon/Documents/linux_programming$
        ```

    2. smallsh 입력 prompt에서의 ctrl+C

        ```c
        /Users/changhyeon/Documents/linux_programming$ ^C
        /Users/changhyeon/Documents/linux_programming$
        ```

2. `|` (pipe) 처리

    ```c
    /Users/changhyeon/Documents/linux_programming$ ls -l | grep smallsh
    -rw-r--r--@  1 changhyeon  staff   6123 Dec 18 21:00 smallsh.c
    -rw-r--r--@  1 changhyeon  staff    863 Dec 18 18:03 smallsh.h
    ```
