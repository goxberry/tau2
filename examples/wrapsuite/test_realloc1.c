#include <stdlib.h>

int main(int argc, char **argv) {
  int *p = malloc(500);
  p = realloc(p, 1000);
  free(p);
  return 0;
}
