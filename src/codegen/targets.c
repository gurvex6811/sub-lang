#define _GNU_SOURCE
#include "targets.h"
#include "windows_compat.h"

#ifndef _WIN32
#include <strings.h>
#endif

/* run_command uses %s placeholders filled with the output basename at print time */
static LanguageInfo language_info_table[] = {
    [LANG_C]          = {"C",          ".c",     "gcc",        "gcc %s.c -o %s && ./%s"},
    [LANG_CPP]        = {"C++",        ".cpp",   "g++",        "g++ %s.cpp -o %s && ./%s"},
    [LANG_CPP17]      = {"C++17",      ".cpp",   "g++",        "g++ -std=c++17 %s.cpp -o %s && ./%s"},
    [LANG_CPP20]      = {"C++20",      ".cpp",   "g++",        "g++ -std=c++20 %s.cpp -o %s && ./%s"},
    [LANG_PYTHON]     = {"Python",     ".py",    "python3",    "python3 %s.py"},
    [LANG_JAVA]       = {"Java",       ".java",  "javac",      "javac SubProgram.java && java SubProgram"},
    [LANG_SWIFT]      = {"Swift",      ".swift", "swiftc",     "swiftc %s.swift -o %s && ./%s"},
    [LANG_KOTLIN]     = {"Kotlin",     ".kt",    "kotlinc",    "kotlinc %s.kt -include-runtime -d %s.jar && java -jar %s.jar"},
    [LANG_RUST]       = {"Rust",       ".rs",    "rustc",      "rustc %s.rs && ./%s"},
    [LANG_JAVASCRIPT] = {"JavaScript", ".js",    "node",       "node %s.js"},
    [LANG_TYPESCRIPT] = {"TypeScript", ".ts",    "tsc",        "tsc %s.ts && node %s.js"},
    [LANG_GO]         = {"Go",         ".go",    "go",         "go run %s.go"},
    [LANG_ASSEMBLY]   = {"Assembly",   ".asm",   "nasm",       "nasm -f elf64 %s.asm && ld %s.o -o %s && ./%s"},
    [LANG_CSS]        = {"CSS",        ".css",   "(browser)",  "# Open %s.css in a browser"},
    [LANG_RUBY]       = {"Ruby",       ".rb",    "ruby",       "ruby %s.rb"},
    [LANG_LLVM_IR]    = {"LLVM IR",    ".ll",    "llc",        "llc %s.ll && gcc %s.s -o %s && ./%s"},
    [LANG_WASM]       = {"WASM",       ".wasm",  "(browser)",  "# Load %s.wasm in WebAssembly"},
};

extern char* codegen_generate_c(ASTNode *ast, Platform platform);
extern char* codegen_rust(ASTNode *ast, const char *source);
extern char* codegen_python(ASTNode *ast, const char *source);
extern char* codegen_java(ASTNode *ast, const char *source);
extern char* codegen_swift(ASTNode *ast, const char *source);
extern char* codegen_kotlin(ASTNode *ast, const char *source);
extern char* codegen_javascript(ASTNode *ast, const char *source);
extern char* codegen_css(ASTNode *ast, const char *source);
extern char* codegen_ruby(ASTNode *ast, const char *source);
extern char* codegen_go(ASTNode *ast, const char *source);
extern char* codegen_assembly(ASTNode *ast, const char *source);

static char* codegen_typescript(ASTNode *ast, const char *source) {
    return codegen_javascript(ast, source);
}

static char* codegen_unimplemented(ASTNode *ast, const char *source) {
    (void)ast; (void)source; return NULL;
}

