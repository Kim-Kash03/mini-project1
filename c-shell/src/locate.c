// PART B: locate Builtin (B4) 
#include "locate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

static int is_executable_regular_file(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISREG(st.st_mode) && access(path, X_OK) == 0) {
            return 1;
        }
    }
    return 0;
}

int execute_locate(int argc, char **argv) {
    if (argc < 2) {
        printf("locate: invalid syntax\n");
        return 1;
    }
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        cwd[0] = '\0';
    }

    const char *orig_path_env = getenv("PATH");
    
    for (int i = 1; i < argc; i++) {
        int found = 0;
        char check_path[PATH_MAX * 2];
        
        // 1. Check current working directory
        if (cwd[0] != '\0') {
            snprintf(check_path, sizeof(check_path), "%s/%s", cwd, argv[i]);
            if (is_executable_regular_file(check_path)) {
                printf("%s\n", check_path);
                found = 1;
            }
        }
        
        // 2. Check PATH
        if (orig_path_env) {
            char *path_env = strdup(orig_path_env);
            if (path_env) {
                char *dir = strtok(path_env, ":");
                while (dir != NULL) {
                    snprintf(check_path, sizeof(check_path), "%s/%s", dir, argv[i]);
                    if (is_executable_regular_file(check_path)) {
                        printf("%s\n", check_path);
                        found = 1;
                    }
                    dir = strtok(NULL, ":");
                }
                free(path_env);
            }
        }
        
        if (!found) {
            printf("locate: command not found (%s)\n", argv[i]);
        }
    }
    return 0;
}
