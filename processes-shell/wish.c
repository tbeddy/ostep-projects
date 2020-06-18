#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void printError()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

int runCommand(char *path, char **command_w_args)
{
  char command_w_path[100] = "";
  int can_access = access(path, X_OK);
  int status;
  if (can_access == 0) {
    int rc = fork();
    if (rc < 0) {
      printError();
      exit(1);
    } else if (rc == 0) {
      strcpy(command_w_path, path);
      strcat(command_w_path, "/");
      strcat(command_w_path, command_w_args[0]);
      command_w_args[0] = command_w_path;
      int errno = execv(command_w_args[0], command_w_args);
      // Anything past the execv command will be reached only if there was an error
      exit(errno);
    } else {
      int rc_wait = wait(&status);
      return status;
    }
  } else {
    // Can't access path
    printError();
    return 1;
  }
  return 0;
}

void runInteractiveMode()
{
  char *paths[] = {"/bin"};
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  char **command_w_args;
  int command_index = 0;
  int paths_len = 1;

  printf("wish> ");

  while ((linelen = getline(&line, &linecap, stdin)) > 0) {
    line = strsep(&line, "\n");
    // method found here: http://c-for-dummies.com/blog/wp-content/uploads/2016/02/0220b.c
    while ((command_w_args[command_index] = strsep(&line, " ")) != NULL) {
      command_index += 1;
    }
    if (strcmp(command_w_args[0], "exit") == 0) {
      if (command_index == 1) {
	// exit is only valid with no arguments
	exit(0);
      } else {
	printError();
      }
    } else if (strcmp(command_w_args[0], "cd") == 0) {
      if (command_index == 2) {
	// cd is only valid with one argument
	if ((chdir(command_w_args[1])) == -1) {
	  printError();
	}
      } else {
	printError();
      }
    } else if (strcmp(command_w_args[0], "path") == 0) {
      if (command_index == 1) {
	// Erase all paths
	memset(&paths[0], 0, sizeof(paths));
	paths_len = 0;
      } else {
	for (int i = 0; i < command_index-1; i++) {
	  paths[i] = command_w_args[i+1];
	}
	paths_len = command_index - 1;
      }
    } else {
      if (paths_len == 0) {
	// There are no paths, so only built-in commands could have worked
	printError();
      } else {
	for (int i = 0; i < paths_len; i++) {
	  int is_command_successful = runCommand(paths[i], command_w_args);
	  if (is_command_successful == 0) {
	    break;
	  }
	  if (i == paths_len-1) {
	    // All paths have been tested, so there's an error
	    printError();
	  }
	}
      }
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
