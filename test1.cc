#include <stdio.h>
#include <unistd.h>
int main() {
    char str[12] = "Hello world";
    int ret = write(1, str, sizeof(str));
    while(1){};
}