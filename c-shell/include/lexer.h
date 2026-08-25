#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_OP_PIPE,
    TOKEN_OP_AMP,
    TOKEN_OP_SEMI,
    TOKEN_OP_LT,
    TOKEN_OP_GT,
    TOKEN_OP_GTGT,
    TOKEN_WORD,
    TOKEN_ERROR
} TokenType;

typedef struct Token {
    TokenType type;
    char *value;
    struct Token *next;
} Token;

Token *tokenize(const char *input);
void free_tokens(Token *head);
void print_tokens(Token *head);

#endif