static TargetRegistry target_registry[] = {
    [LANG_C]          = {LANG_C,          "c",          NULL,                   true},
    [LANG_CPP]        = {LANG_CPP,        "c++",        codegen_unimplemented,  false},
    [LANG_CPP17]      = {LANG_CPP17,      "c++17",      codegen_unimplemented,  false},
    [LANG_CPP20]      = {LANG_CPP20,      "c++20",      codegen_unimplemented,  false},
    [LANG_PYTHON]     = {LANG_PYTHON,     "python",     codegen_python,         true},
    [LANG_JAVA]       = {LANG_JAVA,       "java",       codegen_java,           true},
    [LANG_SWIFT]      = {LANG_SWIFT,      "swift",      codegen_swift,          true},
    [LANG_KOTLIN]     = {LANG_KOTLIN,     "kotlin",     codegen_kotlin,         true},
    [LANG_RUST]       = {LANG_RUST,       "rust",       codegen_rust,           true},
    [LANG_JAVASCRIPT] = {LANG_JAVASCRIPT, "javascript", codegen_javascript,     true},
    [LANG_TYPESCRIPT] = {LANG_TYPESCRIPT, "typescript", codegen_typescript,     true},
    [LANG_GO]         = {LANG_GO,         "go",         codegen_go,             true},
    [LANG_ASSEMBLY]   = {LANG_ASSEMBLY,   "assembly",   codegen_assembly,       true},
    [LANG_CSS]        = {LANG_CSS,        "css",        codegen_css,            true},
    [LANG_RUBY]       = {LANG_RUBY,       "ruby",       codegen_ruby,           true},
    [LANG_LLVM_IR]    = {LANG_LLVM_IR,    "llvm",       codegen_unimplemented,  false},
    [LANG_WASM]       = {LANG_WASM,       "wasm",       codegen_unimplemented,  false},
};

LanguageInfo* language_info_get(TargetLanguage lang) {
    if (lang < 0 || lang >= LANG_COUNT) return NULL;
    return &language_info_table[lang];
}

CodegenFn get_codegen_for_target(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < LANG_COUNT; i++) {
        if (strcasecmp(target_registry[i].name, name) == 0)
            return target_registry[i].codegen;
    }
    return NULL;
}

bool target_is_implemented(TargetLanguage lang) {
    if (lang < 0 || lang >= LANG_COUNT) return false;
    return target_registry[lang].implemented;
}

const char* language_to_string(TargetLanguage lang) {
    if (lang < 0 || lang >= LANG_COUNT) return "unknown";
    return target_registry[lang].name;
}

TargetLanguage parse_language(const char *lang_str) {
    if (!lang_str) return LANG_C;

    struct { const char *alias; TargetLanguage lang; } aliases[] = {
        {"c",          LANG_C},
        {"cpp",        LANG_CPP},
        {"c++",        LANG_CPP},
        {"cpp17",      LANG_CPP17},
        {"c++17",      LANG_CPP17},
        {"cpp20",      LANG_CPP20},
        {"c++20",      LANG_CPP20},
        {"python",     LANG_PYTHON},
        {"py",         LANG_PYTHON},
        {"java",       LANG_JAVA},
        {"swift",      LANG_SWIFT},
        {"kotlin",     LANG_KOTLIN},
        {"kt",         LANG_KOTLIN},
        {"rust",       LANG_RUST},
        {"rs",         LANG_RUST},
        {"javascript", LANG_JAVASCRIPT},
        {"js",         LANG_JAVASCRIPT},
        {"typescript", LANG_TYPESCRIPT},
        {"ts",         LANG_TYPESCRIPT},
        {"go",         LANG_GO},
        {"golang",     LANG_GO},
        {"assembly",   LANG_ASSEMBLY},
        {"asm",        LANG_ASSEMBLY},
        {"css",        LANG_CSS},
        {"ruby",       LANG_RUBY},
        {"rb",         LANG_RUBY},
        {"llvm",       LANG_LLVM_IR},
        {"ll",         LANG_LLVM_IR},
        {"wasm",       LANG_WASM},
        {NULL, LANG_C}
    };

    for (int i = 0; aliases[i].alias; i++)
        if (strcasecmp(lang_str, aliases[i].alias) == 0)
            return aliases[i].lang;

    fprintf(stderr, "Warning: Unknown language '%s', defaulting to C\n", lang_str);
    return LANG_C;
}
