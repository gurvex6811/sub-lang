#define _GNU_SOURCE
#include "sub_compiler.h"
#include "interpreter.h"
#include "logo.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf(SUB_LOGO);
    printf("SUB Interpreter v1.0\n");
    printf("====================\n\n");
    if (argc < 2) {
        printf("Usage: %s <file.sb>\n", argv[0]);
        printf("Example: %s hello.sb\n", argv[0]);
        return 1;
    }
    return interpret_file(argv[1]);
}
