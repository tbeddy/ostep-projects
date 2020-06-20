#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

void printError()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

// Function found here: https://stackoverflow.com/a/122721
char *trimWhiteSpace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) {
    str++;
  }

  // All spaces?
  if (*str == 0) {
    return str;
  }

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) {
    end--;
  }

  return str;
}

int runCommand(char *path, char **command_w_args, int output)
{
  char command_w_path[100] = "";
  int can_access = access(path, X_OK);
  int status;
  int save_out;
  int save_err;
  
  if (can_access == 0) {
    int rc = fork();
    if (rc < 0) {
      return 1;
    } else if (rc == 0) {
      // Redirection logic cribbed from here: https://stackoverflow.com/a/8517401
      save_out = dup(fileno(stdout));
      save_err = dup(fileno(stderr));
      if (-1 == dup2(output, fileno(stdout))) {
	return errno;
      }
      if (-1 == dup2(output, fileno(stderr))) {
	return errno;
      }

      strcpy(command_w_path, path);
      strcat(command_w_path, "/");
      strcat(command_w_path, command_w_args[0]);
      command_w_args[0] = command_w_path;
      execv(command_w_args[0], command_w_args);

      // Anything past the execv command will be reached only if there was an error
      fflush(stdout);
      fflush(stderr);
      close(output);
      dup2(save_out, fileno(stdout));
      dup2(save_err, fileno(stderr));
      close(save_out);
      close(save_err);

      return errno;
    } else {
      int rc_wait = wait(&status);
      return status;
    }
  } else {
    // Can't access path
    return 1;
  }
  return 0;
}

void runThroughEachPath(char *command_w_args[100], char *paths[], int paths_len, int output)
{
  for (int i = 0; i < paths_len; i++) {
    int is_command_successful = runCommand(paths[i], command_w_args, output);
    if (is_command_successful == 0) {
      // No more paths need to be tested
      break;
    }
    if (i == paths_len-1) {
      // All paths have been tested, so there's an error
      printError();
    }
  }
}

void runCommandLoop(FILE *fpinput)
{
  char *paths[] = {"/bin"};
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  char *command_w_args[100] = {};
  int command_index = 0;
  int paths_len = 1;

  if (fpinput == stdin) {
    printf("wish> ");
  }

  while ((linelen = getline(&line, &linecap, fpinput)) > 0) {
    if (strcmp(line, "\n") == 0) {
      exit(0);
    } else {
      line = strsep(&line, "\n");
    }

    // Method found here: http://c-for-dummies.com/blog/wp-content/uploads/2016/02/0220b.c
    while ((command_w_args[command_index] = strsep(&line, " ")) != NULL) {
      command_index += 1;
    }

    // Remove white space from each string in command_w_args
    for (int i = 0; i < command_index-1; i++) {
      command_w_args[i] = trimWhiteSpace(command_w_args[i]);
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
      // It must not be a built-in command
      if (paths_len == 0) {
	// There are no paths, so only built-in commands could have worked
	printError();
      } else {
	// Check for redirection commands
	int redirection_command_list[command_index];
	int rdc_index = 0;
	for (int i = 0; i < command_index; i++) {
	  if (strcmp(command_w_args[i], ">") == 0) {
	    redirection_command_list[rdc_index] = i;
	    rdc_index += 1;
	  }
	}
	if ((rdc_index > 0) && (redirection_command_list[0] != command_index-2)) {
	  // A redirection command exists in a wrong location:
	  // 1. If in the last position, there is no file
	  // 2. If in the third to last position or earlier, there are too many files
	  printError();
	} else if (rdc_index > 1) {
	  // There is more than one redirection command, which is not allowed
	  printError();
	} else {
	  if (rdc_index == 0) {
	    // There are no redirection commands
	    runThroughEachPath(command_w_args, paths, paths_len, fileno(stdout));
	  } else {
	    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	    int output = open(command_w_args[command_index-1], O_RDWR | O_CREAT | O_TRUNC, mode);
	    if (output == -1) {
	      printError();
	    } else {
	      // Take away the redirection command and file name
	      command_w_args[command_index-1] = NULL;
	      command_w_args[command_index-2] = NULL;
	      runThroughEachPath(command_w_args, paths, paths_len, output);
	    }
	  }
	}
      }
    }

    // Clear command_w_args and command_index
    memset(&command_w_args[0], 0, sizeof(command_w_args));
    command_index = 0;

    if (fpinput == stdin) {
      printf("wish> ");
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc == 2) {
    // run batch file
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
      exit(1);
    } else {
      runCommandLoop(fp);
    }
  } else if (argc == 1) {
    // enter interactive mode
    runCommandLoop(stdin);
  } else {
    exit(1);
  }
  return 0;
}
