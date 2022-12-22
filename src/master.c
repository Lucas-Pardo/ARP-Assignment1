#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#define INTIME 20 // Time of inactivity in seconds
#define DT 25000   // Time in usec (40 Hz)

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

int main(int argc, char **argv)
{

  // Debug thing:
  // pid_t pid_mx;
  // sscanf(argv[1], "%d", &pid_mx);

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

  // Remove all log files before spawning:

  DIR *directory;
  struct dirent *entry;

  directory = opendir("./logs");

  if (directory == NULL)
  {
    printf("Error opening directory.\n");
    return 1;
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

  if (closedir(directory) < 0)
    perror("Error closing directory");

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
    perror("Error spawning motorx");
  pid_t pid_mz = spawn("./bin/motorz", arg_list_motorz);
  if (pid_mz < 0)
    perror("Error spawning motorz");
  pid_t pid_world = spawn("./bin/world", arg_list_world);
  if (pid_world < 0)
    perror("Error spawning world");

  // printf("PID motor x: %d\n", pid_mx);
  // printf("PID motor z: %d\n", pid_mz);
  // printf("PID cmd: %d\n", pid_cmd);

  // Add the motors pids as arguments for inspection console:
  char buf1[10], buf2[10], buf3[10];
  sprintf(buf1, "%d", pid_mx);
  sprintf(buf2, "%d", pid_mz);
  sprintf(buf3, "%d", pid_cmd);
  char *arg_list_inspection[] = {"/usr/bin/konsole", "-e", "./bin/inspection", buf1, buf2, buf3, NULL};

  pid_t pid_insp = spawn("/usr/bin/konsole", arg_list_inspection);
  if (pid_insp < 0)
    perror("Error spawning inspection");

  // Like the command process, pid_insp does not contain the PID of
  // the inspection process, but the PID of the konsole. We use another
  // fifo to get the real PID, however, we keep track of the old one to 
  // know when the konsole closes.

  pid_t real_pid_ins;
  mkfifo(pid_fifo, 0666);
  int fd_ins = open(pid_fifo, O_RDONLY);
  if (fd_ins < 0 && errno != EINTR)
    perror("Error opening ins-master fifo");
  if (read(fd_ins, buf, 10) < 0) perror("Error reading from ins fifo (master)");
  sscanf(buf, "%d", &real_pid_ins);
  sleep(1);
  close(fd_ins);


  // ---------------------------------------------------------------------------
  //                        PERFORM WATCHDOG DUTIES
  // ---------------------------------------------------------------------------

  int status;
  int inactivity_times[10];

  // Time structure for sleeps:
  struct timespec tim;
  tim.tv_sec = 0;
  tim.tv_nsec = DT * 1000;

  // Give some time for processes to start and create the log files,
  // otherwise the loop finishes:
  sleep(5);

  // Main loop:
  while (1)
  {
    pid_t wins = waitpid(pid_insp, &status, WNOHANG);

    if (wins > 0)
      break;      // Both processes finished --> exit loop

    // Finish routine:
    if (finished)
    {
      // Just kill inspection because inspection already kills command at termination.
      kill(real_pid_ins, SIGTERM);
      break;
    }

    // Access log files:
    directory = opendir("./logs");

    if (directory == NULL)
    {
      perror("Error opening directory");
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

    if (closedir(directory) < 0)
      perror("Error closing directory");

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

  // Give time for processes to finish:
  for (int i = 3; i > 0; i--)
  {
    printf("Exiting main program in %d seconds.\n", i);
    sleep(1);
  }
  

  printf("Main program exiting with status %d\n", status);
  return 0;
}
