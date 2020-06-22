#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

typedef struct {
  char *program_w_args[100];
  int output;
} command;

void printError()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

void printPrompt(FILE *fpinput)
{
  if (fpinput == stdin) {
    printf("wish> ");
  }
}

bool isThereWhiteSpace(char *list)
{
  for (int i = 0; i < strlen(list); i++) {
    if (isspace(list[i])) {
      return true;
    }
  }
  return false;
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

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

int runCommand(char *path, command command)
{
  char program_w_path[100] = "";
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
      if (-1 == dup2(command.output, fileno(stdout))) {
	return errno;
      }
      if (-1 == dup2(command.output, fileno(stderr))) {
	return errno;
      }
      strcpy(program_w_path, path);
      strcat(program_w_path, "/");
      strcat(program_w_path, command.program_w_args[0]);
      command.program_w_args[0] = program_w_path;
      execv(command.program_w_args[0], command.program_w_args);

      // Anything past the execv command will be reached only if there was an error
      fflush(stdout);
      fflush(stderr);
      close(command.output);
      dup2(save_out, fileno(stdout));
      dup2(save_err, fileno(stderr));
      close(save_out);
      close(save_err);

      exit(errno);
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

void runThroughEachPath(command command, char *paths[], int paths_len)
{
  for (int i = 0; i < paths_len; i++) {
    int is_command_successful = runCommand(paths[i], command);
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

  printPrompt(fpinput);

  while ((linelen = getline(&line, &linecap, fpinput)) > 0) {
    if (strcmp(line, "\n") == 0) {
      exit(0);
    } else {
      line = strsep(&line, "\n");
    }

    command command;
    int count = 0;
    char *separated_line[100];

    while ((separated_line[count] = strsep(&line, ">")) != NULL) {
      count += 1;
    }

    if (count > 2) {
      // There are too many redirection commands
      printError();
      printPrompt(fpinput);
      continue;
    } else if (count == 2) {
      if (strcmp(separated_line[0], "") == 0) {
	// There's nothing before the redirection command
	printError();
	printPrompt(fpinput);
	continue;
      } else if (strcmp(separated_line[1], "") == 0) {
	// There's nothing after the redirection command
	printError();
	printPrompt(fpinput);
	continue;
      } else {
	separated_line[1] = trimWhiteSpace(separated_line[1]);
	// There's more than one file being passed to the redirection command
	if (isThereWhiteSpace(separated_line[1])) {
	  printError();
	  printPrompt(fpinput);
	  continue;
	}
	// Open what's after the redirection command as a file
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int output = open(trimWhiteSpace(separated_line[1]),
			  O_RDWR | O_CREAT | O_TRUNC, mode);
	if (output == -1) {
	  printError();
	  printPrompt(fpinput);
	  continue;
	} else {
	  command.output = output;
	}
      }
    } else {
      // There is no redirection command
      command.output = fileno(stdout);
    }

    separated_line[0] = trimWhiteSpace(separated_line[0]);
    // Method found here: http://c-for-dummies.com/blog/wp-content/uploads/2016/02/0220b.c
    while ((command.program_w_args[command_index] = strsep(&separated_line[0], " ")) != NULL) {
      command_index += 1;
    }

    // Remove white space from each string in command_w_args
    for (int i = 0; i < command_index; i++) {
      command.program_w_args[i] = trimWhiteSpace(command.program_w_args[i]);
    }

    if (strcmp(command.program_w_args[0], "exit") == 0) {
      if (command_index == 1) {
	// exit is only valid with no arguments
	exit(0);
      } else {
	printError();
      }
    } else if (strcmp(command.program_w_args[0], "cd") == 0) {
      if (command_index == 2) {
	// cd is only valid with one argument
	if ((chdir(command.program_w_args[1])) == -1) {
	  printError();
	}
      } else {
	printError();
      }
    } else if (strcmp(command.program_w_args[0], "path") == 0) {
      if (command_index == 1) {
	// Erase all paths
	memset(&paths[0], 0, sizeof(paths));
	paths_len = 0;
      } else {
	for (int i = 0; i < command_index-1; i++) {
	  paths[i] = command.program_w_args[i+1];
	}
	paths_len = command_index - 1;
      }
    } else {
      // It must not be a built-in command
      if (paths_len == 0) {
	// There are no paths, so only built-in commands could have worked
	printError();
      } else {
	runThroughEachPath(command, paths, paths_len);
      }
    }

    // Clear command_w_args and command_index
    memset(&command_w_args[0], 0, sizeof(command_w_args));
    command_index = 0;

    printPrompt(fpinput);
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
