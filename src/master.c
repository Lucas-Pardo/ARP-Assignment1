#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

int finished = 1;

int spawn(const char *program, char *arg_list[])
{

  pid_t child_pid = fork();

  if (child_pid < 0)
  {
    perror("Error while forking...");
    return -1;
  }

  else if (child_pid != 0)
  {
    return child_pid;
  }

  else
  {
    if (execvp(program, arg_list) < 0)
      perror("Exec failed");
      return -1;
  }
}

void handler_exit(int sig)
{
  finished = 1;
}

int main()
{

  // Signal handling to exit process:
  struct sigaction sa_exit;
  sigemptyset(&sa_exit.sa_mask);
  sa_exit.sa_handler = &handler_exit;
  sa_exit.sa_flags = SA_RESTART;
  if (sigaction(SIGTERM, &sa_exit, NULL) < 0)
    printf("Could not catch SIGTERM.\n");
  if (sigaction(SIGHUP, &sa_exit, NULL) < 0)
    printf("Could not catch SIGHUP.\n");
  if (sigaction(SIGINT, &sa_exit, NULL) < 0)
    printf("Could not catch SIGINT.\n");

  // -------------------------------------------------------------------------
  //                          SPAWN PROCESSES
  // -------------------------------------------------------------------------

  char *arg_list_command[] = {"/usr/bin/konsole", "-e", "./bin/command", NULL};
  char *arg_list_motorx[] = {"./bin/motorx", NULL};
  char *arg_list_motorz[] = {"./bin/motorz", NULL};
  char *arg_list_world[] = {"./bin/world", NULL};

  // Spawn everything before inspection console:

  pid_t pid_cmd = spawn("/usr/bin/konsole", arg_list_command);
  if (pid_cmd < 0)
    perror("Error spawning command");

  // PID stored in pid_cmd is not the true PID of command process,
  // is the PID of the konsole executing the process. To get the true PID
  // we create a quick fifo communication:
  char *pid_fifo = "./tmp/pid";
  mkfifo(pid_fifo, 0666);
  int fd_cmd = open(pid_fifo, O_RDONLY);
  if (fd_cmd < 0 && errno != EINTR)
    perror("Error opening cmd-master fifo");
  char buf[10];
  if (read(fd_cmd, buf, 10) < 0) perror("Error reading from cmd fifo (master)");
  sscanf(buf, "%d", &pid_cmd);
  close(fd_cmd);


  pid_t pid_mx = spawn("./bin/motorx", arg_list_motorx);
  if (pid_mx < 0)
    printf("Error spawning motorx");
  pid_t pid_mz = spawn("./bin/motorz", arg_list_motorz);
  if (pid_mz < 0)
    printf("Error spawning motorz");
  pid_t pid_world = spawn("./bin/world", arg_list_world);
  if (pid_world < 0)
    printf("Error spawning world");

  // printf("PID motor x: %d\n", pid_mx);
  // printf("PID motor z: %d\n", pid_mz);
  // printf("PID cmd: %d\n", pid_cmd);

  // Add the motors pids as arguments for inspection console:
  char buf1[10], buf2[10], buf3[10], buf4[10], buf5[10];
  sprintf(buf1, "%d", pid_mx);
  sprintf(buf2, "%d", pid_mz);
  sprintf(buf3, "%d", pid_world);
  sprintf(buf4, "%d", pid_cmd);
  char *arg_list_inspection[] = {"/usr/bin/konsole", "-e", "./bin/inspection", buf1, buf2, buf4, NULL};

  pid_t pid_insp = spawn("/usr/bin/konsole", arg_list_inspection);
  if (pid_insp < 0)
    printf("Error spawning inspection");

  // Like the command process, pid_insp does not contain the PID of
  // the inspection process, but the PID of the konsole. We use another
  // fifo to get the real PID.

  mkfifo(pid_fifo, 0666);
  int fd_ins = open(pid_fifo, O_RDONLY);
  if (fd_ins < 0 && errno != EINTR)
    perror("Error opening ins-master fifo");
  if (read(fd_ins, buf, 10) < 0) perror("Error reading from ins fifo (master)");
  sscanf(buf, "%d", &pid_insp);
  sleep(1);
  close(fd_ins);

  // Add PIDs to watchdog arguments:
  sprintf(buf5, "%d", pid_insp);
  char *arg_list_watchdog[] = {"./bin/watchdog", buf1, buf2, buf3, buf4, buf5, NULL};

  pid_t pid_watch = spawn("./bin/watchdog", arg_list_watchdog);
  if (pid_insp < 0)
    printf("Error spawning watchdog");

  // Wait only for the termination of the watchdog:
  int status;
  waitpid(pid_watch, &status, 0);

  printf("\nMain program exiting with status %d\n", status);
  return 0;
}
