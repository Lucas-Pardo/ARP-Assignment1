#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


int spawn(const char * program, char * arg_list[]) {

  pid_t child_pid = fork();

  if(child_pid < 0) {
    perror("Error while forking...");
    return -1;
  }

  else if(child_pid != 0) {
    return child_pid;
  }

  else {
    if(execvp (program, arg_list) == 0);
    perror("Exec failed");
    return -1;
  }
}

//TODO write errors to a log file in ./logs

int main() {

  char * arg_list_command[] = { "/usr/bin/konsole", "-e", "./bin/command", NULL };
  char * arg_list_inspection[] = { "/usr/bin/konsole", "-e", "./bin/inspection", NULL };
  char * arg_list_motorx[] = { "/usr/bin/konsole", "-e", "./bin/motorx", NULL };
  char * arg_list_motorz[] = { "/usr/bin/konsole", "-e", "./bin/motorz", NULL };
  //TODO args for watchdog

  // TODO catch errors from spawn calls
  pid_t pid_cmd = spawn("/usr/bin/konsole", arg_list_command);
  pid_t pid_insp = spawn("/usr/bin/konsole", arg_list_inspection);
  pid_t pid_mx = spawn("/usr/bin/konsole", arg_list_motorx);
  pid_t pid_mz = spawn("/usr/bin/konsole", arg_list_motorz);
  //TODO spawn for watchdog

  int status;
  waitpid(pid_cmd, &status, 0);
  waitpid(pid_insp, &status, 0);
  waitpid(pid_insp, &status, 0);
  
  printf ("Main program exiting with status %d\n", status);
  return 0;
}

