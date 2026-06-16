/* =====================================================
   SUB Multilang Transpiler
   Usage: subc <input.sb> <lang> [output-name]
   Supported langs: python c js rust go java kotlin swift
   ===================================================== */

#define _GNU_SOURCE
#include "sub_compiler.h"
#include "codegen_multilang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Shared recursion-depth counter used by codegen functions */
int _gen_depth = 0;

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
 * Derive output filename from input path and target language.
 *   input: /some/path/hello.sb   lang: python  -> hello.py
 *   input: first.sb              lang: c       -> first.c
 *   input: first.sb              lang: js      -> first.js
 *
 * If `user_name` is given (non-NULL, non-empty), use it verbatim
 * (the caller may still lack an extension, so we append the correct one).
 */
static void build_output_path(const char *input_path,
                               const char *lang,
                               const char *user_name,
                               char *out, size_t outsz) {
    /* Strip directory from input path */
    const char *base = input_path;
    for (const char *p = input_path; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;

    /* Stem: remove extension */
    char stem[512];
    snprintf(stem, sizeof(stem), "%s", base);
    char *dot = strrchr(stem, '.');
    if (dot) *dot = '\0';

    /* Pick extension for the target language */
    const char *ext = ".txt";
    if (strcmp(lang, "python")  == 0 || strcmp(lang, "py")     == 0) ext = ".py";
    else if (strcmp(lang, "c")  == 0)                                 ext = ".c";
    else if (strcmp(lang, "cpp")== 0 || strcmp(lang, "c++")    == 0) ext = ".cpp";
    else if (strcmp(lang, "js") == 0 || strcmp(lang, "javascript")==0) ext = ".js";
    else if (strcmp(lang, "ts") == 0 || strcmp(lang, "typescript")==0) ext = ".ts";
    else if (strcmp(lang, "rust")== 0 || strcmp(lang, "rs")    == 0) ext = ".rs";
    else if (strcmp(lang, "go") == 0 || strcmp(lang, "golang") == 0) ext = ".go";
    else if (strcmp(lang, "java")== 0)                                ext = ".java";
    else if (strcmp(lang, "kotlin")==0 || strcmp(lang, "kt")   == 0) ext = ".kt";
    else if (strcmp(lang, "swift")== 0)                               ext = ".swift";
    else if (strcmp(lang, "lua") == 0)                                ext = ".lua";
    else if (strcmp(lang, "rb")  == 0 || strcmp(lang, "ruby")  == 0) ext = ".rb";

    /* Build output path */
    if (user_name && user_name[0]) {
        /* User supplied a name. If it already has the correct extension, keep it;
           otherwise append the language extension. */
        const char *has_ext = strrchr(user_name, '.');
        if (has_ext && strcmp(has_ext, ext) == 0)
            snprintf(out, outsz, "%s", user_name);
        else
            snprintf(out, outsz, "%s%s", user_name, ext);
    } else {
        snprintf(out, outsz, "%s%s", stem, ext);
    }
}

/* ---- entry point ---- */

int transpile_file(const char *input_file,
                   const char *target_lang,
                   const char *user_output_name) {
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
        fprintf(stderr, "subc: semantic error in '%s'\n", input_file);
        parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
        return 1;
    }

    /* 5. Reset depth counter and generate target-language code */
    _gen_depth = 0;
    char *output_code = codegen_generate(ast, target_lang);
    if (!output_code) {
        fprintf(stderr, "subc: code generation failed for language '%s'\n", target_lang);
        parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
        return 1;
    }

    /* 6. Determine output filename */
    char out_path[512];
    build_output_path(input_file, target_lang, user_output_name, out_path, sizeof(out_path));

    /* 7. Write */
    if (!write_file(out_path, output_code)) {
        free(output_code);
        parser_free_ast(ast); lexer_free_tokens(tokens, ntok); free(source);
        return 1;
    }
    free(output_code);

    printf("subc: transpiled '%s'  ->  %s\n", input_file, out_path);

    parser_free_ast(ast);
    lexer_free_tokens(tokens, ntok);
    free(source);
    return 0;
}
