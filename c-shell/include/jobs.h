#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef struct process {
    pid_t pid;
    char *command_name;
    int state; // 0 = Running, 1 = Stopped
    struct process *next;
} Process;

typedef struct job {
    int job_id;
    pid_t pgid;
    int is_bg;
    Process *processes;
    struct job *next;
} Job;

void init_jobs(void);
int add_job(pid_t pgid, int is_bg); // Returns assigned job_id
void add_process_to_job(pid_t pgid, pid_t pid, const char *cmd);
void update_process_state(pid_t pid, int status);
Job* get_all_jobs(void);
Job* get_job_by_id(int job_id);
int has_stopped_jobs(void);
void kill_tracked_jobs_sighup(void);

#endif
