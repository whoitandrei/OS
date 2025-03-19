#include <stdio.h>
#include "hello_from_static_lib.h"

int main() {
    printf("hello world [from main]\n");
    hello_from_static_lib();
    return 0;
}