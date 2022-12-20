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

int main() {

  char * arg_list_command[] = { "/usr/bin/konsole", "-e", "./bin/command", NULL };
  char * arg_list_motorx[] = {"./bin/motorx", NULL };
  char * arg_list_motorz[] = {"./bin/motorz", NULL };
  char * arg_list_world[] = {"./bin/world", NULL };

  // Spawn first motors:
  pid_t pid_mx = spawn("./bin/motorx", arg_list_motorx);
  if (pid_mx < 0) printf("Error spawning motorx");
  pid_t pid_mz = spawn("./bin/motorz", arg_list_motorz);
  if (pid_mz < 0) printf("Error spawning motorz");
  pid_t pid_cmd = spawn("/usr/bin/konsole", arg_list_command);
  if (pid_cmd < 0) printf("Error spawning command");
  pid_t pid_world = spawn("./bin/world", arg_list_world);
  if (pid_world < 0) printf("Error spawning world");

  // printf("PID motor x: %d\n", pid_mx);
  // printf("PID motor z: %d\n", pid_mz);

  // Add the motors pids as arguments for inspection console:
  char buf1[10], buf2[10], buf3[10];
  sprintf(buf1, "%d", pid_mx);
  sprintf(buf2, "%d", pid_mz);
  sprintf(buf3, "%d", pid_cmd);
  char * arg_list_inspection[] = { "/usr/bin/konsole", "-e", "./bin/inspection", buf1, buf2, buf3, NULL };


  pid_t pid_insp = spawn("/usr/bin/konsole", arg_list_inspection);
  if (pid_insp < 0) printf("Error spawning inspection");

  // Add PIDs of cmd and insp to watchdog arguments:
  sprintf(buf2, "%d", pid_insp);
  char * arg_list_watchdog[] = {"./bin/watchdog", buf3, buf2, NULL };

  pid_t pid_watch = spawn("./bin/watchdog", arg_list_watchdog);
  if (pid_insp < 0) printf("Error spawning watchdog");

  // Wait only for the termination of the konsoles:
  int status;
  waitpid(pid_cmd, &status, 0);
  waitpid(pid_insp, &status, 0);

  // After, end programs in bg:
  kill(pid_mx, SIGTERM);
  kill(pid_mz, SIGTERM);
  kill(pid_world, SIGTERM);
  kill(pid_watch, SIGTERM);
  
  printf ("\nMain program exiting with status %d\n", status);
  return 0;
}

