// PART A: Input Parsing - Grammar (A3) 
#include "parser.h"
#include <stdio.h>

static int parse_LINE(Token **t);
static int parse_ARG(Token **t);
static int parse_CMD(Token **t);
static int parse_TGT(Token **t);
static int parse_BG(Token **t);

static int parse_LINE(Token **t) {
    if (*t == NULL) return 1;
    if ((*t)->type == TOKEN_WORD) {
        *t = (*t)->next;
        return parse_ARG(t);
    }
    return 0;
}

static int parse_ARG(Token **t) {
    if (*t == NULL) return 1;
    TokenType type = (*t)->type;
    if (type == TOKEN_WORD) {
        *t = (*t)->next;
        return parse_ARG(t);
    } else if (type == TOKEN_OP_LT || type == TOKEN_OP_GT || type == TOKEN_OP_GTGT) {
        *t = (*t)->next;
        return parse_TGT(t);
    } else if (type == TOKEN_OP_PIPE || type == TOKEN_OP_SEMI) {
        *t = (*t)->next;
        return parse_CMD(t);
    } else if (type == TOKEN_OP_AMP) {
        *t = (*t)->next;
        return parse_BG(t);
    }
    return 0;
}

static int parse_CMD(Token **t) {
    if (*t == NULL) return 0;
    if ((*t)->type == TOKEN_WORD) {
        *t = (*t)->next;
        return parse_ARG(t);
    }
    return 0;
}

static int parse_TGT(Token **t) {
    if (*t == NULL) return 0;
    if ((*t)->type == TOKEN_WORD) {
        *t = (*t)->next;
        return parse_ARG(t);
    }
    return 0;
}

static int parse_BG(Token **t) {
    if (*t == NULL) return 1;
    if ((*t)->type == TOKEN_WORD) {
        *t = (*t)->next;
        return parse_ARG(t);
    }
    return 0;
}

int validate_syntax(Token *head) {
    if (head && head->type == TOKEN_ERROR) {
        printf("cshell: invalid syntax\n");
        return 0;
    }
    Token *current = head;
    if (parse_LINE(&current) && current == NULL) {
        return 1;
    }
    printf("cshell: invalid syntax\n");
    return 0;
}
