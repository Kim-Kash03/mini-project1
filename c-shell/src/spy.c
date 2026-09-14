// PART F1: Spy Command
#include "spy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

static void print_spy_line(int pid, const char *fd_name, const char *path) {
    const char *type = "UNKNOWN";
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) type = "DIR";
        else if (S_ISREG(st.st_mode)) type = "REG";
        else if (S_ISCHR(st.st_mode)) type = "CHR";
        else if (S_ISBLK(st.st_mode)) type = "BLK";
        else if (S_ISFIFO(st.st_mode)) type = "FIFO";
        else if (S_ISSOCK(st.st_mode)) type = "SOCK";
    }
    printf("%d\t%s\t%s\t%s\n", pid, fd_name, type, path);
}

int execute_spy(int argc, char **argv) {
    if (argc > 2) {
        printf("spy: invalid syntax\n");
        return 1;
    }
    
    int target_pid = getpid();
    if (argc == 2) {
        target_pid = atoi(argv[1]);
    }
    
    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d", target_pid);
    if (access(proc_path, F_OK) != 0) {
        printf("spy: no such process\n");
        return 1;
    }
    
    printf("%s\t%s\t%s\t%s\n", "PID", "FD", "TYPE", "PATH");
    
    char buf[4096];
    char link_path[1024];
    ssize_t len;
    
    // cwd
    snprintf(link_path, sizeof(link_path), "/proc/%d/cwd", target_pid);
    len = readlink(link_path, buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        print_spy_line(target_pid, "cwd", buf);
    }
    
    // txt
    snprintf(link_path, sizeof(link_path), "/proc/%d/exe", target_pid);
    len = readlink(link_path, buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        print_spy_line(target_pid, "txt", buf);
    }
    
    // mem
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", target_pid);
    FILE *f = fopen(maps_path, "r");
    if (f) {
        char *seen_paths[1024];
        int seen_count = 0;
        char line[2048];
        while (fgets(line, sizeof(line), f)) {
            char *saveptr;
            char *token = strtok_r(line, " \t\n", &saveptr);
            int col = 1;
            char *pathname = NULL;
            while (token) {
                if (col == 6) {
                    pathname = token;
                    break;
                }
                token = strtok_r(NULL, " \t\n", &saveptr);
                col++;
            }
            if (pathname && pathname[0] != '[') {
                int duplicate = 0;
                for (int i = 0; i < seen_count; i++) {
                    if (strcmp(seen_paths[i], pathname) == 0) {
                        duplicate = 1;
                        break;
                    }
                }
                if (!duplicate) {
                    if (seen_count < 1024) {
                        seen_paths[seen_count++] = strdup(pathname);
                    }
                    print_spy_line(target_pid, "mem", pathname);
                }
            }
        }
        fclose(f);
        for (int i = 0; i < seen_count; i++) {
            free(seen_paths[i]);
        }
    }
    
    // fds
    char fd_dir_path[256];
    snprintf(fd_dir_path, sizeof(fd_dir_path), "/proc/%d/fd", target_pid);
    DIR *dir = opendir(fd_dir_path);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            snprintf(link_path, sizeof(link_path), "%s/%s", fd_dir_path, ent->d_name);
            len = readlink(link_path, buf, sizeof(buf) - 1);
            if (len != -1) {
                buf[len] = '\0';
                print_spy_line(target_pid, ent->d_name, buf);
            }
        }
        closedir(dir);
    }
    
    return 0;
}
