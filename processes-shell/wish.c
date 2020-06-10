#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
  char *path = "/bin";
   if (argc == 2) {
     int canAccess = access(path, X_OK);
     if (canAccess == 0) {
       int rc = fork();
       if (rc < 0) {
	 fprintf(stderr, "Fork failed\n");
	 exit(1);
       } else if (rc == 0) {
	 char cmd[] = "";
	 strcpy(cmd, path);
	 strcat(cmd, "/");
	 strcat(cmd, argv[1]);
	 char *myargs[] = {cmd, NULL};
	 execv(myargs[0], myargs);
       } else {
	 int rc_wait = wait(NULL);
       }
     } else {
       fprintf(stderr, "Cannot access path\n");
     }
   } else if (argc > 2) {
      fprintf(stderr, "Please enter a command with no arguments\n");
   } else {
      fprintf(stderr, "No command entered\n");
   }
  return 0;
}
