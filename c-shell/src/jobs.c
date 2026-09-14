// PART D2 & E1: Background Execution and Activities (Job Tracking)
// PART E1: Activities Builtin
#include "jobs.h"
#include "activities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

static Job *job_list = NULL;

void init_jobs(void) {
    job_list = NULL;
}

int add_job(pid_t pgid, int is_bg) {
    int job_id = 1;
    while (1) {
        int found = 0;
        Job *curr = job_list;
        while (curr) {
            if (curr->job_id == job_id) {
                found = 1;
                break;
            }
            curr = curr->next;
        }
        if (!found) break;
        job_id++;
    }

    Job *j = malloc(sizeof(Job));
    j->job_id = job_id;
    j->pgid = pgid;
    j->is_bg = is_bg;
    j->processes = NULL;
    j->next = NULL;

    if (!job_list) {
        job_list = j;
    } else {
        Job *curr = job_list;
        while (curr->next) curr = curr->next;
        curr->next = j;
    }
    return j->job_id;
}

void add_process_to_job(pid_t pgid, pid_t pid, const char *cmd) {
    Job *curr = job_list;
    while (curr) {
        if (curr->pgid == pgid) {
            Process *p = malloc(sizeof(Process));
            p->pid = pid;
            p->command_name = strdup(cmd);
            p->state = 0; // Running
            p->next = NULL;

            if (!curr->processes) {
                curr->processes = p;
            } else {
                Process *pc = curr->processes;
                while (pc->next) pc = pc->next;
                pc->next = p;
            }
            return;
        }
        curr = curr->next;
    }
}

static void free_process(Process *p) {
    if (p->command_name) free(p->command_name);
    free(p);
}

static void remove_empty_jobs(void) {
    Job *curr = job_list;
    Job *prev = NULL;
    while (curr) {
        if (curr->processes == NULL) {
            Job *to_del = curr;
            if (prev) {
                prev->next = curr->next;
            } else {
                job_list = curr->next;
            }
            curr = curr->next;
            free(to_del);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

void update_process_state(pid_t pid, int status) {
    Job *curr_job = job_list;
    while (curr_job) {
        Process *curr_proc = curr_job->processes;
        Process *prev_proc = NULL;
        while (curr_proc) {
            if (curr_proc->pid == pid) {
                // PART E1: Activities Builtin
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // Print completion message 
                    if (curr_job->is_bg) {
                        char buf[1024];
                        int is_normal = WIFEXITED(status);
                        int len = snprintf(buf, sizeof(buf), "\n%s with pid %d exited %s\n", 
                                           curr_proc->command_name, curr_proc->pid, 
                                           is_normal ? "normally" : "abnormally");
                        write(STDOUT_FILENO, buf, len);
                    }
                    
                    // Remove process from job
                    Process *to_del = curr_proc;
                    if (prev_proc) {
                        prev_proc->next = curr_proc->next;
                    } else {
                        curr_job->processes = curr_proc->next;
                    }
                    curr_proc = curr_proc->next;
                    free_process(to_del);
                    
                    remove_empty_jobs();
                    return; // PID belongs to one job
                } else if (WIFSTOPPED(status)) {
                    if (curr_proc->state != 1) {
                        curr_proc->state = 1; // Stopped
                        char buf[1024];
                        int len = snprintf(buf, sizeof(buf), "\n[%d] + Stopped    %s\n", curr_job->job_id, curr_proc->command_name);
                        write(STDOUT_FILENO, buf, len);
                    }
                    return;
                } else if (WIFCONTINUED(status)) {
                    curr_proc->state = 0; // Running
                    return;
                }
            }
            prev_proc = curr_proc;
            curr_proc = curr_proc->next;
        }
        curr_job = curr_job->next;
    }
}

int has_stopped_jobs(void) {
    Job *curr_job = job_list;
    while (curr_job) {
        Process *curr_proc = curr_job->processes;
        while (curr_proc) {
            if (curr_proc->state == 1) return 1;
            curr_proc = curr_proc->next;
        }
        curr_job = curr_job->next;
    }
    return 0;
}

void kill_tracked_jobs_sighup(void) {
    Job *curr_job = job_list;
    while (curr_job) {
        kill(-curr_job->pgid, SIGHUP);
        curr_job = curr_job->next;
    }
}

Job* get_job_by_id(int job_id) {
    Job *curr = job_list;
    while (curr) {
        if (curr->job_id == job_id) return curr;
        curr = curr->next;
    }
    return NULL;
}

Job* get_all_jobs(void) {
    return job_list;
}
