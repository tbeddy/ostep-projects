#include <stdio.h>
#include <stdlib.h>

void printZip(int count, char lastchr)
{
  fwrite(&count, sizeof(count), 1, stdout);
  fwrite(&lastchr, sizeof(lastchr), 1, stdout);
}

int main(int argc, char *argv[])
{
  char lastchr;
  char newchr;
  int count = 1;
  if (argc == 1) {
    printf("wzip: file1 [file2 ...]\n");
    exit(1);
  } else {
    for (int i = 1; i < argc; i++) {
      FILE *fp = fopen(argv[i], "r");
      if (fp == NULL) {
	printf("wzip: cannot open file\n");
	exit(1);
      } else {
	// for all subsequent files, there should be a lastchr from the previous file
	if (i == 1) {
	  lastchr = getc(fp);
	}
	while ((newchr = getc(fp)) != EOF) {
	  if (lastchr == newchr) {
	    count++;
	  } else {
	    printZip(count, lastchr);
	    count = 1;
	  }
	  lastchr = newchr;
	}
      }
      fclose(fp);
    }
    printZip(count, lastchr);
  }
  return 0;
}
