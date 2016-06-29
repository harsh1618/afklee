#include <klee/klee.h>
#include <stdio.h>
#include <stdlib.h>

int status = 0;
int freed = 0;

void klee_free(void *ptr) {
  if (status == 0) status = 1;
  freed = 1;
  free(ptr);
}
