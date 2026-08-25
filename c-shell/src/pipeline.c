// PART C: Command Piping & I/O Multiplexing (C2-C4) 
#include "pipeline.h"
#include <stdlib.h>
#include <string.h>

static void init_command(CommandData *cmd) {
    cmd->argc = 0; cmd->argv_cap = 16;
    cmd->argv = malloc(cmd->argv_cap * sizeof(char*));
    cmd->in_count = 0; cmd->in_cap = 4;
    cmd->in_files = malloc(cmd->in_cap * sizeof(char*));
    cmd->out_count = 0; cmd->out_cap = 4;
    cmd->out_files = malloc(cmd->out_cap * sizeof(OutRedir));
}

Pipeline* parse_pipeline(Token *head) {
    if (!head) return NULL;

    Pipeline *p = malloc(sizeof(Pipeline));
    p->cmd_count = 0; p->cmd_cap = 4;
    p->cmds = malloc(p->cmd_cap * sizeof(CommandData));
    
    init_command(&p->cmds[0]);
    int curr_cmd = 0;

    Token *curr = head;
    while (curr) {
        if (curr->type == TOKEN_OP_SEMI || curr->type == TOKEN_OP_AMP) {
            break;
        }

        if (curr->type == TOKEN_OP_PIPE) {
            curr_cmd++;
            if (curr_cmd >= p->cmd_cap) {
                p->cmd_cap *= 2;
                p->cmds = realloc(p->cmds, p->cmd_cap * sizeof(CommandData));
            }
            init_command(&p->cmds[curr_cmd]);
            curr = curr->next;
            continue;
        }

        if (curr->type == TOKEN_OP_LT) { // < file
            curr = curr->next;
            if (curr && curr->type == TOKEN_WORD) {
                CommandData *cmd = &p->cmds[curr_cmd];
                if (cmd->in_count >= cmd->in_cap) {
                    cmd->in_cap *= 2;
                    cmd->in_files = realloc(cmd->in_files, cmd->in_cap * sizeof(char*));
                }
                cmd->in_files[cmd->in_count++] = curr->value;
            }
        } else if (curr->type == TOKEN_OP_GT || curr->type == TOKEN_OP_GTGT) {
            int is_append = (curr->type == TOKEN_OP_GTGT);
            curr = curr->next;
            if (curr && curr->type == TOKEN_WORD) {
                CommandData *cmd = &p->cmds[curr_cmd];
                if (cmd->out_count >= cmd->out_cap) {
                    cmd->out_cap *= 2;
                    cmd->out_files = realloc(cmd->out_files, cmd->out_cap * sizeof(OutRedir));
                }
                cmd->out_files[cmd->out_count].filename = curr->value;
                cmd->out_files[cmd->out_count].append = is_append;
                cmd->out_count++;
            }
        } else if (curr->type == TOKEN_WORD) {
            CommandData *cmd = &p->cmds[curr_cmd];
            if (cmd->argc + 1 >= cmd->argv_cap) {
                cmd->argv_cap *= 2;
                cmd->argv = realloc(cmd->argv, cmd->argv_cap * sizeof(char*));
            }
            cmd->argv[cmd->argc++] = curr->value;
        }
        
        curr = curr->next;
    }
    
    for (int i = 0; i <= curr_cmd; i++) {
        p->cmds[i].argv[p->cmds[i].argc] = NULL; // null-terminate argv for execvp
    }
    
    p->cmd_count = curr_cmd + 1;
    return p;
}

void free_pipeline(Pipeline *p) {
    if (!p) return;
    for (int i = 0; i < p->cmd_count; i++) {
        free(p->cmds[i].argv);
        free(p->cmds[i].in_files);
        free(p->cmds[i].out_files);
    }
    free(p->cmds);
    free(p);
}
