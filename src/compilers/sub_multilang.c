/* ========================================
   SUB Language Multi-Language Compiler Driver
   Compile SUB code to: Python, Java, Swift, Kotlin, C, C++, Rust, JS, Assembly, CSS
   File: sub_multilang.c
   ======================================== */

#define _GNU_SOURCE
#include "sub_compiler.h"
#include "logo.h"
#include "windows_compat.h"
#include "targets.h"

/* Depth guard for recursive codegen functions */
static int _gen_depth = 0;

/* External codegen functions (from codegen_multilang.c) */
extern char* codegen_python(ASTNode *ast, const char *source);
extern char* codegen_java(ASTNode *ast, const char *source);
extern char* codegen_swift(ASTNode *ast, const char *source);
extern char* codegen_kotlin(ASTNode *ast, const char *source);
extern char* codegen_cpp(ASTNode *ast, const char *source);
extern char* codegen_rust(ASTNode *ast, const char *source);
extern char* codegen_javascript(ASTNode *ast, const char *source);
extern char* codegen_css(ASTNode *ast, const char *source);
extern char* codegen_ruby(ASTNode *ast, const char *source);
extern char* codegen_go(ASTNode *ast, const char *source);
extern char* codegen_assembly(ASTNode *ast, const char *source);

/* From codegen.c */
extern char* codegen_generate_c(ASTNode *ast, Platform platform);

/* Helper: strip directory and .sb/.sub extension */
static void get_output_basename(const char *input_file, char *out, size_t n) {
    const char *base = input_file;
    for (const char *p = input_file; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    strncpy(out, base, n - 1);
    out[n - 1] = '\0';
    char *dot = strrchr(out, '.');
    if (dot && (strcmp(dot, ".sb") == 0 || strcmp(dot, ".sub") == 0))
        *dot = '\0';
}

/* Utility: Read file */
char* read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    if (fread(content, 1, size, file) != (size_t)size) {
        /* short read — tolerate text mode diffs */
    }
    content[size] = '\0';
    fclose(file);
    return content;
}

/* Utility: Write file */
void write_file(const char *filename, const char *content) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot write to file %s\n", filename);
        return;
    }
    fprintf(file, "%s", content);
    fclose(file);
}

/* Generate code for target language */
char* generate_code(ASTNode *ast, TargetLanguage lang, const char *source) {
    _gen_depth = 0; /* reset depth guard before each top-level codegen call */
    if (lang == LANG_C) {
        return codegen_generate_c(ast, PLATFORM_LINUX);
    }
    
    if (!target_is_implemented(lang)) {
        const char *lang_name = language_to_string(lang);
        fprintf(stderr, "%s codegen not yet implemented\n", lang_name);
        return NULL;
    }
    
    CodegenFn codegen = get_codegen_for_target(language_to_string(lang));
    if (!codegen) {
        fprintf(stderr, "Unknown target language\n");
        return NULL;
    }
    
    return codegen(ast, source);
}

/* Print usage */
void print_usage(const char *prog_name) {
    printf(SUB_LOGO);
    printf("Usage: %s <input.sb> [target_language]\n\n", prog_name);
    printf("Supported Target Languages:\n");
    printf("  c, cpp/c++     - C and C++\n");
    printf("  cpp17, cpp20   - C++17, C++20\n");
    printf("  python/py      - Python 3\n");
    printf("  java           - Java\n");
    printf("  swift          - Swift\n");
    printf("  kotlin/kt      - Kotlin\n");
    printf("  rust/rs        - Rust\n");
    printf("  javascript/js  - JavaScript\n");
    printf("  typescript/ts  - TypeScript\n");
    printf("  go/golang      - Go\n");
    printf("  assembly/asm   - x86-64 Assembly\n");
    printf("  css            - CSS Stylesheet\n");
    printf("  ruby/rb        - Ruby\n");
    printf("\nExamples:\n");
    printf("  %s hello.sb python      # Transpile to hello.py\n", prog_name);
    printf("  %s hello.sb cpp17       # Transpile to hello.cpp\n", prog_name);
    printf("  %s hello.sb java        # Transpile to SubProgram.java\n", prog_name);
    printf("  %s hello.sb rust        # Transpile to hello.rs\n", prog_name);
    printf("  %s hello.sb c           # Transpile to hello.c (default)\n\n", prog_name);
}

/* Main function */
int main(int argc, char *argv[]) {
    printf(SUB_LOGO);

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *target_lang_str = argc > 2 ? argv[2] : "c";
    TargetLanguage target_lang = parse_language(target_lang_str);
    LanguageInfo *info = language_info_get(target_lang);
    
    /* Derive output basename from input filename */
    char base_name[256];
    get_output_basename(input_file, base_name, sizeof(base_name));

    /* Determine output filename */
    char output_file[256];
    if (target_lang == LANG_JAVA) {
        snprintf(output_file, sizeof(output_file), "SubProgram%s", info->extension);
    } else {
        snprintf(output_file, sizeof(output_file), "%s%s", base_name, info->extension);
    }

    printf("\u256c\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u256a\n");
    printf("\u2551  SUB Language Compiler v2.0            \u2551\n");
    printf("\u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d\n\n");
    
    printf("\ud83d\udcc4 Input:  %s\n", input_file);
    printf("\ud83c\udfaf Target: %s\n", info->name);
    printf("\ud83d\udce6 Output: %s\n\n", output_file);
    
    // Phase 1: Read source
    printf("[1/5] \ud83d\udcd6 Reading source file...\n");
    char *source = read_file(input_file);
    if (!source) return 1;
    printf("      \u2713 Read %zu bytes\n", strlen(source));
    
    // Phase 2: Lexical Analysis
    printf("[2/5] \ud83d\udd24 Lexical analysis...\n");
    int token_count;
    Token *tokens = lexer_tokenize(source, &token_count);
    printf("      \u2713 Generated %d tokens\n", token_count);
    
    // Phase 3: Parsing
    printf("[3/5] \ud83c\udf33 Parsing...\n");
    ASTNode *ast = parser_parse(tokens, token_count);
    printf("      \u2713 AST created\n");
    
    // Phase 4: Semantic Analysis
    printf("[4/5] \ud83d\udd0d Semantic analysis...\n");
    if (!semantic_analyze(ast)) {
        fprintf(stderr, "      \u2717 Semantic analysis failed\n");
        return 1;
    }
    printf("      \u2713 Passed\n");
    
    // Phase 5: Code Generation
    printf("[5/5] \u2699\ufe0f  Code generation (%s)...\n", info->name);
    char *output_code = generate_code(ast, target_lang, source);
    
    if (!output_code) {
        fprintf(stderr, "      \u2717 Code generation failed\n");
        return 1;
    }
    
    write_file(output_file, output_code);
    printf("      \u2713 Generated %zu bytes\n", strlen(output_code));
    
    // Success!
    printf("\n\u2705 Compilation successful!\n");
    printf("\ud83d\udcdd Output: %s\n\n", output_file);
    
    // Print next steps
    printf("Next steps:\n");
    printf("  %s\n\n", info->run_command);
    
    // Cleanup
    free(source);
    lexer_free_tokens(tokens, token_count);
    parser_free_ast(ast);
    free(output_code);
    
    return 0;
}
