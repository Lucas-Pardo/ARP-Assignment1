#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#define VEL_INC 0.2
#define SIZE_MSG 2*sizeof(char)+1

// TODO write status changes to a log file in ./logs

int main(int argc, char **argv){
    // Paths for fifos:
    char * cmd_fifo = "./tmp/cmd_mx";
    char * ins_fifo = "./tmp/ins_mx";
    char * watch_fifo = "./tmp/watch_mx";

    // Create fifos:
    mkfifo(cmd_fifo, 0666);
    mkfifo(ins_fifo, 0666);
    mkfifo(watch_fifo, 0666);

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Variables for inactivity time:
    struct timeval tv;
    float timeout = 1;

    // Current velocity:
    float v = 0;

    // Buffer for messages:
    char buf[2];
    char v_buf[4] = "0000";
    int vel;

    // Main loop:
    while(1){

        // Open fifos:
        int fd_cmd = open(cmd_fifo, O_RDONLY);
        if (fd_cmd < 0) perror("Error opening cmd-mx fifo");
        int fd_ins = open(ins_fifo, O_RDWR);
        if (fd_ins < 0) perror("Error opening ins-mx fifo");

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_cmd, &rfds);
        FD_SET(fd_ins, &rfds);

        // Set the timeout:
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        retval = select(FD_SETSIZE + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0) perror("Error in select");
        else if (retval) {
            if (FD_ISSET(fd_cmd, &rfds)){
                if (read(fd_cmd, buf, SIZE_MSG) < 0) perror("Error reading from cmd-mx fifo");
                printf("Buffer: %s\n", buf);
                sscanf(buf, "%d", &vel);
                printf("vel: %d\n", vel);
                if (vel == 0) {
                    v = 0;
                } else {
                    v += vel * VEL_INC;
                }
            }
            else {
                if (read(fd_ins, buf, SIZE_MSG) < 0) perror("Error reading from ins-mx fifo");
                printf("Motor stopped: v = 0.");
                v = 0;
            }

            // Send current velocity to inspection console:
            int length = snprintf(v_buf, 4, "%f", v);
            if (write(fd_ins, v_buf, 4*sizeof(char) + 1) < 0) perror("Error writing to ins-mx fifo");
            
            // Send ALIVE signal to watchdog:
            int fd_watch = open(watch_fifo, O_WRONLY);
            if (fd_watch < 0) perror("Error opening watch-mx fifo");
            if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in mx-watch fifo");
            close(fd_watch);

        } 
        printf("v_buf: %s\n", v_buf);
        printf("v: %f\n", v);
        close(fd_cmd);
        close(fd_ins);
    }
    return 0;
}
