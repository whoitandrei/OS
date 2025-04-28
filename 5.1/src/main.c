#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#define ERR -1
#define IS_CHILD 0

int global_var = 1; 

void child(int* local_var) {
    printf("Child process:\n");
    pid_t pid = getpid();
    pid_t ppid = getppid();
    printf("  PID: %d, Parent PID: %d\n", pid, ppid);
    printf("  [Child] Address of global_var: %p, value: %d\n", (void*)&global_var, global_var);
    printf("  [Child] Address of local_var:  %p, value: %d\n", (void*)local_var, *local_var);

    getchar();

    global_var = 3;
    *local_var = 4;
    printf("  [Child] Modified: global_var: %p, value = %d\n\t\t    local_var: %p, value = %d\n",(void*)&global_var, global_var, (void*)local_var, *local_var);

    getchar();
    //sleep(100);
}

void parent(int* local_var) {
    printf("  [Parent after fork]: global_var: %p, value = %d\n\t\t    local_var: %p, value = %d\n",(void*)&global_var, global_var, (void*)local_var, *local_var);

    sleep(30);

    printf("  [Parent after fork and sleep]: global_var: %p, value = %d\n\t\t    local_var: %p, value = %d\n",(void*)&global_var, global_var, (void*)local_var, *local_var);

    int status;
    pid_t w = wait(&status);
    if (w == ERR) {
        perror("wait");
        exit(EXIT_FAILURE);
    }
    
    bool isNormallyExit = WIFEXITED(status); // проверяет, нормально ли завершен процесс
    if (isNormallyExit == true) {
        int exitStatus = WEXITSTATUS(status);
        printf("Child exited normally, exit code = %d\n", exitStatus);
        return;
    } 
    
    bool isSignalExit = WIFSIGNALED(status); // проверяет, завершен ли процесс сигналом
    if (isSignalExit == true) {
        int signalStatus = WTERMSIG(status);
        printf("Child killed by signal %d", signalStatus);
        return;
    }

    printf("Child terminated not normally\n");
}

int main(void) {
    int local_var = 2; 
    pid_t pid;

    printf("Before fork:\n");
    printf("  Address of global_var: %p, value: %d\n", (void*)&global_var, global_var);
    printf("  Address of local_var:  %p, value: %d\n", (void*)&local_var, local_var);

    pid = getpid();
    printf("Parent PID: %d\n", pid);

    pid = fork();
    if (pid == ERR) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == IS_CHILD) {
        child(&local_var);
        _exit(5); 
    } 

    parent(&local_var);
    return 0;
}
