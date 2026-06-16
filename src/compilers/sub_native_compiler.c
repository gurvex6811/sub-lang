/* ===================================================
   SUB Native Compiler — C-backend path
   Compiles .sb source → C → machine binary via gcc/cc
   =================================================== */

#define _GNU_SOURCE
#include "sub_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  define PATH_SEP '\\'
#  define NULL_DEV "NUL"
#else
#  define PATH_SEP '/'
#  define NULL_DEV "/dev/null"
#endif

/* ---- helpers ---- */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "subc: cannot open '%s'\n", path); return NULL; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f); buf[sz] = '\0'; fclose(f);
    return buf;
}

static int write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) { fprintf(stderr, "subc: cannot write '%s'\n", path); return 0; }
    fputs(content, f);
    fclose(f);
    return 1;
}

/**
 * Derive output binary name from input path:
 *   /some/path/hello.sb  ->  hello   (same dir as caller)
 *   hello.sb             ->  hello
 */
static void derive_output_name(const char *input_path, char *out, size_t outsz) {
    /* find last separator */
    const char *base = input_path;
    for (const char *p = input_path; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;

    /* strip .sb extension */
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", base);
    char *dot = strrchr(tmp, '.');
    if (dot) *dot = '\0';

    snprintf(out, outsz, "%s", tmp);
}

/* ---- public entry point ---- */

int compile_native(const char *input_file, const char *output_file_arg) {
    /* 1. Read source */
    char *source = read_file(input_file);
    if (!source) return 1;

    /* 2. Lex */
    int ntok;
    Token *tokens = lexer_tokenize(source, &ntok);
    if (!tokens) { free(source); return 1; }

    /* 3. Parse */
    ASTNode *ast = parser_parse(tokens, ntok);
    if (!ast) {
        lexer_free_tokens(tokens, ntok); free(source); return 1;
    }

    /* 4. Semantic analysis */
    if (!semantic_analyze(ast)) {
        fprintf(stderr, "subc: semantic error\n");
        parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
        return 1;
    }

    /* 5. Generate C code from AST */
    char *c_code = codegen_generate_c(ast, PLATFORM_LINUX);
    if (!c_code) {
        fprintf(stderr, "subc: C codegen failed\n");
        parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
        return 1;
    }

    /* 6. Determine output binary name */
    char bin_name[512];
    if (output_file_arg && output_file_arg[0]) {
        snprintf(bin_name, sizeof(bin_name), "%s", output_file_arg);
    } else {
        derive_output_name(input_file, bin_name, sizeof(bin_name));
    }

    /* 7. Write C file to /tmp */
    char c_path[512];
    snprintf(c_path, sizeof(c_path), "/tmp/sub_%s.c", bin_name);
    if (!write_file(c_path, c_code)) {
        free(c_code);
        parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
        return 1;
    }
    free(c_code);

    /* 8. Invoke gcc/cc to produce a real native binary */
    char cmd[1024];
    /* Try gcc first, fall back to cc */
    snprintf(cmd, sizeof(cmd),
        "gcc -O2 -Wall -Wno-unused-variable -o %s %s 2>&1 || "
        "cc  -O2 -Wall -Wno-unused-variable -o %s %s 2>&1",
        bin_name, c_path, bin_name, c_path);

    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr,
            "subc: C compilation failed (exit %d).\n"
            "  Intermediate C file: %s\n"
            "  Run: gcc -o %s %s\n",
            rc, c_path, bin_name, c_path);
        parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
        return 1;
    }

    /* 9. Clean up temp file */
    char rm_cmd[600];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -f %s", c_path);
    system(rm_cmd);

    parser_free_ast(ast);
    lexer_free_tokens(tokens, ntok);
    free(source);

    printf("subc: compiled '%s'  ->  ./%s\n", input_file, bin_name);
    return 0;
}
