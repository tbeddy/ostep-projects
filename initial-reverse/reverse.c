#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *basefilename(char *pathname)
{
  char *name = strchr(pathname, '/');
  if (name == NULL) {
    return pathname;
  } else {
    return name;
  }
}

void reverseAndPrint(FILE *fpinput, FILE *fpoutput) {
  // Both FILEs should be checked if NULL beforehand
  char *alllines = (char *) malloc(1000);
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fpinput)) > 0) {
    alllines = (char *) realloc(alllines, sizeof(alllines) + sizeof(line));
    strcat(line, alllines);
    strcpy(alllines, line);
  }
  fprintf(fpoutput, "%s", alllines);
  free(alllines);
}

int main(int argc, char *argv[])
{
  if (argc > 3) {
    fprintf(stderr, "usage: reverse <input> <output>\n");
    exit(1);
  } else if (argc == 3) {
    if (strcmp(basefilename(argv[1]), basefilename(argv[2])) == 0) {
      fprintf(stderr, "reverse: input and output file must differ\n");
      exit(1);
    } else {
      FILE *fpinput = fopen(argv[1], "r");
      if (fpinput == NULL) {
	fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
	exit(1);
      } else {
	FILE *fpoutput = fopen(argv[2], "w");
	if (fpoutput == NULL) {
	  fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
	  exit(1);
	} else {
	  reverseAndPrint(fpinput, fpoutput);
	  fclose(fpinput);
	  fclose(fpoutput);
	}
      }
    }
  } else if (argc == 2) {
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
      fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
      exit(1);
    } else {
      reverseAndPrint(fp, stdout);
      fclose(fp);
    }
  } else {
    reverseAndPrint(stdin, stdout);
  }
}
