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
#define MAXPOSITION 10 // Maximum position in z

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

int main(int argc, char **argv){

    // Log file:
    int fd_log = creat("./logs/motorz.txt", 0666);
    char log_msg[64];
    int length;

    // Signal handler:
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        length = snprintf(log_msg, 64, "Could not catch SIGUSR1: %d\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (mz)");
    }
    if (sigaction(SIGUSR2, &sa, NULL) < 0) {
        length = snprintf(log_msg, 64, "Could not catch SIGUSR2: %d\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (mz)");
    }

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    sa_exit.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0) {
        length = snprintf(log_msg, 64, "Could not catch SIGTERM: %d\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (mz)");
    }
    if (sigaction(SIGPIPE, &sa_exit, NULL) < 0) {
        length = snprintf(log_msg, 64, "Could not catch SIGPIPE: %d\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (mz)");
    }
    
    // Paths for fifos:
    char * cmd_fifo = "./tmp/cmd_mz";
    char * world_fifo = "./tmp/world_mz";

    // Create fifos:
    mkfifo(cmd_fifo, 0666);
    mkfifo(world_fifo, 0666);

    // Open fifos:
    int fd_cmd = open(cmd_fifo, O_RDONLY);
    if (fd_cmd < 0) perror("Error opening cmd-mz fifo");
    int fd_world = open(world_fifo, O_WRONLY);
    if (fd_world < 0) perror("Error opening world-mz fifo");

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
    while(!finish){

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_cmd, &rfds);

        // Set the timeout:
        tv.tv_sec = 0;
        tv.tv_usec = DT;

        retval = select(fd_world + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0 && errno != EINTR) perror("Error in select");
        else if (retval > 0) {
            if (read(fd_cmd, buf, SIZE_MSG) < 0 && errno != EINTR) {
                length = snprintf(log_msg, 64, "Error reading from cmd-mz fifo: %d\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (mz)");
            }
            sscanf(buf, "%d", &vel);
            if (vel == 0) {
                v = 0;
            } else {
                v += vel * VEL_INC;
            }
            // printf("vz: %f\n", v);
        } 

        // Reset process:
        if (reset) v = -10 * VEL_INC;

        if (v != 0) {
            // Send current desired position to world process:
            zhat += v * DT / 1e6;
            if (zhat > MAXPOSITION) zhat = MAXPOSITION;
            if (zhat < 0) {
                zhat = 0;
                if (reset) {
                    v = 0;
                    reset = 0;
                }
            }
            sprintf(z_buf, "%.2f", zhat);
            if (write(fd_world, z_buf, 7 && errno != EINTR) < 0) {
                length = snprintf(log_msg, 64, "Error writing to world-mz fifo: %d\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (mz)");
            }
            
            // Write to log file:
            length = snprintf(log_msg, 64, "Current position z = %.2f\n", zhat);
            if (write_log(fd_log, log_msg, length) < 0) perror("Error writing in log (mz)");
                
        }
    }

    // Write to log file:
    length = snprintf(log_msg, 64, "Exited succesfully.\n");
    if (write_log(fd_log, log_msg, length) < 0) perror("Error writing to log (mz)");

    // Terminate:
    close(fd_cmd);
    close(fd_log);
    close(fd_world);
    return 0;
}
