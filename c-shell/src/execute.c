//PART C: Command Execution & Piping
#include "execute.h"
#include "execute.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include "hop.h"
#include "reveal.h"
#include "peek.h"
#include "locate.h"
#include "pipeline.h"

static int is_builtin(const char *cmd) {
    if (!cmd) return 0;
    return (strcmp(cmd, "hop") == 0 || strcmp(cmd, "reveal") == 0 || 
            strcmp(cmd, "peek") == 0 || strcmp(cmd, "locate") == 0);
}

static int run_builtin(int argc, char **argv) {
    if (strcmp(argv[0], "hop") == 0) return execute_hop(argc, argv);
    if (strcmp(argv[0], "reveal") == 0) return execute_reveal(argc, argv);
    if (strcmp(argv[0], "peek") == 0) return execute_peek(argc, argv);
    if (strcmp(argv[0], "locate") == 0) return execute_locate(argc, argv);
    return 1;
}

static void run_external(CommandData *cmd) {
    if (strchr(cmd->argv[0], '/') != NULL) {
        execv(cmd->argv[0], cmd->argv);
    } else {
        int ignore_cwd = 0;
        char *target = cmd->argv[0];
        if (target[0] == '%') {
            ignore_cwd = 1;
            target++; 
            cmd->argv[0] = target; 
        }

        if (!ignore_cwd) {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd))) {
                char local_path[PATH_MAX*2];
                snprintf(local_path, sizeof(local_path), "%s/%s", cwd, target);
                struct stat st;
                if (stat(local_path, &st) == 0 && S_ISREG(st.st_mode) && access(local_path, X_OK) == 0) {
                    execv(local_path, cmd->argv);
                }
            }
        }
        execvp(target, cmd->argv);
    }
    printf("cshell: command not found (%s)\n", cmd->argv[0]);
}

static int setup_io_redirections(CommandData *cmd) {
    if (cmd->in_count > 0) {
        if (cmd->in_count == 1) {
            int fd = open(cmd->in_files[0], O_RDONLY);
            if (fd < 0) {
                printf("cshell: no such file or directory\n");
                return -1;
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        } else {
            int cat_pipe[2];
            pipe(cat_pipe);
            pid_t fpid = fork();
            if (fpid == 0) {
                close(cat_pipe[0]);
                for (int j = 0; j < cmd->in_count; j++) {
                    int fd = open(cmd->in_files[j], O_RDONLY);
                    if (fd < 0) {
                        printf("cshell: no such file or directory\n");
                        exit(1);
                    }
                    char b[4096];
                    ssize_t n;
                    while ((n = read(fd, b, 4096)) > 0) {
                        write(cat_pipe[1], b, n);
                    }
                    close(fd);
                }
                exit(0);
            }
            close(cat_pipe[1]);
            dup2(cat_pipe[0], STDIN_FILENO);
            close(cat_pipe[0]);
        }
    }

    if (cmd->out_count > 0) {
        if (cmd->out_count == 1) {
            int flags = O_WRONLY | O_CREAT | (cmd->out_files[0].append ? O_APPEND : O_TRUNC);
            int fd = open(cmd->out_files[0].filename, flags, 0644);
            if (fd < 0) {
                printf("cshell: unable to create file for writing\n");
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else {
            int tee_pipe[2];
            pipe(tee_pipe);
            pid_t mpid = fork();
            if (mpid == 0) {
                close(tee_pipe[1]);
                int *fds = malloc(cmd->out_count * sizeof(int));
                for (int j = 0; j < cmd->out_count; j++) {
                    int flags = O_WRONLY | O_CREAT | (cmd->out_files[j].append ? O_APPEND : O_TRUNC);
                    fds[j] = open(cmd->out_files[j].filename, flags, 0644);
                    if (fds[j] < 0) {
                        printf("cshell: unable to create file for writing\n");
                        exit(1); 
                    }
                }
                char b[4096];
                ssize_t n;
                while ((n = read(tee_pipe[0], b, 4096)) > 0) {
                    for (int j = 0; j < cmd->out_count; j++) {
                        write(fds[j], b, n);
                    }
                }
                exit(0);
            }
            close(tee_pipe[0]);
            dup2(tee_pipe[1], STDOUT_FILENO);
            close(tee_pipe[1]);
        }
    }
    return 0;
}

static void execute_pipeline(Pipeline *p) {
    if (p->cmd_count == 0) return;

    if (p->cmd_count == 1 && is_builtin(p->cmds[0].argv[0])) {
        CommandData *cmd = &p->cmds[0];
        int saved_stdin = dup(STDIN_FILENO);
        int saved_stdout = dup(STDOUT_FILENO);
        
        if (setup_io_redirections(cmd) == 0) {
            run_builtin(cmd->argc, cmd->argv);
        }
        
        dup2(saved_stdin, STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdin);
        close(saved_stdout);
        return;
    }

    int pipes[128][2];
    for (int i = 0; i < p->cmd_count - 1; i++) {
        pipe(pipes[i]);
    }

    pid_t pids[128];
    for (int i = 0; i < p->cmd_count; i++) {
        CommandData *cmd = &p->cmds[i];
        if (cmd->argc == 0) continue;

        pids[i] = fork();
        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if (i < p->cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < p->cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if (setup_io_redirections(cmd) != 0) {
                exit(1);
            }

            if (is_builtin(cmd->argv[0])) {
                exit(run_builtin(cmd->argc, cmd->argv));
            } else {
                run_external(cmd);
                exit(1);
            }
        }
    }

    for (int i = 0; i < p->cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < p->cmd_count; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], NULL, 0);
        }
    }
}

void execute_command(Token *head) {
    if (!head || head->type == TOKEN_EOF) return;
    
    Pipeline *p = parse_pipeline(head);
    if (!p) return;
    
    execute_pipeline(p);
    
    free_pipeline(p);
}
