#include "activities.h"
#include "jobs.h"
#include <stdio.h>

int execute_activities(int argc, char **argv) {
    Job *curr_job = get_all_jobs();
    while (curr_job) {
        printf("[%d] pgid %d\n", curr_job->job_id, curr_job->pgid);
        Process *curr_proc = curr_job->processes;
        while (curr_proc) {
            printf("  %d %s   %s\n", curr_proc->pid, curr_proc->command_name, 
                   curr_proc->state == 0 ? "Running" : "Stopped");
            curr_proc = curr_proc->next;
        }
        curr_job = curr_job->next;
    }
    return 0;
}
