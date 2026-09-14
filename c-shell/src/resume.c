// PART E3: Resume Command
#include "resume.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

volatile sig_atomic_t resume_timed_out = 0;
pid_t resume_target_pgid = 0;

static void resume_alarm_handler(int sig) {
    if (sig == SIGALRM) {
        resume_timed_out = 1;
        if (resume_target_pgid > 0) {
            kill(-resume_target_pgid, SIGTERM);
        }
    }
}

int execute_resume(int argc, char **argv) {
    if (argc < 3) {
        printf("resume: invalid syntax\n");
        return 1;
    }
    
    if (argv[1][0] != '%') {
        printf("resume: invalid syntax\n");
        return 1;
    }
    
    int job_id = atoi(argv[1] + 1);
    Job *job = get_job_by_id(job_id);
    
    if (!job) {
        printf("resume: no such job\n");
        return 1;
    }
    
    int is_fg = -1;
    long timeout = 0;
    
    if (strcmp(argv[2], "bg") == 0) {
        if (argc != 3) {
            printf("resume: invalid syntax\n");
            return 1;
        }
        is_fg = 0;
    } else if (strcmp(argv[2], "fg") == 0) {
        if (argc == 5 && strcmp(argv[3], "--timeout") == 0) {
            timeout = atol(argv[4]);
            if (timeout <= 0) {
                printf("resume: invalid syntax\n");
                return 1;
            }
        } else if (argc != 3) {
            printf("resume: invalid syntax\n");
            return 1; 
        }
        is_fg = 1;
    } else {
        printf("resume: invalid syntax\n");
        return 1;
    }
    
    // Mark as running manually
    Process *p = job->processes;
    while (p) {
        p->state = 0; // Running
        p = p->next;
    }
    
    if (is_fg == 0) {
        // bg implementation
        if (job->processes) {
            printf("[%d] + Running    %s\n", job->job_id, job->processes->command_name);
        }
        kill(-job->pgid, SIGCONT);
        return 0;
    } else {
        // fg implementation
        p = job->processes;
        while (p) {
            printf("%s", p->command_name);
            p = p->next;
            if (p) printf(" | ");
        }
        printf("\n");
        
        sigset_t mask, oldmask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, &oldmask);
        
        tcsetpgrp(STDIN_FILENO, job->pgid);
        
        if (timeout > 0) {
            resume_timed_out = 0;
            resume_target_pgid = job->pgid;
            signal(SIGALRM, resume_alarm_handler);
            alarm(timeout);
        }
        
        kill(-job->pgid, SIGCONT);
        
        int timed_out_notice_printed = 0;
        
        p = job->processes;
        while (p) {
            int status;
            pid_t pid = p->pid;
            
            p = p->next; // advance pointer before update_process_state unlinks it
            
            while (1) {
                pid_t res = waitpid(pid, &status, WUNTRACED);
                if (res < 0) {
                    if (errno == EINTR) {
                        if (resume_timed_out) {
                            if (!timed_out_notice_printed) {
                                printf("resume: job timed out\n");
                                timed_out_notice_printed = 1;
                            }
                            continue;
                        }
                    }
                    break; 
                } else if (res > 0) {
                    update_process_state(pid, status);
                    break;
                }
            }
        }
        
        if (timeout > 0) {
            alarm(0);
        }
        
        tcsetpgrp(STDIN_FILENO, getpgrp());
        sigprocmask(SIG_SETMASK, &oldmask, NULL);
        
        return 0;
    }
}
