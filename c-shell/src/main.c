// PART A: User Input (A2)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "shell.h"
#include "prompt.h"
#include "lexer.h"
#include "parser.h"
#include "execute.h"
#include "jobs.h"
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

char shell_home_dir[PATH_MAX];

void init_shell() {
    if (getcwd(shell_home_dir, sizeof(shell_home_dir)) == NULL) {
        perror("getcwd");
        exit(1);
    }
}

// PART D2 & E1: Main cleanup and background job logger
void sigchld_handler(int sig) {
    int saved_errno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        update_process_state(pid, status);
    }
    errno = saved_errno;
}

int main() {
    init_shell();
    init_jobs();

    // PART D2: Hook standard posix async handler globally 
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    // PART E1: Allow shell to delegate Terminal ownership without dying
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    
    // Ignore SIGINT and SIGTSTP in the parent shell
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    char buffer[1025];
    int ctrl_d_tapped_recently = 0;

    while (1) {
        display_prompt();
        
        // PART D2: Safely abort interrupted inputs due to SIGCHLD prints
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            if (errno == EINTR) {
                clearerr(stdin);
                continue;
            }
            // EOF (Ctrl-D)
            if (has_stopped_jobs() && ctrl_d_tapped_recently == 0) {
                printf("\ncshell: there are stopped jobs\n");
                ctrl_d_tapped_recently = 1;
                clearerr(stdin);
                continue;
            }
            kill_tracked_jobs_sighup();
            printf("\n");
            break;
        }
        
        if (strlen(buffer) > 0 && buffer[0] != '\n') {
            ctrl_d_tapped_recently = 0;
        }
        
        buffer[strcspn(buffer, "\n")] = '\0';

        Token *tokens = tokenize(buffer);
        if (tokens) {
            if (validate_syntax(tokens)) {
                execute_command(tokens);
            }
            free_tokens(tokens);
        } else {
            // Null means empty line or only spaces, which is valid and requires no action.
        }
    }

    return 0;
}
