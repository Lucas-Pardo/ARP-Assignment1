#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#define KILLTIME 60*1e6 // Time in usec
#define SIZE_MSG 3
#define DT 25000 // Time in usec = 40 Hz

int main(int argc, char ** argv){

    // Get PIDs:
    int pid_cmd, pid_ins;
    if (argc != 3) {
        printf("Wrong number of arguments: pid_cmd pid_ins");
        return -1;
    } else {
        sscanf(argv[1], "%d", &pid_cmd);
        sscanf(argv[2], "%d", &pid_ins);
    }

    // Paths for fifos:
    char * cmd_fifo = "./tmp/watch_cmd";
    char * ins_fifo = "./tmp/watch_ins";
    char * mx_fifo = "./tmp/watch_mx";
    char * mz_fifo = "./tmp/watch_mz";

    // Create fifos:
    mkfifo(cmd_fifo, 0666);
    mkfifo(ins_fifo, 0666);
    mkfifo(mx_fifo, 0666);
    mkfifo(mz_fifo, 0666);

    // Open fifos:
    int fd_cmd = open(cmd_fifo, O_RDONLY);
    if (fd_cmd < 0) perror("Error opening cmd-watch fifo");
    int fd_ins = open(ins_fifo, O_RDONLY);
    if (fd_ins < 0) perror("Error opening ins-watch fifo");
    int fd_mx = open(mx_fifo, O_RDONLY);
    if (fd_mx < 0) perror("Error opening watch-mx fifo");
    int fd_mz = open(mz_fifo, O_RDONLY);
    if (fd_mz < 0) perror("Error opening watch-mz fifo");

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Variables for inactivity time:
    struct timeval tv;
    float in_time_cmd = 0;
    float in_time_ins = 0;
    float in_time_mx = 0;
    float in_time_mz = 0;

    // Buffer for messages:
    char buf[2];

    // Main loop:
    while(1){

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_cmd, &rfds);
        FD_SET(fd_ins, &rfds);
        FD_SET(fd_mx, &rfds);
        FD_SET(fd_mz, &rfds);

        // Set the timeout:
        tv.tv_sec = 0;
        tv.tv_usec = DT;

        retval = select(fd_mz + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0) perror("Error in select");
        else if (retval) {
            if (FD_ISSET(fd_cmd, &rfds)){
                if (read(fd_cmd, buf, SIZE_MSG) < 0) perror("Error reading from cmd-watch fifo");
                in_time_cmd = 0;
            } else {
                in_time_cmd += DT;
            }

            if (FD_ISSET(fd_ins, &rfds)){
                if (read(fd_ins, buf, SIZE_MSG) < 0) perror("Error reading from ins-watch fifo");
                in_time_ins = 0;
            } else {
                in_time_ins += DT;
            }

            if (FD_ISSET(fd_mx, &rfds)){
                if (read(fd_mx, buf, SIZE_MSG) < 0) perror("Error reading from mx-watch fifo");
                in_time_mx = 0;
            } else {
                in_time_mx += DT;
            }

            if (FD_ISSET(fd_mz, &rfds)){
                if (read(fd_mz, buf, SIZE_MSG) < 0) perror("Error reading from mz-watch fifo");
                in_time_mz = 0;
            } else {
                in_time_mz += DT;
            }
        } else {
            in_time_cmd += DT;
            in_time_ins += DT;
            in_time_mx += DT;
            in_time_mz += DT;
        }

        // Reset signal due to inactivity time:
        if (in_time_cmd > KILLTIME && in_time_ins > KILLTIME && in_time_mx > KILLTIME && in_time_mz > KILLTIME) {
            kill(pid_cmd, SIGINT);
            kill(pid_ins, SIGINT);
            break;
        }
    }
    return 0;
}