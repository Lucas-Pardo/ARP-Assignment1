#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#define VEL_INC 0.2
#define MSG_SIZE 2*sizeof(char)


int main(int argc, char **argv){
    // Paths for fifos:
    char * cmd_fifo = "./tmp/cmd_mx";
    char * ins_fifo = "./tmp/ins_mx";
    char * watch_fifo = "./tmp/watch_mx";

    // Create fifos:
    if (mkfifo(cmd_fifo, S_IRUSR | S_IRGRP | S_IROTH) < 0) perror("Error while creating cmd-mx fifo");
    if (mkfifo(ins_fifo, S_IRUSR | S_IRGRP | S_IROTH) < 0) perror("Error while creating ins-mx fifo");
    if (mkfifo(watch_fifo, S_IRUSR | S_IRGRP | S_IROTH) < 0) perror("Error while creating watch-mx fifo");
    // Open fifos:
    int fd_cmd = open(cmd_fifo, O_RDONLY);
    if (fd_cmd < 0) perror("Error opening cmd-mx fifo");
    int fd_ins = open(ins_fifo, O_RDONLY);
    if (fd_ins < 0) perror("Error opening ins-mx fifo");
    int fd_watch = open(watch_fifo, O_RDONLY);
    if (fd_watch < 0) perror("Error opening watch-mx fifo");

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Variables for inactivity time:
    struct timeval tv;
    float timeout = 1.5;
    float in_time = 0;

    // Current velocity:
    float v = 0;

    // Buffer for messages:
    char buf[10];
    int vel;

    // Main loop:
    while(1){
        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_cmd, &rfds);
        FD_SET(fd_ins, &rfds);

        // Set the timeout:
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        retval = select(fd_ins + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0) perror("Error in select");
        else if (retval) {
            if (FD_ISSET(fd_cmd, &rfds)){
                if (read(fd_cmd, buf, MSG_SIZE) < 0) perror("Error reading from cmd-mx fifo");
                sscanf(buf, "%d", vel);
                if (vel == 0) {
                    v = 0;
                } else {
                    v += vel * VEL_INC;
                }
            }
            else {
                if (read(fd_ins, buf, MSG_SIZE) < 0) perror("Error reading from ins-mx fifo");
                v = 0;
            }

            // Send current velocity to inspection console:
            int length = snprintf(buf, 10, "%f", v);
            if (write(fd_ins, buf, length + 1) < 0) perror("Error writing to ins-mx fifo");
            
            // Reset inactivity timer:
            in_time = 0;
        } 
        else
        {
            in_time += timeout;
        }
        
    }
    return 0;
}
