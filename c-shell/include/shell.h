#ifndef SHELL_H
#define SHELL_H

#include <limits.h>

extern char shell_home_dir[PATH_MAX];
extern char prev_hop_dir[PATH_MAX];

void init_shell();

#endif
