// PART E4: Ping Command
#include "ping.h"
#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

int execute_ping(int argc, char **argv) {
    if (argc != 3) {
        printf("ping: invalid syntax\n");
        return 1;
    }
    
    // Validate signal number
    const char *sig_str = argv[2];
    for (int i = 0; sig_str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)sig_str[i])) {
            printf("ping: invalid syntax\n");
            return 1;
        }
    }
    
    int signal_num = atoi(sig_str);
    int sig_to_send = signal_num % 64;
    
    const char *target = argv[1];
    
    if (target[0] == '%') {
        int job_id = atoi(target + 1);
        Job *job = get_job_by_id(job_id);
        if (!job) {
            printf("ping: no such process found\n");
            return 1;
        }
        
        if (kill(-job->pgid, sig_to_send) == 0) {
            printf("Sent signal %s to %s\n", sig_str, target);
            return 0;
        } else {
            perror("ping");
            return 1;
        }
    } else {
        pid_t pid_target = (pid_t)atoi(target);
        
        Job *curr_job = get_all_jobs();
        int found = 0;
        while (curr_job && !found) {
            Process *curr_proc = curr_job->processes;
            while (curr_proc) {
                if (curr_proc->pid == pid_target) {
                    found = 1;
                    break;
                }
                curr_proc = curr_proc->next;
            }
            curr_job = curr_job->next;
        }
        
        if (!found) {
            printf("ping: no such process found\n");
            return 1;
        }
        
        if (kill(pid_target, sig_to_send) == 0) {
            printf("Sent signal %s to %s\n", sig_str, target);
            return 0;
        } else {
            perror("ping");
            return 1;
        }
    }
}
