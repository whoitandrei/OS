#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

// Глобальные переменные
int global_initialized = 42;    // iv
int global_uninitialized;       // v
const int global_const = 100;   // vi

void A() {
    printf("A:\n");

    // i. Локальная переменная в функции
    int local_var;
    printf("[Локальная] Адрес: %p\n", (void*)&local_var);

    // ii. Статическая переменная в функции
    static int static_var;
    printf("[Статическая] Адрес: %p\n", (void*)&static_var);

    // iii. Константа в функции
    const int const_var = 50;
    printf("[Константа] Адрес: %p\n", (void*)&const_var);

    //вывод глобальных переменных
    printf("[Глобальная инициализированная] Адрес: %p\n", (void*)&global_initialized);
    printf("[Глобальная неинициализированная] Адрес: %p\n", (void*)&global_uninitialized);
    printf("[Глобальная константа] Адрес: %p\n\n", (void*)&global_const);
}

void B() {
    printf("B:\nPID: %d\n\n", getpid());
}

int* dangerous_func() {
    int local = 123;
    int* res = &local;
    return res; 
}

void D() {
    int* ptr = dangerous_func();
    printf("D:\n[Опасный указатель] Адрес: %p, Значение: %d\n\n", (void*)ptr, *ptr);
}

void E() {
    // i. Выделяем буфер и инициализируем
    char *buf1 = malloc(100);
    if (!buf1) {
        perror("malloc");
        return;
    }
    strncpy(buf1, "Hello world", 99);
    buf1[99] = '\0'; 

    // ii-iii. Вывод содержимого
    printf("E: (malloc)\nbuf1: %s (%p)\n", buf1, (void*)buf1);

    // iv. Освобождаем память
    free(buf1);

    // v. Вывод после free (UB, но подавим предупреждение для демо)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wuse-after-free"
    printf("После free(buf1): %s\n", buf1); 
    #pragma GCC diagnostic pop

    // vi. Выделяем новый буфер
    char *buf2 = malloc(100);
    if (!buf2) {
        perror("malloc");
        return;
    }
    strncpy(buf2, "Hello again", 99);
    buf2[99] = '\0';

    // vii-viii. Вывод
    printf("buf2: %s (%p)\n", buf2, (void*)buf2);

    // ix-x. Освобождение по смещённому указателю (ошибка)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfree-nonheap-object"
    char *mid = buf2 + 50;
    free(mid); 
    #pragma GCC diagnostic pop

    free(buf2);
}

void H() {
    // g. Установка переменной (в shell: export MY_VAR="test")
    const char* var_name = "MY_VAR";

    // h.i-ii. Чтение и изменение
    printf("H: \nИсходное значение: %s\n", getenv(var_name));
    setenv(var_name, "new_value", 1); // 1 = перезаписать
    printf("Новое значение: %s\n", getenv(var_name));

    // j. После завершения программы в shell:
    // echo $MY_VAR → вернёт исходное значение
}


int main() {
    A();
    B();
    D();
    //E();
    H();

    return 0;
}