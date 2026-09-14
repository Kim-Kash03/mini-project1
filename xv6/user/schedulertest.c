#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void burn_cpu(int target_ticks) {
    int start = uptime();
    while (uptime() - start < target_ticks) {
    }
    exit(0);
}

int main() {
  int pid1, pid2, pid3, pid4;

  printf("Scheduler test starting...\n");

  if ((pid1 = fork()) == 0) {
    // Process 1: Short CPU-Bound (20 ticks, visible line down to Q3)
    burn_cpu(20);
  }

  if ((pid2 = fork()) == 0) {
    // Process 2: Medium CPU-Bound (60 ticks, gets boosted once)
    burn_cpu(60);
  }

  if ((pid3 = fork()) == 0) {
    // Process 3: Very Long CPU-Bound (150 ticks, triggers 3 boosts)
    burn_cpu(150);
  }

  if ((pid4 = fork()) == 0) {
    // Process 4: I/O Bound
    for (int i = 0; i < 150; i++) {
      pause(1);
    }
    exit(0);
  }

  wait(0);
  wait(0);
  wait(0);
  wait(0);

  printf("Scheduler test done!\n");
  exit(0);
}
