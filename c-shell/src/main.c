// PART A: User Input (A2)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "shell.h"
#include "prompt.h"
#include "lexer.h"
#include "parser.h"
#include "execute.h"

char shell_home_dir[PATH_MAX];

void init_shell() {
    if (getcwd(shell_home_dir, sizeof(shell_home_dir)) == NULL) {
        perror("getcwd");
        exit(1);
    }
}

int main() {
    init_shell();

    char buffer[1025];

    while (1) {
        display_prompt();
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("\n");
            break;
        }
        
        buffer[strcspn(buffer, "\n")] = '\0';

        Token *tokens = tokenize(buffer);
        if (tokens) {
            if (validate_syntax(tokens)) {
                execute_command(tokens);
            }
            free_tokens(tokens);
        } else {
            // Null means empty line or only spaces, which is valid and requires no action.
        }
    }

    return 0;
}
