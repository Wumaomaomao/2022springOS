#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>

int pipefd[2];
char buf[1222];

int main()
{
    int dup_stdin = dup(STDERR_FILENO);
    int dup_stdout = dup(STDOUT_FILENO);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    int ret = pipe(pipefd);
    
    if (ret == 0)
    {
        ret = fork();
        if (ret > 0)
        {
            
            close(pipefd[1]);//only read
            read(pipefd[0], buf, sizeof(buf) );
            close(pipefd[0]);
            dup2(dup_stdin,STDIN_FILENO);
            dup2(dup_stdout,STDOUT_FILENO);
            printf("This is father process!\n");
            printf("bufout:\n%s",buf);
            
        }
        else if (ret == 0)
        {
            close(pipefd[0]);//only write
            system("ls -l");
            close(pipefd[1]);
            dup2(dup_stdin,STDIN_FILENO);
            dup2(dup_stdout,STDOUT_FILENO);
            printf("This is Child process!\n");
            
        }
    }
    else
    {
        printf("Create pipe Error!\n");
        exit(-1);
    }

    return 0;
}