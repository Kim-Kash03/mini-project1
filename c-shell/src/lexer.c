// PART A: Input Parsing - Lexer (A3) 
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token* new_token(TokenType type, const char* val) {
    Token* t = malloc(sizeof(Token));
    t->type = type;
    t->value = val ? strdup(val) : NULL;
    t->next = NULL;
    return t;
}

void free_tokens(Token *head) {
    while (head) {
        Token *tmp = head;
        head = head->next;
        if (tmp->value) free(tmp->value);
        free(tmp);
    }
}

static int is_space_char(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static int is_special(char c) {
    return (c == '|' || c == '&' || c == '>' || c == '<' || c == ';');
}

Token* tokenize(const char *input) {
    Token *head = NULL, *tail = NULL;
    int i = 0;
    while (input[i] != '\0') {
        if (is_space_char(input[i])) {
            i++;
            continue;
        }

        if (is_special(input[i])) {
            Token *t = NULL;
            if (input[i] == '>') {
                if (input[i+1] == '>') {
                    t = new_token(TOKEN_OP_GTGT, ">>");
                    i += 2;
                } else {
                    t = new_token(TOKEN_OP_GT, ">");
                    i++;
                }
            } else {
                char op[2] = {input[i], '\0'};
                TokenType type = TOKEN_ERROR;
                if (input[i] == '|') type = TOKEN_OP_PIPE;
                else if (input[i] == '&') type = TOKEN_OP_AMP;
                else if (input[i] == ';') type = TOKEN_OP_SEMI;
                else if (input[i] == '<') type = TOKEN_OP_LT;
                t = new_token(type, op);
                i++;
            }
            if (head == NULL) { head = t; tail = t; }
            else { tail->next = t; tail = t; }
            continue;
        }

        int buf_capacity = 64;
        char *buf = malloc(buf_capacity);
        int buf_len = 0;

        int in_word = 1;
        while (in_word && input[i] != '\0') {
            if (buf_len + 2 >= buf_capacity) {
                buf_capacity *= 2;
                buf = realloc(buf, buf_capacity);
            }

            if (input[i] == '\\') {
                i++;
                if (input[i] == '\0') {
                    free(buf);
                    free_tokens(head);
                    return new_token(TOKEN_ERROR, NULL);
                }
                buf[buf_len++] = input[i++];
            } else if (input[i] == '\'') {
                i++;
                int closed = 0;
                while (input[i] != '\0') {
                    if (input[i] == '\'') {
                        closed = 1;
                        i++;
                        break;
                    }
                    if (buf_len + 2 >= buf_capacity) {
                        buf_capacity *= 2;
                        buf = realloc(buf, buf_capacity);
                    }
                    buf[buf_len++] = input[i++];
                }
                if (!closed) {
                    free(buf);
                    free_tokens(head);
                    return new_token(TOKEN_ERROR, NULL);
                }
            } else if (input[i] == '"') {
                i++;
                int closed = 0;
                while (input[i] != '\0') {
                    if (input[i] == '"') {
                        closed = 1;
                        i++;
                        break;
                    }
                    if (buf_len + 3 >= buf_capacity) {
                        buf_capacity *= 2;
                        buf = realloc(buf, buf_capacity);
                    }
                    if (input[i] == '\\') {
                        i++;
                        if (input[i] == '\0') break; 
                        if (input[i] == '"' || input[i] == '\\') {
                            buf[buf_len++] = input[i++];
                        } else {
                            buf[buf_len++] = '\\';
                            buf[buf_len++] = input[i++];
                        }
                    } else {
                        buf[buf_len++] = input[i++];
                    }
                }
                if (!closed) {
                    free(buf);
                    free_tokens(head);
                    return new_token(TOKEN_ERROR, NULL);
                }
            } else if (is_space_char(input[i]) || is_special(input[i])) {
                in_word = 0;
            } else {
                buf[buf_len++] = input[i++];
            }
        }
        
        buf[buf_len] = '\0';
        Token *t = new_token(TOKEN_WORD, buf);
        free(buf);
        if (head == NULL) { head = t; tail = t; }
        else { tail->next = t; tail = t; }
    }
    
    return head;
}

void print_tokens(Token *head) {
    while (head) {
        printf("TYPE: %d, VAL: %s\n", head->type, head->value);
        head = head->next;
    }
}
