#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>

#define INTIME 30 // Time of inactivity in seconds
#define DT 25000  // Time in usec (40 Hz)

int finished = 0;

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
    if (execvp(program, arg_list) == 0)
      ;
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

  // Remove all log files before spawning:

  DIR *directory;
  struct dirent *entry;

  directory = opendir("./logs");

  if (directory == NULL)
  {
    printf("Error opening directory.\n");
    return -1;
  }

  char filename[512];

  while ((entry = readdir(directory)) != NULL)
  {
    if (entry->d_type == DT_REG)
    {
      sprintf(filename, "./logs/%s", entry->d_name);
      if (unlink(filename) < 0)
        perror("Error deleting log files");
    }
  }

  // Start spawning:

  char *arg_list_command[] = {"/usr/bin/konsole", "-e", "./bin/command", NULL};
  char *arg_list_motorx[] = {"./bin/motorx", NULL};
  char *arg_list_motorz[] = {"./bin/motorz", NULL};
  char *arg_list_world[] = {"./bin/world", NULL};

  // Spawn everything before inspection console:
  pid_t pid_mx = spawn("./bin/motorx", arg_list_motorx);
  if (pid_mx < 0)
    printf("Error spawning motorx");
  pid_t pid_mz = spawn("./bin/motorz", arg_list_motorz);
  if (pid_mz < 0)
    printf("Error spawning motorz");
  pid_t pid_world = spawn("./bin/world", arg_list_world);
  if (pid_world < 0)
    printf("Error spawning world");
    pid_t pid_cmd = spawn("/usr/bin/konsole", arg_list_command);
  if (pid_cmd < 0)
    printf("Error spawning command");

  // printf("PID motor x: %d\n", pid_mx);
  // printf("PID motor z: %d\n", pid_mz);

  // Add the motors pids as arguments for inspection console:
  char buf1[10], buf2[10], buf3[10];
  sprintf(buf1, "%d", pid_mx);
  sprintf(buf2, "%d", pid_mz);
  sprintf(buf3, "%d", pid_cmd);
  char *arg_list_inspection[] = {"/usr/bin/konsole", "-e", "./bin/inspection", buf1, buf2, buf3, NULL};

  pid_t pid_insp = spawn("/usr/bin/konsole", arg_list_inspection);
  if (pid_insp < 0)
    printf("Error spawning inspection");  

  // ---------------------------------------------------------------------------
  //                        PERFORM WATCHDOG DUTIES
  // ---------------------------------------------------------------------------

  int status;
  int inactivity_times[10];

  // Time structure for sleeps:
  struct timespec tim;
  tim.tv_sec = 0;
  tim.tv_nsec = DT * 1000;

  // Give some time for processes to start:
  sleep(4);

  // Main loop:
  while (1)
  {
    pid_t wcmd = waitpid(pid_cmd, &status, WNOHANG);
    pid_t wins = waitpid(pid_insp, &status, WNOHANG);

    if (wcmd > 0 && wins > 0)
      break; // Both processes finished --> exit loop

    // Finish routine:
    if (finished)
    {
      kill(pid_cmd, SIGTERM);
      kill(pid_insp, SIGTERM);
      break;
    }

    // Access log files:
    directory = opendir("./logs");

    if (directory == NULL)
    {
      printf("Error opening directory.\n");
      continue;
    }

    int index = 0;
    time_t now = time(NULL);

    while ((entry = readdir(directory)) != NULL)
    {
      if (entry->d_type == DT_REG)
      {
        struct stat buf;
        sprintf(filename, "./logs/%s", entry->d_name);
        if (stat(filename, &buf) < 0)
          perror("Error obtaining stats");
        struct timespec mtime = buf.st_mtim;
        int in_time = now - mtime.tv_sec;
        inactivity_times[index++] = in_time;
      }
    }

    // Check for inactivity:
    finished = 1;
    for (int i = 0; i < index; i++)
    {
      if (inactivity_times[i] < INTIME)
      {
        finished = 0;
        break;
      }
    }
    nanosleep(&tim, NULL);
  }

  // After loop, end programs in bg:
  kill(pid_mx, SIGTERM);
  kill(pid_mz, SIGTERM);
  kill(pid_world, SIGTERM);

  printf("\nMain program exiting with status %d\n", status);
  return 0;
}
