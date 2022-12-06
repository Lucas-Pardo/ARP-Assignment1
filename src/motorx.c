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

#define VEL_INC 0.25
#define SIZE_MSG 3
#define DT 25000 // Time in usec = 40 Hz
#define MAXPOSITION 40 // Maximum position in x

// TODO write status changes to a log file in ./logs

int reset = 0;

// Current velocity:
float v = 0;

void signal_handler(int sig) {
    if (sig == SIGUSR1) { 
        printf("Stopped.\n");
        reset = 0;
        v = 0;
    } else if (sig == SIGUSR2) {
        printf("Reseting position.\n");
        reset = 1;
    }
}

int main(int argc, char **argv){

    // Signal handler:
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) printf("Could not catch SIGUSR1\n");
    if (sigaction(SIGUSR2, &sa, NULL) < 0) printf("Could not catch SIGUSR2\n");

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

    // Main loop:
    while(1){

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
                
            // Send ALIVE signal to watchdog:
            if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in mx-watch fifo");
        }
    }
    return 0;
}
