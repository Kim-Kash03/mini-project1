// PART B: reveal Builtin (B2) 
#include "reveal.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

static int cmp_str(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

static void reveal_dir(const char *base_path, const char *prefix, int show_hidden, int recursive) {
    DIR *d = opendir(base_path);
    if (!d) return;

    struct dirent *dir;
    char **entries = NULL;
    int count = 0;
    int capacity = 16;
    entries = malloc(capacity * sizeof(char*));

    while ((dir = readdir(d)) != NULL) {
        if (!show_hidden && dir->d_name[0] == '.') {
            continue;
        }
        
        if (count >= capacity) {
            capacity *= 2;
            entries = realloc(entries, capacity * sizeof(char*));
        }
        entries[count++] = strdup(dir->d_name);
    }
    closedir(d);

    qsort(entries, count, sizeof(char*), cmp_str);

    for (int i = 0; i < count; i++) {
        char full_entry_path[PATH_MAX];
        snprintf(full_entry_path, sizeof(full_entry_path), "%s/%s", base_path, entries[i]);

        struct stat st;
        if (stat(full_entry_path, &st) != 0) {
            printf("%s%s\n", prefix, entries[i]);
            free(entries[i]);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (recursive) {
                printf("%s%s/\n", prefix, entries[i]);
            } else {
                printf("%s%s\n", prefix, entries[i]);
            }
            if (recursive && strcmp(entries[i], ".") != 0 && strcmp(entries[i], "..") != 0) {
                char new_prefix[PATH_MAX];
                snprintf(new_prefix, sizeof(new_prefix), "%s%s/", prefix, entries[i]);
                reveal_dir(full_entry_path, new_prefix, show_hidden, recursive);
            }
        } else {
            printf("%s%s\n", prefix, entries[i]);
        }
        free(entries[i]);
    }
    free(entries);
}

int execute_reveal(int argc, char **argv) {
    int show_hidden = 0;
    int recursive = 0;
    const char *target = ".";
    int target_count = 0;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'a') show_hidden = 1;
                else if (argv[i][j] == 't') recursive = 1;
                else {
                    printf("reveal: invalid syntax\n");
                    return 1;
                }
            }
        } else {
            target = argv[i];
            target_count++;
        }
    }
    
    if (target_count > 1) {
        printf("reveal: invalid syntax\n");
        return 1;
    }

    char actual_path[PATH_MAX];
    if (strcmp(target, "~") == 0) {
        strcpy(actual_path, shell_home_dir);
    } else if (strcmp(target, "-") == 0) {
        if (strlen(prev_hop_dir) == 0) {
            printf("reveal: no such directory\n");
            return 1;
        }
        strcpy(actual_path, prev_hop_dir);
    } else {
        strcpy(actual_path, target);
    }

    struct stat st;
    if (stat(actual_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("reveal: no such directory\n");
        return 1;
    }

    reveal_dir(actual_path, "", show_hidden, recursive);
    return 0;
}
