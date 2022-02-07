# Term Project 1: Making Own Shell (smallsh)

### 1. 개발 환경

- 운영체제 : Mac OS X에서 개발하였고, Ubuntu에서도 검증하였습니다.
- 언어 : C
- IDE : Clion
- Compiler : gcc

### 2. Term Project 1: Making Own Shell (smallsh) 설명

주어진 shell program code에서, (1) cd (2) exit (3) > (output redirection) 기능을 추가 구현하고, 주어진 & (background) 동작에 대해 생길 수 있는 문제점을 분석하는 프로젝트 였습니다.

### 3. cd code 설명

먼저 cd에 대해 설명하겠습니다. cd는 ls,kill과 같은 시스템콜의 동작과 다릅니다. ls ,kill과 같은 시스템콜은 `fork()`를 통해 자식 프로세스를 실행시킨뒤, execvp()를 통해 실행됩니다. 즉, 자식프로세스에서 ls, kill 프로그램이 실행됩니다. 그에 반해 cd는 change directory를 의미하는 built in command로써, 현재 shell을 실행시키는 process의 current directory를 바꾸는데 사용해야합니다. 만약 자식을 fork하여 cd를 실행하면 부모의 directory가 바뀌는 것이 아닌 자식의 directory가 바뀝니다.

먼저 procline 함수에 argument가 cd이면 cmd_cd를 호출하였습니다.

```c
if(strcmp(arg[0],"cd")==0)
{
    cmd_cd(narg,arg);
}
```

`int cmd_cd(int argc, char* argv[])` : argc는 argument의 개수, argv는 argument가 들어가 있는 문자열 배열입니다.

```c
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
```

위와 같이 cmd_cd를 구현했습니다. chdir systemcall을 이용하여, argument[1]로 directory를 바꿔 주었습니다. 만약 정상적으로 바꾼다면 0을 반환하여 cd command가 종료됩니다.

### 4. exit code 설명

exit 기능 추가는 arg[0]가 exit일 때, exit (c library 함수)를 호출하게 하여 구현했습니다. 다음은 procline에 추가한 코드입니다.

```c
else if(strcmp(arg[0],"exit")==0)
  {
      exit(0);
  }
```

exit 함수에 argument로 들어간 0은 보통 성공적으로 종료했을때 0을 대입합니다.

### 5. > (output redirection) code 설명

먼저 >가 command에 들어가 있는지 여부와 출력을 저장할 파일에 대해 인자를 별도로 저장해야합니다.

```c
int out = 0;
char* output=NULL;

for(int i=0;i<narg;i++){
    if(strcmp(arg[i],">")==0) {
        out = 1;
        output = arg[i+1];
        arg[i]=NULL;
    }
}
```

procline 함수에 다음 코드를 추가하여, output file과 output '>' 의 사용여부를 저장하였습니다.

arg[i] = NULL; 을 대입한 이유는 예를 들어 `ls -l > file.txt` 라 사용했을때, ls -l 까지에 대한 command를 별도로 읽어야 하기 때문입니다.

runcommand에 추가한 코드를 설명하기 앞서, runcommand의 함수 원형을 변형했습니다.

`int runcommand(char **cline, int where,int argc,int out, char* output);` '>'의 사용여부를 뜻하는 out, output을 저장할 file char* output을 추가하였습니다.

runcommand 함수에 다음 코드를 추가하였습니다.

```c
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
```

먼저 주어진 file이름으로 0644권한을 주어 파일을 만들고서, 그에대한 file descriptor를 할당하였습니다.  `dup2(fd1, STDOUT_FILENO);` 를 이용하여 해당 child process가 만드는 출력을 output file에 저장되게 redirection을 하였습니다.

STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO은 0,1,2에 값에 각각 대응되는 file descriptor입니다. (unistd.h에 매크로 상수로 정의됩니다.)

### 6.  & (background) 동작에 대해 생길 수 있는 문제점

background code가 구현된 runcommand 함수는 다음과 같습니다.

```c
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
```

where에 대해 전달된 인자가 BACKGROUND라면, pid값을 출력한뒤 종료합니다. 이렇게 된다면 waitpid를 통해 부모프로세스에서 자식프로세스의 종료를 확인하지 않고, 부모프로세스가 종료됩니다. 즉 자식 프로세스는 의도적으로 만든 고아 프로세스가 됩니다.

하지만 background에서 돌아가는 자식 프로세스가 부모프로세스보다  먼저 끝나면 문제가 생깁니다. 즉, 좀비 프로세스가 됩니다. 위의 코드에서는 background로 돌아가는 자식프로세스가 종료되도, 그에 대한 메모리,리소스가 해제되지 않습니다.

