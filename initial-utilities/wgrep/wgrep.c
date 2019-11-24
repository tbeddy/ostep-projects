#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void searchAndPrintFile(char* term, char* filename)
{
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("wgrep: cannot open file\n");
    exit(1);
  } else {
    // based on the getline man page example
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) > 0) {
      char* result = strstr(line, term);
      if (result != NULL) {
	printf("%s", line);
      }
    }
    fclose(fp);
    
    /*
    The following is my intial solution, which passes the tests but appears to be
    less safe than the code above. It would replace the contents of the else block:

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
      char* result = strstr(buffer, term);
      if (result != NULL) {
	printf("%s", buffer);
      }
    }
    fclose(fp);
    */
  }
}
void searchAndPrintStdin(char* term)
{
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, stdin)) > 0) {
    char* result = strstr(line, term);
      if (result != NULL) {
	printf("%s", line);
    }
  }
  
  /*
  The following is my intial solution, which passes the tests but appears to be
  less safe than the code above. It would replace the entire function:

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
    char* result = strstr(buffer, term);
      if (result != NULL) {
	printf("%s", buffer);
    }
  }
  */
}

int main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("wgrep: searchterm [file ...]\n");
    exit(1);
  } else if (argc == 2) {
    searchAndPrintStdin(argv[1]);
  } else {
    for (int i = 2; i < argc; i++) {
      searchAndPrintFile(argv[1], argv[i]);
    }
  }
  return 0;
}
