/* ========================================
   SUB Language - Native Compiler Driver
   Compiles SUB to native binary via C backend + gcc
   File: sub_native.c
   ======================================== */

#define _GNU_SOURCE
#include "sub_compiler.h"
#include "logo.h"
#include "windows_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

/* Helper: derive output name from input filename, or use user_out if provided */
static void derive_output_name(const char *input, const char *user_out,
                               char *out, size_t n) {
    if (user_out && *user_out) {
        strncpy(out, user_out, n - 1);
        out[n - 1] = '\0';
        return;
    }
    const char *base = input;
    for (const char *p = input; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    strncpy(out, base, n - 1);
    out[n - 1] = '\0';
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';
}

/* Print usage */
void print_usage_native(const char *prog_name) {
    printf(SUB_LOGO);
    printf("Usage: %s <input.sb> [options]\n\n", prog_name);
    printf("Output Options:\n");
    printf("  -o <file>          Output filename (default: derived from input)\n\n");
    printf("Optimization:\n");
    printf("  -O0                No optimization (fast compile)\n");
    printf("  -O1                Basic optimization\n");
    printf("  -O2                Standard optimization (default)\n");
    printf("  -O3                Aggressive optimization\n\n");
    printf("Debug:\n");
    printf("  -v, --verbose      Verbose output\n\n");
    printf("Examples:\n");
    printf("  %s hello.sb                  # Compile to ./hello\n", prog_name);
    printf("  %s hello.sb -O3              # Max optimization\n", prog_name);
    printf("  %s hello.sb -o myapp         # Custom output name\n\n", prog_name);
}

int compile_to_native(const char *input_file, const char *output_name,
                      bool verbose, int opt_level) {
    /* ---- Phase 1: Read source ---- */
    FILE *f = fopen(input_file, "rb");
    if (!f) { fprintf(stderr, "Cannot open: %s\n", input_file); return 1; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *source = malloc(sz + 1);
    if (!source) { fclose(f); return 1; }
    fread(source, 1, sz, f); source[sz] = '\0'; fclose(f);

    /* ---- Phase 2: Lex ---- */
    if (verbose) printf("[1/4] Lexing...\n");
    int ntok;
    Token *tokens = lexer_tokenize(source, &ntok);

    /* ---- Phase 3: Parse ---- */
    if (verbose) printf("[2/4] Parsing...\n");
    ASTNode *ast = parser_parse(tokens, ntok);

    /* ---- Phase 4: Semantic ---- */
    if (verbose) printf("[3/4] Semantic analysis...\n");
    if (!semantic_analyze(ast)) {
        fprintf(stderr, "Semantic analysis failed.\n");
        parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
        return 1;
    }

    /* ---- Phase 5: Generate C, write temp file ---- */
    if (verbose) printf("[4/4] Generating binary via gcc...\n");
    char *c_code = codegen_generate(ast, PLATFORM_LINUX);
    parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
    if (!c_code) { fprintf(stderr, "Code generation failed.\n"); return 1; }

    char tmp_c[512];
#ifdef _WIN32
    snprintf(tmp_c, sizeof(tmp_c), "sub_tmp_%d.c", (int)GetCurrentProcessId());
#else
    snprintf(tmp_c, sizeof(tmp_c), "/tmp/sub_tmp_%d.c", (int)getpid());
#endif
    FILE *cf = fopen(tmp_c, "w");
    if (!cf) { free(c_code); fprintf(stderr, "Cannot write temp file.\n"); return 1; }
    fputs(c_code, cf); fclose(cf); free(c_code);

    /* ---- Phase 6: Compile with gcc ---- */
    const char *opt = opt_level >= 2 ? "-O2" : opt_level == 1 ? "-O1" : "-O0";
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc %s -o \"%s\" \"%s\"", opt, output_name, tmp_c);
    int ret = system(cmd);
    remove(tmp_c);

    if (ret != 0) {
        fprintf(stderr, "Compilation failed. Make sure gcc is installed.\n");
        return 1;
    }
    printf("\u2705 Compiled: %s\n", output_name);
    return 0;
}

int main(int argc, char *argv[]) {
    printf(SUB_LOGO);

    if (argc < 2) {
        print_usage_native(argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *user_out = NULL;
    bool verbose = false;
    int opt_level = 2;
    
    // Parse command line options
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            user_out = argv[++i];
        } else if (strcmp(argv[i], "-O0") == 0) {
            opt_level = 0;
        } else if (strcmp(argv[i], "-O1") == 0) {
            opt_level = 1;
        } else if (strcmp(argv[i], "-O2") == 0) {
            opt_level = 2;
        } else if (strcmp(argv[i], "-O3") == 0) {
            opt_level = 3;
        }
    }

    char output_name[512];
    derive_output_name(input_file, user_out, output_name, sizeof(output_name));

    if (verbose) {
        printf("\ud83d\udcc4 Input:  %s\n", input_file);
        printf("\u2699\ufe0f  Mode:   Native via gcc (-O%d)\n", opt_level);
        printf("\ud83d\udce6 Output: %s\n\n", output_name);
    }
    
    return compile_to_native(input_file, output_name, verbose, opt_level);
}
