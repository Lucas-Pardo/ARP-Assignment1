#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#define VEL_INC 0.1
#define SIZE_MSG 3
#define DT 25000 // Time in usec = 40 Hz
#define MAXPOSITION 10 // Maximum position in z

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
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        printf("Can't catch SIGUSR1.\n");
    }
    if (signal(SIGUSR2, signal_handler) == SIG_ERR) {
        printf("Can't catch SIGUSR2.\n");
    }
    
    // Paths for fifos:
    char * cmd_fifo = "./tmp/cmd_mz";
    char * world_fifo = "./tmp/world_mz";
    char * watch_fifo = "./tmp/watch_mz";

    // Create fifos:
    mkfifo(cmd_fifo, 0666);
    mkfifo(world_fifo, 0666);
    mkfifo(watch_fifo, 0666);

    // Open fifos:
    int fd_cmd = open(cmd_fifo, O_RDONLY);
    if (fd_cmd < 0) perror("Error opening cmd-mz fifo");
    int fd_world = open(world_fifo, O_WRONLY);
    if (fd_world < 0) perror("Error opening world-mz fifo");
    int fd_watch = open(watch_fifo, O_WRONLY);
    if (fd_watch < 0) perror("Error opening watch-mz fifo");

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Variables for inactivity time:
    struct timeval tv;

    // Current desired position:
    float zhat = 0;

    // Buffer for messages:
    char buf[2];
    char z_buf[6] = "000000";
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
        if (retval < 0) perror("Error in select");
        else if (retval) {
            if (read(fd_cmd, buf, SIZE_MSG) < 0) perror("Error reading from cmd-mz fifo");
            sscanf(buf, "%d", &vel);
            if (vel == 0) {
                v = 0;
            } else {
                v += vel * VEL_INC;
            }
            printf("v: %f\n", v);
        } 
        if (v != 0) {
            // Reset process:
            if (reset) v = -5 * VEL_INC;
            // Send current velocity to world process:
            zhat += v * DT / 1e6;
            if (zhat > MAXPOSITION) zhat = MAXPOSITION;
            if (zhat < 0) {
                zhat = 0;
                reset = 0;
            }
            sprintf(z_buf, "%.2f", zhat);
            if (write(fd_world, z_buf, 7) < 0) perror("Error writing to world-mz fifo");
                
                
            // Send ALIVE signal to watchdog:
            if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in mz-watch fifo");
        }
    }
    return 0;
}