### 7. Build & run

```c
gcc -o smallsh main.c smallsh.c
./smallsh
```

### 8. result

1. ls -l

    ```c
    Command> ls -l
    total 96
    drwxr-xr-x@  7 changhyeon  staff    224 Nov 18 16:33 12171483_EX8
    -rw-r--r--   1 changhyeon  staff   2122 Nov 19 17:59 12171483_PRJ1.zip
    -rw-r--r--   1 changhyeon  staff    154 Nov 19 11:33 CMakeLists.txt
    drwxr-xr-x@ 13 changhyeon  staff    416 Nov 19 12:49 cmake-build-debug
    -rw-r--r--   1 changhyeon  staff    640 Nov 19 17:23 file
    -rw-r--r--   1 changhyeon  staff    582 Nov 19 17:21 file.txt
    drwxr-xr-x@  2 changhyeon  staff     64 Nov 19 12:19 linux
    -rw-r--r--   1 changhyeon  staff    123 Nov 19 11:33 main.c
    -rwxr-xr-x   1 changhyeon  staff  50544 Nov 19 17:19 smallsh
    -rw-r--r--   1 changhyeon  staff   3658 Nov 19 13:19 smallsh.c
    -rw-r--r--   1 changhyeon  staff    687 Nov 19 17:22 smallsh.h
    ```

2. cd

    ```c
    Command> cd linux
    Command> pwd
    /Users/changhyeon/Documents/linux_programming/linux
    Command> cd ..
    Command> pwd
    /Users/changhyeon/Documents/linux_programming
    ```

3. `>` (redirection)

    ```c
    Command> ls -l > file.txt
    Command> vim file.txt
    total 80
    drwxr-xr-x@  7 changhyeon  staff    224 Nov 18 16:33 12171483_EX8
    -rw-r--r--   1 changhyeon  staff   2122 Nov 19 17:59 12171483_PRJ1.zip
    -rw-r--r--   1 changhyeon  staff    154 Nov 19 11:33 CMakeLists.txt
    drwxr-xr-x@ 13 changhyeon  staff    416 Nov 19 12:49 cmake-build-debug
    -rw-r--r--   1 changhyeon  staff      0 Nov 19 18:03 file.txt
    drwxr-xr-x@  3 changhyeon  staff     96 Nov 19 18:03 linux
    -rw-r--r--   1 changhyeon  staff    123 Nov 19 11:33 main.c
    -rwxr-xr-x   1 changhyeon  staff  50544 Nov 19 17:19 smallsh
    -rw-r--r--   1 changhyeon  staff   3658 Nov 19 13:19 smallsh.c
    -rw-r--r--   1 changhyeon  staff    687 Nov 19 17:22 smallsh.h
    ~                                                                                                                                                                                                         
    ~                                                                                                                                                                                                         
    "file.txt" 11L, 653
    ```

4. `;, >, &` 동시 동작

    ```c
    Command> ls -l&; ls
    [Process id] 5297
    12171483_EX8            CMakeLists.txt          file.txt                main.c                  smallsh.c
    12171483_PRJ1.zip       cmake-build-debug       linux                   smallsh                 smallsh.h
    Command> total 88
    drwxr-xr-x@  7 changhyeon  staff    224 Nov 18 16:33 12171483_EX8
    -rw-r--r--   1 changhyeon  staff   2122 Nov 19 17:59 12171483_PRJ1.zip
    -rw-r--r--   1 changhyeon  staff    154 Nov 19 11:33 CMakeLists.txt
    drwxr-xr-x@ 13 changhyeon  staff    416 Nov 19 12:49 cmake-build-debug
    -rw-r--r--   1 changhyeon  staff    653 Nov 19 18:03 file.txt
    drwxr-xr-x@  3 changhyeon  staff     96 Nov 19 18:03 linux
    -rw-r--r--   1 changhyeon  staff    123 Nov 19 11:33 main.c
    -rwxr-xr-x   1 changhyeon  staff  50544 Nov 19 17:19 smallsh
    -rw-r--r--   1 changhyeon  staff   3658 Nov 19 13:19 smallsh.c
    -rw-r--r--   1 changhyeon  staff    687 Nov 19 17:22 smallsh.h
    ```

5. `exit`

    ```c
    Command> exit
    changhyeon@changhyeonnamui-MacBookPro-5 linux_programming %
    ```


### 9. reference

- [https://stackoverflow.com/questions/27430642/student-shell-cd-not-working](https://stackoverflow.com/questions/27430642/student-shell-cd-not-working) (cd 원리)
