// PART C: Command Piping & I/O Multiplexing (C2-C4) 
#ifndef PIPELINE_H
#define PIPELINE_H

#include "lexer.h"

typedef struct {
    char *filename;
    int append;
} OutRedir;

typedef struct {
    char **argv;
    int argc;
    int argv_cap;
    char **in_files;
    int in_count;
    int in_cap;
    OutRedir *out_files;
    int out_count;
    int out_cap;
} CommandData;

typedef struct {
    CommandData *cmds;
    int cmd_count;
    int cmd_cap;
} Pipeline;

Pipeline* parse_pipeline(Token *head);
void free_pipeline(Pipeline *p);

#endif
