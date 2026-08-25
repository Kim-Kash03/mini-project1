// PART B: peek Builtin (B3) 
#include "peek.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

struct LineBuf { char *data; size_t len; size_t cap; };

static void append_char(struct LineBuf *lb, char c) {
    if (lb->len >= lb->cap) {
        lb->cap = lb->cap ? lb->cap * 2 : 128;
        lb->data = realloc(lb->data, lb->cap);
    }
    lb->data[lb->len++] = c;
}

static void reverse_and_print(struct LineBuf *lb, int *line_num, int n_flag) {
    int is_empty = (lb->len == 0);

    if (n_flag && !is_empty) {
        printf("%d ", *line_num);
        (*line_num)--;
    }
    
    for (ssize_t i = lb->len - 1; i >= 0; i--) {
        putchar(lb->data[i]);
    }
    putchar('\n');
    lb->len = 0;
}

static void peek_file_backward(int fd, int total_non_empty, int n_flag) {
    off_t size = lseek(fd, 0, SEEK_END);
    if (size <= 0) return;

    off_t pos = size;
    char buf[4096];
    struct LineBuf lb = {NULL, 0, 0};
    int current_line_num = total_non_empty;

    char last_byte;
    lseek(fd, size - 1, SEEK_SET);
    if (read(fd, &last_byte, 1) != 1) return;
    
    int skip_first_newline = (last_byte == '\n');

    while (pos > 0) {
        ssize_t to_read = (pos > 4096) ? 4096 : pos;
        pos -= to_read;
        lseek(fd, pos, SEEK_SET);
        if (read(fd, buf, to_read) <= 0) break;

        for (ssize_t i = to_read - 1; i >= 0; i--) {
            if (skip_first_newline && pos == size - to_read && i == to_read - 1) {
                continue;
            }
            if (buf[i] == '\n') {
                reverse_and_print(&lb, &current_line_num, n_flag);
            } else {
                append_char(&lb, buf[i]);
            }
        }
    }
    
    if (size > 0 && !(skip_first_newline && lb.len == 0)) {
        reverse_and_print(&lb, &current_line_num, n_flag);
    }
    
    if (lb.data) free(lb.data);
}

static void peek_nonseekable_backward(int fd, int n_flag) {
    char *full_buf = NULL;
    size_t cap = 4096;
    size_t len = 0;
    full_buf = malloc(cap);
    
    ssize_t n;
    while ((n = read(fd, full_buf + len, cap - len)) > 0) {
        len += n;
        if (len == cap) { cap *= 2; full_buf = realloc(full_buf, cap); }
    }
    
    int total_non_empty = 0;
    int is_empty_line = 1;
    for (size_t i = 0; i < len; i++) {
        if (full_buf[i] != '\n') {
            if (is_empty_line) total_non_empty++;
            is_empty_line = 0;
        } else {
            is_empty_line = 1;
        }
    }
    
    struct LineBuf lb = {NULL, 0, 0};
    int current_line_num = total_non_empty;
    int skip_first_newline = (len > 0 && full_buf[len-1] == '\n');
    
    for (ssize_t i = len - 1; i >= 0; i--) {
        if (skip_first_newline && i == (ssize_t)len - 1) continue;
        if (full_buf[i] == '\n') reverse_and_print(&lb, &current_line_num, n_flag);
        else append_char(&lb, full_buf[i]);
    }
    
    if (len > 0 && !(skip_first_newline && lb.len == 0)) {
        reverse_and_print(&lb, &current_line_num, n_flag);
    }
    
    if (lb.data) free(lb.data);
    free(full_buf);
}

static void peek_file_forward(int fd, int n_flag) {
    char buf[4096];
    ssize_t n;
    int current_line_num = 1;
    int is_empty = 1;
    
    while ((n = read(fd, buf, 4096)) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (is_empty && n_flag && buf[i] != '\n') {
                printf("%d ", current_line_num++);
                is_empty = 0;
            }
            putchar(buf[i]);
            if (buf[i] == '\n') is_empty = 1;
        }
    }
}

static int get_total_non_empty_lines(int fd) {
    lseek(fd, 0, SEEK_SET);
    char buf[4096];
    ssize_t n;
    int cur = 0;
    int is_empty = 1;
    while ((n = read(fd, buf, 4096)) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] != '\n') {
                if (is_empty) cur++;
                is_empty = 0;
            } else {
                is_empty = 1;
            }
        }
    }
    return cur;
}

static void peek_fd(int fd, int r_flag, int n_flag) {
    int is_seekable = (lseek(fd, 0, SEEK_CUR) != -1);
    if (r_flag) {
        if (is_seekable) {
            int total = 0;
            if (n_flag) total = get_total_non_empty_lines(fd);
            peek_file_backward(fd, total, n_flag);
        } else {
            peek_nonseekable_backward(fd, n_flag);
        }
    } else {
        if (is_seekable) lseek(fd, 0, SEEK_SET); // Reset if someone else seeked
        peek_file_forward(fd, n_flag);
    }
}

int execute_peek(int argc, char **argv) {
    int r_flag = 0;
    int n_flag = 0;
    char *files[128];
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0' && argv[i][1] != '\0') {
            int is_flag = 1;
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] != 'r' && argv[i][j] != 'n') {
                    is_flag = 0; break;
                }
            }
            if (is_flag) {
                for (int j = 1; argv[i][j] != '\0'; j++) {
                    if (argv[i][j] == 'r') r_flag = 1;
                    else if (argv[i][j] == 'n') n_flag = 1;
                }
            } else {
                files[file_count++] = strdup(argv[i]); // Treat as filename
            }
        } else {
            files[file_count++] = strdup(argv[i]); 
        }
    }

    if (file_count == 0) {
        peek_fd(STDIN_FILENO, r_flag, n_flag);
    } else {
        for (int i = 0; i < file_count; i++) {
            if (strcmp(files[i], "-") == 0) {
                peek_fd(STDIN_FILENO, r_flag, n_flag);
            } else {
                struct stat st;
                if (stat(files[i], &st) != 0) {
                    printf("peek: no such file or directory\n");
                } else if (S_ISDIR(st.st_mode)) {
                    printf("peek: is a directory\n");
                } else {
                    int fd = open(files[i], O_RDONLY);
                    if (fd != -1) {
                        peek_fd(fd, r_flag, n_flag);
                        close(fd);
                    }
                }
            }
            free(files[i]);
        }
    }
    
    return 0;
}
