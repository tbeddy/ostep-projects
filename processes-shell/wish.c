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

void runCommand(char *path, char **command_w_args)
{
  int canAccess = access(path, X_OK);
  if (canAccess == 0) {
    int rc = fork();
    if (rc < 0) {
      printError();
    } else if (rc == 0) {
      char command_w_path[] = "";
      strcpy(command_w_path, path);
      strcat(command_w_path, "/");
      strcat(command_w_path, command_w_args[0]);
      command_w_args[0] = command_w_path;
      execv(command_w_args[0], command_w_args);
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
  char **command_w_args;
  int command_index;
  printf("wish> ");
  while ((linelen = getline(&line, &linecap, stdin)) > 0) {
    line = strsep(&line, "\n");
    // method found here: http://c-for-dummies.com/blog/wp-content/uploads/2016/02/0220b.c
    while ( (command_w_args[command_index] = strsep(&line, " ")) != NULL) {
      command_index += 1;
    }
    if (strcmp(command_w_args[0], "exit") == 0) {
      if (command_index == 1) {
	// exit with no arguments is valid
	exit(0);
      } else {
	// Any thing else is invalid
	printError();
      }
    } else {
      runCommand(path, command_w_args);
    }

    // Clear command_w_args and command_index
    memset(&command_w_args[0], 0, sizeof(command_w_args));
    command_index = 0;

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
