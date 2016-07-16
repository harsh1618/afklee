#include <klee/klee.h>
#include <stdio.h>
#include <stdlib.h>

int klee_free_status = 0;
int klee_free_freed = 0;

void klee_free(void *ptr) {
  if (klee_free_status == 1) {
    klee_report_error(__FILE__, __LINE__, "double free", "free.err");
  }
  if (klee_free_status == 0) klee_free_status = 1;
  klee_free_freed = 1;
  free(ptr);
}
