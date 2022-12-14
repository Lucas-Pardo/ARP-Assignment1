#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <errno.h>

#define ERROR 0.05 // 5% error
#define DT 25000 // Time in usec (40 Hz)

int finish = 0;

float rand_err(float v) {
    // Initialize random seed:
    srand(time(NULL));

    double x = rand() / ((double) RAND_MAX); // x in [0, 1]
    float er = 2 * ERROR * x - ERROR; // Scale x to [-ERROR, ERROR] 
    return (1 + er) * v;
}

void handler_exit(int sig) {
    finish = 1;
}

int write_log(int fd_log, char *msg, int lmsg)
{
    char log_msg[64];
    time_t now = time(NULL);
    struct tm *timenow = localtime(&now);
    int length = strftime(log_msg, 64, "[%H:%M:%S]: ", timenow);
    if (write(fd_log, log_msg, length) < 0 && errno != EINTR)
        return -1;
    if (write(fd_log, msg, lmsg) < 0 && errno != EINTR)
        return -1;
    return 0;
}

int main(int argc, char ** argv){

    // Log file:
    int fd_log = creat("./logs/world.txt", 0666);
    char log_msg[64];
    int length;

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    sa_exit.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0) {
        length = snprintf(log_msg, 64, "Could not catch SIGTERM: %d\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (world)");
    }
    if (sigaction(SIGPIPE, &sa_exit, NULL) < 0) {
        length = snprintf(log_msg, 64, "Could not catch SIGPIPE: %d\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (world)");
    }

    // Paths for fifos:
    char * mx_fifo = "./tmp/world_mx";
    char * mz_fifo = "./tmp/world_mz";
    char * insx_fifo = "./tmp/worldx_ins";
    char * insz_fifo = "./tmp/worldz_ins";

    // Create fifos:
    mkfifo(mx_fifo, 0666);
    mkfifo(mz_fifo, 0666);
    mkfifo(insx_fifo, 0666);
    mkfifo(insz_fifo, 0666);

    // Open fifos:
    int fd_mx = open(mx_fifo, O_RDONLY);
    if (fd_mx < 0) perror("Error opening world-mx fifo");
    int fd_mz = open(mz_fifo, O_RDONLY);
    if (fd_mz < 0) perror("Error opening world-mz fifo");
    int fd_insx = open(insx_fifo, O_WRONLY);
    if (fd_insx < 0) perror("Error opening worldx-ins fifo (world)");
    int fd_insz = open(insz_fifo, O_WRONLY);
    if (fd_insz < 0) perror("Error opening worldz-ins fifo (world)");

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Variables for inactivity time:
    struct timeval tv;

    // Positions:
    float ee_x, ee_z;

    // Buffer for messages:
    char buf[6];

    // Main loop:
    while(!finish){

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_mx, &rfds);
        FD_SET(fd_mz, &rfds);

        // Set the timeout:
        tv.tv_sec = 0;
        tv.tv_usec = DT;

        retval = select(fd_insz + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0 && errno != EINTR) perror("Error in select");
        else if (retval) {
            if (FD_ISSET(fd_mx, &rfds)) {
                if (read(fd_mx, buf, 7) < 0 && errno != EINTR) {
                    length = snprintf(log_msg, 64, "Error reading from world-mx: %d\n", errno);
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing to log (world)");
                }
                sscanf(buf, "%f", &ee_x);
                ee_x = rand_err(ee_x);

                // Write to log file:
                length = snprintf(log_msg, 64, "Received x = %s, generated x = %.2f\n", buf, ee_x);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR) perror("Error writing in log (world)");

                // Write to inspection console:
                sprintf(buf, "%.2f", ee_x);
                if (write(fd_insx, buf, 7) < 0 && errno != EINTR) {
                    length = snprintf(log_msg, 64, "Error writing to worldx-ins: %d\n", errno);
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing to log (world)");
                }
            }
            if (FD_ISSET(fd_mz, &rfds)) {
                if (read(fd_mz, buf, 7) < 0 && errno != EINTR) {
                    length = snprintf(log_msg, 64, "Error reading from world-mz: %d\n", errno);
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing to log (world)");
                }
                sscanf(buf, "%f", &ee_z);
                ee_z = rand_err(ee_z);

                // Write to log file:
                length = snprintf(log_msg, 64, "Received z = %s, generated z = %.2f\n", buf, ee_z);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR) perror("Error writing in log (world)");

                // Write to inspection console:
                sprintf(buf, "%.2f", ee_z);
                if (write(fd_insz, buf, 7) < 0 && errno != EINTR) {
                    length = snprintf(log_msg, 64, "Error writing to worldz-ins: %d\n", errno);
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing to log (world)");
                }
            }
            
        } 

    }

    // Write to log file:
    length = snprintf(log_msg, 64, "Exited succesfully.\n");
    if (write_log(fd_log, log_msg, length) < 0) perror("Error writing to log (world)");

    // Terminate:
    close(fd_insx);
    close(fd_insz);
    close(fd_log);
    close(fd_mx);
    close(fd_mz);
    return 0;
}
