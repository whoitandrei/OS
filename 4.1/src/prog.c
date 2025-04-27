#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

int global_initialized = 42;
int global_uninitialized;
const int global_const = 100;

void A() {
    printf("A:\n");

    int local_var;
    printf("[Local (uninitialized)] Address: %p\n", (void*)&local_var);
    
    static int static_var;
    printf("[Static (uninitialized)] Address: %p\n", (void*)&static_var);
    
    const int const_var = 50;
    printf("[Constant] Address: %p\n", (void*)&const_var);
    
    printf("[Global initialized] Address: %p\n", (void*)&global_initialized);
    printf("[Global uninitialized] Address: %p\n", (void*)&global_uninitialized);
    printf("[Global constant] Address: %p\n\n", (void*)&global_const);
}

void B() {
    int pid = getpid();
    printf("B:\nPID: %d\n", pid);

    printf("/proc/%d/maps", pid);

    getchar();
}

int* return_local_variable() {
    int local = 123;
    int* res = &local;
    return res; 
}

void D() {
    int* ptr = return_local_variable();
    printf("\nD:\n[Local variable] Address: %p, Value: %d\n\n", (void*)ptr, *ptr);
}

void E() {
    char *buf1 = malloc(100);
    if (buf1 == NULL) {
        perror("malloc");
        return;
    }
    
    strncpy(buf1, "Hello world", 99);
    buf1[99] = '\0'; 
    
    printf("E: (malloc)\nbuf1: %s (%p)\n", buf1, (void*)buf1);

    free(buf1);
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wuse-after-free"
    printf("After free(buf1): %s\n", buf1); 
    #pragma GCC diagnostic pop
    
    char *buf2 = malloc(100);
    if (buf2 == NULL) {
        perror("malloc");
        return;
    }
    
    strncpy(buf2, "Hello again", 99);
    buf2[99] = '\0';
    
    printf("buf2: %s (%p)\n", buf2, (void*)buf2);
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfree-nonheap-object"
    char *mid = buf2 + 50;
    buf2 = mid;
    free(mid); 
    #pragma GCC diagnostic pop
}

void H() {
    const char* var_name = "MY_VAR";
    printf("H:\nName: %s\n", var_name);

    char* var_value = getenv(var_name);
    if (var_value == NULL) {
        // обрабатываю это как ошибку, так как по заданию нужно увидеть различие до и после для уже созданной переменной
        fprintf(stderr, "no such env var: %s\n", var_name);
        return;
    }

    printf("Original value: %s\n", var_value);

    int returned_setenv = setenv(var_name, "new_value", 1);
    if (returned_setenv == -1) {
        perror("setenv");
        return;
    }

    var_value = getenv(var_name);
    if (var_value == NULL) {
        fprintf(stderr, "no such env var");
        return;
    }
    printf("New value: %s\n\n", var_value);
}

int main() {
    A();
    B();
    D();
    H();
    E();
    return 0;
}
