// PART F2: Snoop Command
#include "snoop.h"
#include "syscalls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <time.h>
#include <signal.h>

struct SyscallStats {
    int count;
    double time_spent;
    long first_occur;
};

struct StatRecord {
    int sys_num;
    int count;
    double time_spent;
    long first_occur;
};

static int compare_records(const void *a, const void *b) {
    const struct StatRecord *sa = (const struct StatRecord *)a;
    const struct StatRecord *sb = (const struct StatRecord *)b;
    if (sa->count != sb->count) {
        return sb->count - sa->count; // Descending
    }
    return sa->first_occur - sb->first_occur; // Ascending
}

int execute_snoop(int argc, char **argv) {
    if (argc < 2) {
        printf("snoop: invalid syntax\n");
        return 1;
    }
    
    pid_t target_pid = -1;
    int is_attach = 0;
    
    if (strcmp(argv[1], "-p") == 0) {
        if (argc != 3) {
            printf("snoop: invalid syntax\n");
            return 1;
        }
        target_pid = atoi(argv[2]);
        is_attach = 1;
    }
    
    struct SyscallStats stats[1024] = {0};
    long order_counter = 0;
    
    if (is_attach) {
        if (ptrace(PTRACE_ATTACH, target_pid, 0, 0) == -1) {
            printf("snoop: no such process\n");
            return 1;
        }
    } else {
        target_pid = fork();
        if (target_pid == 0) {
            ptrace(PTRACE_TRACEME, 0, NULL, NULL);
            raise(SIGSTOP);
            execvp(argv[1], &argv[1]);
            printf("snoop: command not found\n");
            exit(255);
        }
    }
    
    int status;
    waitpid(target_pid, &status, 0); 
    
    if (!is_attach && WIFEXITED(status) && WEXITSTATUS(status) == 255) {
        return 1;
    }
    
    ptrace(PTRACE_SETOPTIONS, target_pid, 0, PTRACE_O_TRACESYSGOOD);
    
    int in_syscall = 0;
    long current_syscall = -1;
    struct timespec start, end;
    
    while (1) {
        ptrace(PTRACE_SYSCALL, target_pid, 0, 0);
        waitpid(target_pid, &status, 0);
        
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            break;
        }
        
        if (WIFSTOPPED(status)) {
            int sig = WSTOPSIG(status);
            if (sig == (SIGTRAP | 0x80)) {
                struct user_regs_struct regs;
                if (ptrace(PTRACE_GETREGS, target_pid, 0, &regs) == 0) {
                    if (!in_syscall) {
                        current_syscall = (long)regs.orig_rax;
                        in_syscall = 1;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                    } else {
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        in_syscall = 0;
                        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                        
                        if (current_syscall >= 0 && current_syscall < 1024) {
                            if (stats[current_syscall].count == 0) {
                                stats[current_syscall].first_occur = order_counter++;
                            }
                            stats[current_syscall].count++;
                            stats[current_syscall].time_spent += elapsed;
                        }
                    }
                }
            } else if (sig == SIGTRAP) {
                // Regular trap, continue
            } else if (sig == SIGSTOP && !is_attach) {
                // Ignore inherited sigstops
            } else {
            }
        }
    }
    
    struct StatRecord records[1024];
    int rcount = 0;
    for (int i = 0; i < 1024; i++) {
        if (stats[i].count > 0) {
            records[rcount].sys_num = i;
            records[rcount].count = stats[i].count;
            records[rcount].time_spent = stats[i].time_spent;
            records[rcount].first_occur = stats[i].first_occur;
            rcount++;
        }
    }
    
    qsort(records, rcount, sizeof(struct StatRecord), compare_records);
    
    if (rcount > 0) {
        printf("%-15s %-7s %-7s\n", "syscall", "calls", "time");
        for (int i = 0; i < rcount; i++) {
            char name_buf[64];
            int n = records[i].sys_num;
            if (n >= 0 && n < 1024 && syscall_names[n] != NULL) {
                strncpy(name_buf, syscall_names[n], sizeof(name_buf) - 1);
                name_buf[sizeof(name_buf) - 1] = '\0';
            } else {
                snprintf(name_buf, sizeof(name_buf), "syscall_%d", n);
            }
            printf("%-15s %-7d %0.3fs\n", name_buf, records[i].count, records[i].time_spent);
        }
    }
    return 0;
}
