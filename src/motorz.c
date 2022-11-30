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

// TODO write status changes to a log file in ./logs

int stopped = 0;

void signal_handler() {
    signal(SIGINT, signal_handler);
    if (stopped) {
        stopped = 0;
    } else {
        stopped = 1;
    }
}

int main(int argc, char **argv){

    // Signal handler:
    signal(SIGINT, signal_handler);
    
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

    // Current velocity:
    float v = 0;

    // Buffer for messages:
    char buf[2];
    char v_buf[6] = "000000";
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

            if (!stopped) {
                // Send current velocity to inspection console:
                sprintf(v_buf, "%.2f", v);
                if (write(fd_world, v_buf, 7) < 0) perror("Error writing to ins-mz fifo");
                printf("v: %f\n", v);
                
                // Send ALIVE signal to watchdog:
                if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in mz-watch fifo");
            }
        } 
        // printf("v_buf: %s\n", v_buf);
        // printf("v: %f\n", v);
    }
    return 0;
}
