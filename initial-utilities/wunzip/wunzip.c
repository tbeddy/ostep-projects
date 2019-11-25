#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  int len;
  char newchr;
  if (argc == 1) {
    printf("wunzip: file1 [file2 ...]\n");
    exit(1);
  } else {
    for (int i = 1; i < argc; i++) {
      FILE *fp = fopen(argv[i], "r");
      if (fp == NULL) {
	printf("wzip: cannot open file\n");
	exit(1);
      } else {
	while (fread(&len, 4, 1, fp) > 0) {
	  fread(&newchr, 1, 1, fp);
	  for (int i = 0; i < len; i++) {
	    printf("%c", newchr);
	  }
	}
      }
    }
  }
  return 0;
}
