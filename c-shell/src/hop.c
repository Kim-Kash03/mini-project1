// PART B: hop Builtin (B1) 
#include "hop.h"
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <limits.h>

#define MAX_FRECENCY_ENTRIES 1000

typedef struct {
    char path[PATH_MAX];
    double score;
    time_t last_visited;
} FrecencyEntry;

static FrecencyEntry entries[MAX_FRECENCY_ENTRIES];
static int num_entries = 0;
char prev_hop_dir[PATH_MAX] = {0};
static char frecency_file[PATH_MAX] = {0};

static void load_frecency() {
    char *home = getenv("HOME");
    if (!home) return;
    snprintf(frecency_file, sizeof(frecency_file), "%s/.cshell_frecency", home);
    
    FILE *f = fopen(frecency_file, "r");
    if (!f) return;
    
    num_entries = 0;
    while (num_entries < MAX_FRECENCY_ENTRIES) {
        if (fscanf(f, "%s %lf %ld", entries[num_entries].path, &entries[num_entries].score, &entries[num_entries].last_visited) != 3) {
            break;
        }
        num_entries++;
    }
    fclose(f);
}

static void save_frecency() {
    if (strlen(frecency_file) == 0) return;
    FILE *f = fopen(frecency_file, "w");
    if (!f) return;
    for (int i = 0; i < num_entries; i++) {
        fprintf(f, "%s %lf %ld\n", entries[i].path, entries[i].score, entries[i].last_visited);
    }
    fclose(f);
}

static void add_frecency(const char *path) {
    time_t now = time(NULL);
    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].path, path) == 0) {
            entries[i].score += 1.0;
            entries[i].last_visited = now;
            save_frecency();
            return;
        }
    }
    if (num_entries < MAX_FRECENCY_ENTRIES) {
        strcpy(entries[num_entries].path, path);
        entries[num_entries].score = 1.0;
        entries[num_entries].last_visited = now;
        num_entries++;
        save_frecency();
    }
}

static const char* lookup_frecency(const char *name) {
    int best_idx = -1;
    double best_rank = -1;
    
    time_t now = time(NULL);
    
    for (int i = 0; i < num_entries; i++) {
        if (strstr(entries[i].path, name) != NULL) {
            struct stat st;
            if (stat(entries[i].path, &st) == 0 && S_ISDIR(st.st_mode)) {
                // simple recency decay weight
                double age_hours = difftime(now, entries[i].last_visited) / 3600.0;
                if (age_hours < 0) age_hours = 0;
                double rank = entries[i].score / (1.0 + age_hours * 0.1); 
                
                if (rank > best_rank) {
                    best_rank = rank;
                    best_idx = i;
                }
            } else {
                entries[i].score = 0; // Invalid path penalty
            }
        }
    }
    return (best_idx != -1) ? entries[best_idx].path : NULL;
}

static void do_hop(const char *arg) {
    char target[PATH_MAX];
    char cwd[PATH_MAX];
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) return;
    
    if (strcmp(arg, "~") == 0) {
        strcpy(target, shell_home_dir);
    } else if (strcmp(arg, ".") == 0) {
        return;
    } else if (strcmp(arg, "..") == 0) {
        strcpy(target, "..");
    } else if (strcmp(arg, "-") == 0) {
        if (strlen(prev_hop_dir) == 0) {
            return;
        }
        strcpy(target, prev_hop_dir);
    } else {
        strcpy(target, arg);
    }
    
    if (chdir(target) != 0) {
        // Direct jump failed, try frecency if it's a name search
        const char *fallback = lookup_frecency(arg);
        if (fallback && chdir(fallback) == 0) {
            strcpy(prev_hop_dir, cwd);
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
                add_frecency(cwd);
            }
            return;
        }
        printf("hop: no such directory\n");
    } else {
        strcpy(prev_hop_dir, cwd);
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
            add_frecency(cwd);
        }
    }
}

int execute_hop(int argc, char **argv) {
    if (num_entries == 0) {
        load_frecency();
    }
    
    if (argc <= 1) {
        do_hop("~");
    } else {
        for (int i = 1; i < argc; i++) {
            do_hop(argv[i]);
        }
    }
    return 0;
}
