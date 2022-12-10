#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>

#define KILLTIME 10*1e6 // Time in usec
#define SIZE_MSG 3
#define DT 25000 // Time in usec (40 Hz)

int finish = 0;

void handler_exit(int sig) {
    finish = 1;
}

int write_log(int fd_log, int process, int status) {
    // Write to log file:
    time_t now = time(NULL);
    struct tm *timenow = localtime(&now);
    char log_msg[64];
    int length = strftime(log_msg, 64, "[%H:%M:%S]: ", timenow);
    if (write(fd_log, log_msg, length) != length) return -1;
    char st[10];
    if (status) {
        sprintf(st, "%s", "inactive");
    } else {
        sprintf(st, "%s", "active");
    }
    switch (process)
    {
    case 0:
        length = snprintf(log_msg, 64, "Command console is %s.\n", st);
        break;
    case 1:
        length = snprintf(log_msg, 64, "Inspection console is %s.\n", st);
        break;
    case 2:
        length = snprintf(log_msg, 64, "Motor X is %s.\n", st);
        break;
    case 3:
        length = snprintf(log_msg, 64, "Motor Z is %s.\n", st);
        break;
    default:
        break;
    }
    if (write(fd_log, log_msg, length) != length) return -1;
    return 0;
}

int main(int argc, char ** argv){

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    sa_exit.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0) printf("Could not catch SIGTERM.\n");

    // Get PIDs:
    int pid_cmd, pid_ins;
    if (argc != 3) {
        printf("Wrong number of arguments: pid_cmd pid_ins");
        return -1;
    } else {
        sscanf(argv[1], "%d", &pid_cmd);
        sscanf(argv[2], "%d", &pid_ins);
    }

    // Log file:
    int fd_log = creat("./logs/watch.txt", 0666);

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

    // Log variables:
    int cmd_status = 0;
    int ins_status = 0;
    int mx_status = 0;
    int mz_status = 0;

    // Buffer for messages:
    char buf[2];

    // Write to log file:
    time_t now = time(NULL);
    struct tm *timenow = localtime(&now);
    char log_msg[64];
    int length = strftime(log_msg, 64, "[%H:%M:%S]: Started watching.\n", timenow);
    if (write(fd_log, log_msg, length) != length) return -1;

    // Main loop:
    while(!finish){

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
        if (retval < 0 && errno != EINTR) perror("Error in select");
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

        // Set status variables and write to log:
        if (!cmd_status && in_time_cmd > KILLTIME) {
            cmd_status = 1;
            if (write_log(fd_log, 0, 1) < 0) perror("Error writing to log file");
        }
        if (!ins_status && in_time_ins > KILLTIME) {
            ins_status = 1;
            if (write_log(fd_log, 1, 1) < 0) perror("Error writing to log file");
        }
        if (!mx_status && in_time_mx > KILLTIME) {
            mx_status = 1;
            if (write_log(fd_log, 2, 1) < 0) perror("Error writing to log file");
        }
        if (!mz_status && in_time_mz > KILLTIME) {
            mz_status = 1;
            if (write_log(fd_log, 3, 1) < 0) perror("Error writing to log file");
        }

        if (cmd_status && in_time_cmd == 0) {
            cmd_status = 0;
            if (write_log(fd_log, 0, 0) < 0) perror("Error writing to log file");
        }
        if (ins_status && in_time_ins == 0) {
            ins_status = 0;
            if (write_log(fd_log, 1, 0) < 0) perror("Error writing to log file");
        }
        if (mx_status && in_time_mx == 0) {
            mx_status = 0;
            if (write_log(fd_log, 2, 0) < 0) perror("Error writing to log file");
        }
        if (mz_status && in_time_mz == 0) {
            mz_status = 0;
            if (write_log(fd_log, 3, 0) < 0) perror("Error writing to log file");
        }

        // Reset signal due to inactivity time:
        if (cmd_status && ins_status && mx_status && mz_status) {
            kill(pid_cmd, SIGTERM);
            kill(pid_ins, SIGTERM);
            break;
        }
    }

    // Write to log file:
    now = time(NULL);
    timenow = localtime(&now);
    log_msg[64];
    length = strftime(log_msg, 64, "[%H:%M:%S]: Exited succesfully.\n", timenow);
    if (write(fd_log, log_msg, length) != length) printf("Could not write the msg.\n");

    close(fd_cmd);
    close(fd_ins);
    close(fd_log);
    close(fd_mx);
    close(fd_mz);

    return 0;
}