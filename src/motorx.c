#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define VEL_INC 0.25
#define SIZE_MSG 3
#define DT 25000 // Time in usec (40 Hz)
#define MAXPOSITION 40 // Maximum position in x

// TODO write status changes to a log file in ./logs

int reset = 0;
int finish = 0;

// Current velocity:
float v = 0;

void signal_handler(int sig) {
    if (sig == SIGUSR1) { 
        reset = 0;
        v = 0;
    } else if (sig == SIGUSR2) {
        reset = 1;
    }
}

void handler_exit(int sig) {
    finish = 1;
}

int main(int argc, char **argv){

    // Signal handler:
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) printf("Could not catch SIGUSR1\n");
    if (sigaction(SIGUSR2, &sa, NULL) < 0) printf("Could not catch SIGUSR2\n");

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0) printf("Could not catch SIGTERM.\n");

    // Log file:
    int fd_log = creat("./logs/motorx.txt", 0666);

    // Paths for fifos:
    char * cmd_fifo = "./tmp/cmd_mx";
    char * world_fifo = "./tmp/world_mx";
    char * watch_fifo = "./tmp/watch_mx";

    // Create fifos:
    mkfifo(cmd_fifo, 0666);
    mkfifo(world_fifo, 0666);
    mkfifo(watch_fifo, 0666);

    // Open fifos:
    int fd_cmd = open(cmd_fifo, O_RDONLY);
    if (fd_cmd < 0) perror("Error opening cmd-mx fifo");
    int fd_world = open(world_fifo, O_WRONLY);
    if (fd_world < 0) perror("Error opening world-mx fifo");
    int fd_watch = open(watch_fifo, O_WRONLY);
    if (fd_watch < 0) perror("Error opening watch-mx fifo");

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Variables for inactivity time:
    struct timeval tv;

    // Current desired position:
    float xhat = 0;

    // Buffer for messages:
    char buf[2];
    char x_buf[6] = "000000";
    int vel;
    char log_msg[64];

    // Main loop:
    while(!finish){

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_cmd, &rfds);

        // Set the timeout:
        tv.tv_sec = 0;
        tv.tv_usec = DT;

        retval = select(fd_watch + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0 && errno != EINTR) perror("Error in select");
        else if (retval > 0) {
            if (read(fd_cmd, buf, SIZE_MSG) < 0) perror("Error reading from cmd-mx fifo");
            sscanf(buf, "%d", &vel);
            if (vel == 0) {
                v = 0;
            } else {
                v += vel * VEL_INC;
            }
            printf("vx: %f\n", v);
        } 

        // Reset process:
        if (reset) v = -10 * VEL_INC;

        if (v != 0) {
            // Send current desired position to world process:
            xhat += v * DT / 1e6;
            if (xhat > MAXPOSITION) xhat = MAXPOSITION;
            if (xhat < 0) {
                xhat = 0;
                if (reset) {
                    v = 0;
                    reset = 0;
                }
            }
            sprintf(x_buf, "%.2f", xhat);
            if (write(fd_world, x_buf, 7) < 0) perror("Error writing to world-mx fifo");

            // Write to log file:
            time_t now = time(NULL);
            struct tm *timenow = localtime(&now);
            int length = strftime(log_msg, 64, "[%H:%M:%S]: ", timenow);
            if (write(fd_log, log_msg, length) != length) perror("Error writing in log");
            length = snprintf(log_msg, 64, "Current position x = %.2f\n", xhat);
            if (write(fd_log, log_msg, length) != length) perror("Error writing in log");
                
            // Send ALIVE signal to watchdog:
            if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in mx-watch fifo");
        }
    }

    // Write to log file:
    time_t now = time(NULL);
    struct tm *timenow = localtime(&now);
    int length = strftime(log_msg, 64, "[%H:%M:%S]: Exited succesfully.\n", timenow);
    if (write(fd_log, log_msg, length) != length) return -1;

    // Terminate:
    close(fd_cmd);
    close(fd_log);
    close(fd_watch);
    close(fd_world);
    return 0;
}
