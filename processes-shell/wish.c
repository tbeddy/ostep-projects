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
  int arg_count;
  int output;
  int output_err;
} command;

const command EMPTY_COMMAND = {{}, 0, -1, -1};

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

bool isAnyCommandEmpty(command *commands, int command_count)
{
  for (int i = 0; i < command_count; i++) {
    if (commands[i].output == EMPTY_COMMAND.output) {
      return true;
    }
  }
  return false;
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

command createCommand(char *line, char *paths[100], int paths_len)
{
  char *separated_line[100] = {};
  int arg_count = 0;
  command command;
  int count = 0;
  char program_w_path[100] = "";
  int can_access;

  while ((separated_line[count] = strsep(&line, ">")) != NULL) {
    count += 1;
  }

  if (count > 2) {
    // There are too many redirection commands
    return EMPTY_COMMAND;
  } else if (count == 2) {
    if (strcmp(separated_line[0], "") == 0) {
      // There's nothing before the redirection command
      return EMPTY_COMMAND;
    } else if (strcmp(separated_line[1], "") == 0) {
      // There's nothing after the redirection command
      return EMPTY_COMMAND;
    } else {
      separated_line[1] = trimWhiteSpace(separated_line[1]);
      if (isThereWhiteSpace(separated_line[1])) {
	// There's more than one file being passed to the redirection command
	return EMPTY_COMMAND;
      }
      // Open what's after the redirection command as a file
      mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      int output = open(separated_line[1], O_RDWR | O_CREAT | O_TRUNC, mode);
      if (output == -1) {
	return EMPTY_COMMAND;
      } else {
	command.output = output;
	command.output_err = output;
      }
    }
  } else {
    // There is no redirection command
    command.output = fileno(stdout);
    command.output_err= fileno(stderr);
  }

  separated_line[0] = trimWhiteSpace(separated_line[0]);
  // Method found here: http://c-for-dummies.com/blog/wp-content/uploads/2016/02/0220b.c
  while ((command.program_w_args[arg_count] = strsep(&separated_line[0], " ")) != NULL) {
    arg_count += 1;
  }

  // Remove white space from each string in separated_line
  for (int i = 0; i < arg_count; i++) {
    command.program_w_args[i] = trimWhiteSpace(command.program_w_args[i]);
  }

  // Don't count the actual program
  command.arg_count = arg_count - 1;

  // Find which path (if any) contains the command's program
  // Skip if the command is a built-in
  if ((strcmp(command.program_w_args[0], "exit") == 0) ||
      (strcmp(command.program_w_args[0], "cd") == 0) ||
      (strcmp(command.program_w_args[0], "path") == 0)) {
    return command;
  } else {
    for (int i = 0; i < paths_len; i++) {
      strcpy(program_w_path, paths[i]);
      strcat(program_w_path, "/");
      strcat(program_w_path, command.program_w_args[0]);
      can_access = access(program_w_path, X_OK);
      if (can_access == 0) {
	// No more paths need to be tested
	command.program_w_args[0] = strdup(program_w_path);
	break;
      }
      if ((can_access == -1) && (i == paths_len-1)) {
	// All paths have been tested, so there's an error
	return EMPTY_COMMAND;
      }
    }
    return command;
  }
}

int runCommands(command commands[], int command_count)
{
  int child_pids[100];
  int status;
  
  for (int i = 0; i < command_count; i++) {
    command command = commands[i];
    int save_out;
    int save_err;

    int rc = child_pids[i] = fork();
    if (rc < 0) {
      return 1;
    } else if (rc == 0) {
      child_pids[i] = getpid();
      // Redirection logic cribbed from here: https://stackoverflow.com/a/8517401
      save_out = dup(fileno(stdout));
      save_err = dup(fileno(stderr));
      if (-1 == dup2(command.output, fileno(stdout))) {
	return errno;
      }
      if (-1 == dup2(command.output_err, fileno(stderr))) {
	return errno;
      }
      
      execv(command.program_w_args[0], command.program_w_args);
      
      // Anything past the execv command will be reached only if there was an error
      fflush(stdout);
      fflush(stderr);
      close(command.output);
      close(command.output_err);
      dup2(save_out, fileno(stdout));
      dup2(save_err, fileno(stderr));
      close(save_out);
      close(save_err);
      
      exit(errno);
    }
  }
  for (int i = 0; i < command_count; i++) {
    int rc_wait = waitpid((pid_t) child_pids[i], &status, 0);
    printf("exit status: %d\n", status);
    return status;
  }
}

void runCommandLoop(FILE *fpinput)
{
  char *paths[100] = {"/bin"};
  int paths_len = 1;
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  printPrompt(fpinput);

  while ((linelen = getline(&line, &linecap, fpinput)) > 0) {
    if (strcmp(line, "\n") == 0) {
      exit(0);
    } else {
      line = strsep(&line, "\n");
    }

    command commands[100];
    char *possible_commands[100];
    int command_count = 0;
    
    while ((possible_commands[command_count] = strsep(&line, "&")) != NULL) {
      command_count += 1;
    }

    for (int i = 0; i < command_count; i++) {
      command command = createCommand(possible_commands[i], paths, paths_len);
      commands[i] = command;
    }

    if (isAnyCommandEmpty(commands, command_count)) {
      printError();
      printPrompt(fpinput);
      continue;
    }

    if (command_count == 1) {
      // If there's only one command, then built-in commands are available
      if (strcmp(commands[0].program_w_args[0], "exit") == 0) {
	if (commands[0].arg_count == 0) {
	  // exit is only valid with no arguments
	  exit(0);
	} else {
	  printError();
	}
      } else if (strcmp(commands[0].program_w_args[0], "cd") == 0) {
	if (commands[0].arg_count == 1) {
	  // cd is only valid with one argument
	  if ((chdir(commands[0].program_w_args[1])) == -1) {
	    printError();
	  }
	} else {
	  printError();
	}
      } else if (strcmp(commands[0].program_w_args[0], "path") == 0) {
	if (commands[0].arg_count == 0) {
	  // Erase all paths
	  memset(&paths[0], 0, sizeof(paths));
	  paths_len = 0;
	} else {
	  paths_len = 0;
	  for (int i = 0; i < commands[0].arg_count; i++) {
	    paths[i] = strdup(commands[0].program_w_args[i+1]);
	    paths_len += 1;
	  }
	}
      } else {
	// It must not be a built-in command
	if (paths_len == 0) {
	  // There are no paths, so only built-in commands could have worked
	  printError();
	} else {
	  if (0 != runCommands(commands, command_count)) {
	    printError();
	  }
	}
      }
    } else {
      if (0 != runCommands(commands, command_count)) {
	printError();
      }
    }

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
