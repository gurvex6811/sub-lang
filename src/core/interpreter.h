#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "sub_compiler.h"

typedef enum { VAL_INT, VAL_FLOAT, VAL_STRING, VAL_BOOL, VAL_NULL, VAL_FUNC } ValType;

typedef struct SubVal {
    ValType type;
    union {
        long long  iv;
        double     fv;
        char      *sv;
        int        bv;
        ASTNode   *fn; /* function AST node */
    };
} SubVal;

typedef struct EnvEntry {
    char *name;
    SubVal val;
    struct EnvEntry *next;
} EnvEntry;

typedef struct Env {
    EnvEntry    *vars;
    struct Env  *parent;
    int          returning;
    SubVal       ret_val;
} Env;

Env   *env_new(Env *parent);
void   env_free(Env *env);
SubVal env_get(Env *env, const char *name);
void   env_set(Env *env, const char *name, SubVal val);
void   env_define(Env *env, const char *name, SubVal val);

SubVal eval(ASTNode *node, Env *env);
int    interpret_file(const char *path);
#endif
