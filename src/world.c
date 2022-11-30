#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>

#define ERROR 0.05 // 5% error
#define DT 25000 // Time in usec = 40 Hz


float rand_err(float v) {
    // Initialize random seed:
    srand(time(NULL));

    double x = rand() / ((double) RAND_MAX); // x in [0, 1]
    float er = 2 * ERROR * x - ERROR; // Scale x to [-ERROR, ERROR] 
    return (1 + er) * v;
}

int main(int argc, char ** argv){

    // Paths for fifos:
    char * mx_fifo = "./tmp/world_mx";
    char * mz_fifo = "./tmp/world_mz";
    char * ins_fifo = "./tmp/world_ins";

    // Create fifos:
    mkfifo(mx_fifo, 0666);
    mkfifo(mz_fifo, 0666);
    mkfifo(ins_fifo, 0666);

    // Open fifos:
    int fd_mx = open(mx_fifo, O_RDONLY);
    if (fd_mx < 0) perror("Error opening world-mx fifo");
    int fd_mz = open(mz_fifo, O_RDONLY);
    if (fd_mz < 0) perror("Error opening world-mz fifo");
    int fd_ins = open(ins_fifo, O_WRONLY);
    if (fd_ins < 0) perror("Error opening world-ins fifo");

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Variables for inactivity time:
    struct timeval tv;

    // Velocities:
    float v_x, v_y;

    // Buffer for messages:
    char v_buf[6];

    // Main loop:
    while(1){

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_mx, &rfds);
        FD_SET(fd_mz, &rfds);

        // Set the timeout:
        tv.tv_sec = 0;
        tv.tv_usec = DT;

        retval = select(fd_ins + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0) perror("Error in select");
        else if (retval) {
            if (FD_ISSET(fd_mx, &rfds)) {
                if (read(fd_mx, v_buf, 7) < 0) perror("Error reading from world-mx");
                sscanf(v_buf, "%f", &v_x);
                v_x = rand_err(v_x);
                sprintf(v_buf, "%.2f", v_x);
                if (write(fd_ins, "00", 3) < 0) perror("Error writing to world-ins");
                if (write(fd_ins, v_buf, 7) < 0) perror("Error writing to world-ins");
            }
            if (FD_ISSET(fd_mz, &rfds)) {
                if (read(fd_mz, v_buf, 7) < 0) perror("Error reading from world-mz");
                sscanf(v_buf, "%f", &v_y);
                v_y = rand_err(v_y);
                sprintf(v_buf, "%.2f", v_y);
                if (write(fd_ins, "01", 3) < 0) perror("Error writing to world-ins");
                if (write(fd_ins, v_buf, 7) < 0) perror("Error writing to world-ins");
            }
            printf("v_x: %.2f", v_x);
            printf("v_y: %.2f", v_y);
        } 

    }
    return 0;
}
