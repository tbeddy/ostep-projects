#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void printError()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
  exit(1);
}

void runCommand(char *path, char *line)
{
  int canAccess = access(path, X_OK);
  if (canAccess == 0) {
    int rc = fork();
    if (rc < 0) {
      printError();
    } else if (rc == 0) {
      char cmd[] = "";
      strcpy(cmd, path);
      strcat(cmd, "/");
      strcat(cmd, line);
      char *myargs[] = {cmd, NULL};
      execv(myargs[0], myargs);
      // Anything past the execv command will be reached only if there was an error
      printError();
    } else {
      int rc_wait = wait(NULL);
    }
  } else {
    printError();
  }
}

void runInteractiveMode()
{
  char *path = "/bin";
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  printf("wish> ");
  while ((linelen = getline(&line, &linecap, stdin)) > 0) {
    runCommand(path, strsep(&line, "\n"));
    printf("wish> ");
  }
}

int main(int argc, char *argv[])
{
  if (argc == 2) {
    // run batch file
    printf("Batch file option not yet implemented\n");
  } else if (argc == 1) {
    // enter interactive mode
    runInteractiveMode();
  } else {
    printError();
  }
  return 0;
}
