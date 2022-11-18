#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#define VEL_INC 0.2


int main(int argc, char **argv){
    // Paths for fifos:
    char * cmd_fifo = "./tmp/cmd_mx";
    char * ins_fifo = "./tmp/ins_mx";

    // Create fifos:
    if (mkfifo(cmd_fifo, S_IRUSR | S_IRGRP | S_IROTH) < 0) perror("Error while creating cmd fifo");
    if (mkfifo(ins_fifo, S_IRUSR | S_IRGRP | S_IROTH) < 0) perror("Error while creating ins fifo");

    // Open fifos:
    int fd_cmd = open(cmd_fifo, O_RDONLY);
    int fd_ins = open(ins_fifo, O_RDONLY);

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Variables for inactivity time:
    struct timeval tv;
    float timeout = 1.5;
    float in_time = 0;

    // Current velocity:
    float v = 0;

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
                // TODO cmd things
            }
            else {
                // TODO ins things
            }
        } 
        else
        {
            in_time += timeout;
        }
        
    }
    return 0;
}
