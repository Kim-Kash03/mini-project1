// PART A: Shell Prompt (A1) 
#include "prompt.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

void display_prompt() {
    char username[256] = {0};
    char hostname[256] = {0};
    char cwd[PATH_MAX] = {0};

    if (getlogin_r(username, sizeof(username)) != 0) {
        char *env_user = getenv("USER");
        if (env_user) {
            strncpy(username, env_user, sizeof(username) - 1);
        } else {
            strcpy(username, "unknown");
        }
    }
    
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strcpy(hostname, "unknown");
    }
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strcpy(cwd, "unknown");
    }

    size_t home_len = strlen(shell_home_dir);
    
    if (strncmp(cwd, shell_home_dir, home_len) == 0 && 
        (cwd[home_len] == '\0' || cwd[home_len] == '/')) {
        printf("<%s@%s:~%s> ", username, hostname, cwd + home_len);
    } else {
        printf("<%s@%s:%s> ", username, hostname, cwd);
    }
    fflush(stdout);
}
