#include <sys/syscall.h>
#include <unistd.h>

int main() {
    syscall(SYS_write, 1, "hello world\n", 12);
    return 0;
}