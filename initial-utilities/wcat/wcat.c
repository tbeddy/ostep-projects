#include <stdio.h>
#include <stdlib.h>

void printFile(char* filename)
{
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("wcat: cannot open file\n");
    exit(1);
  } else {
    char buffer[64];
    while(fgets(buffer, sizeof(buffer), fp) != NULL) {
      printf("%s", buffer);
    }
    fclose(fp);
  }
}

int main(int argc, char *argv[])
{
  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      printFile(argv[i]);
    }
  }
  return 0;
}
