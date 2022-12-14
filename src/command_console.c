#include "./../include/command_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define SIZE_MSG 3
#define DT 25000 // Time in usec (40 Hz)

int finish = 0;
int reset = 0;

void signal_handler(int sig)
{
    if (sig == SIGUSR2) {
        reset = !reset;
    } else if (sig == SIGUSR1) {
        reset = 0;
    }
    
}

void handler_exit(int sig)
{
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

int main(int argc, char const *argv[])
{

    // Send PID to master:
    pid_t pid = getpid();
    char *master_fifo = "./tmp/pid";
    mkfifo(master_fifo, 0666);
    int fd_master = open(master_fifo, O_WRONLY);
    if (fd_master < 0 && errno != EINTR)
        perror("Error opening cmd-master fifo");
    char buf[10];
    sprintf(buf, "%d", pid);
    if (write(fd_master, buf, 10) < 0) perror("Error writing to master fifo (cmd)");
    sleep(1);
    close(fd_master);

    // Log file:
    int fd_log = creat("./logs/cmd.txt", 0666);
    char log_msg[64];
    int length;

    // Signal handler:
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR2, &sa, NULL) < 0)
    {
        int length = snprintf(log_msg, 64, "Cannot catch SIGUSR2.\n");
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (cmd)");
    }
    if (sigaction(SIGUSR1, &sa, NULL) < 0)
    {
        int length = snprintf(log_msg, 64, "Cannot catch SIGUSR1.\n");
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (cmd)");
    }

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    sa_exit.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0)
    {
        int length = snprintf(log_msg, 64, "Cannot catch SIGTERM.\n");
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (cmd)");
    }

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize User Interface
    init_console_ui();

    // Paths for fifos:
    char *mx_fifo = "./tmp/cmd_mx";
    char *mz_fifo = "./tmp/cmd_mz";

    // Create fifos:
    mkfifo(mx_fifo, 0666);
    mkfifo(mz_fifo, 0666);

    // sleep(1);

    // Open fifos:
    int fd_mx = open(mx_fifo, O_WRONLY);
    if (fd_mx < 0 && errno != EINTR)
        perror("Error opening cmd-mx fifo");
    int fd_mz = open(mz_fifo, O_WRONLY);
    if (fd_mz < 0 && errno != EINTR)
        perror("Error opening cmd-mz fifo");

    // Buffers for fifo communication:
    const char inc[] = "01";
    const char stop[] = "00";
    const char dec[] = "-1";

    // Infinite loop
    while (!finish)
    {

        // int length = snprintf(log_msg, 64, "Value of (reset, finish): (%d, %d).\n", reset, finish);
        // if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        //     perror("Error writing in log (cmd)");

        // Get mouse/resize commands in non-blocking mode...
        int cmd = getch();

        // Time structure for sleeps:
        struct timespec tim;
        tim.tv_sec = 0;
        tim.tv_nsec = DT * 1000;

        // If user resizes screen, re-draw UI
        if (cmd == KEY_RESIZE)
        {
            if (first_resize)
            {
                first_resize = FALSE;
            }
            else
            {
                reset_console_ui();
            }
        }
        // Else if mouse has been pressed
        else if (cmd == KEY_MOUSE && !reset)
        {

            // Check which button has been pressed...
            if (getmouse(&event) == OK)
            {

                // Vx-- button pressed
                if (check_button_pressed(vx_decr_btn, &event))
                {

                    // Write command to motor:
                    if (write(fd_mx, dec, SIZE_MSG) < 0 && errno != EINTR)
                    {
                        length = snprintf(log_msg, 64, "Error writing in cmd-mx fifo: %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing in log (cmd)");
                    }

                    // Write command to log file:
                    length = snprintf(log_msg, 64, "Pressed velocity decrease of motor X.\n");
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing in log (cmd)");

                }

                // Vx++ button pressed
                else if (check_button_pressed(vx_incr_btn, &event))
                {

                    // Write command to motor:
                    if (write(fd_mx, inc, SIZE_MSG) < 0 && errno != EINTR)
                    {
                        length = snprintf(log_msg, 64, "Error writing in cmd-mx fifo: %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing in log (cmd)");
                    }

                    // Write command to log file:
                    length = snprintf(log_msg, 64, "Pressed velocity increase of motor X.\n");
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing in log (cmd)");

                }

                // Vx stop button pressed
                else if (check_button_pressed(vx_stp_button, &event))
                {

                    // Write command to motor:
                    if (write(fd_mx, stop, SIZE_MSG) < 0 && errno != EINTR)
                    {
                        length = snprintf(log_msg, 64, "Error writing in cmd-mx fifo: %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing in log (cmd)");
                    }

                    // Write command to log file:
                    length = snprintf(log_msg, 64, "Pressed velocity stop of motor X.\n");
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing in log (cmd)");

                }

                // Vz-- button pressed
                else if (check_button_pressed(vz_decr_btn, &event))
                {

                    // Write command to motor:
                    if (write(fd_mz, dec, SIZE_MSG) < 0 && errno != EINTR)
                    {
                        length = snprintf(log_msg, 64, "Error writing in cmd-mz fifo: %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing in log (cmd)");
                    }

                    // Write command to log file:
                    length = snprintf(log_msg, 64, "Pressed velocity decrease of motor Z.\n");
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing in log (cmd)");

                }

                // Vz++ button pressed
                else if (check_button_pressed(vz_incr_btn, &event))
                {

                    // Write command to motor:
                    if (write(fd_mz, inc, SIZE_MSG) < 0 && errno != EINTR)
                    {
                        length = snprintf(log_msg, 64, "Error writing in cmd-mz fifo: %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing in log (cmd)");
                    }

                    // Write command to log file:
                    length = snprintf(log_msg, 64, "Pressed velocity increase of motor Z.\n");
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing in log (cmd)");

                }

                // Vz stop button pressed
                else if (check_button_pressed(vz_stp_button, &event))
                {

                    // Write command to motor:
                    if (write(fd_mz, stop, SIZE_MSG) < 0 && errno != EINTR)
                    {
                        length = snprintf(log_msg, 64, "Error writing in cmd-mz fifo: %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing in log (cmd)");
                    }

                    // Write command to log file:
                    length = snprintf(log_msg, 64, "Pressed velocity stop of motor Z.\n");
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing in log (cmd)");

                }
            }
        }

        refresh();
        if (nanosleep(&tim, NULL) < 0 && errno != EINTR)
        {
            length = snprintf(log_msg, 64, "Error writing in cmd-watch fifo: %d.\n", errno);
            if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                perror("Error writing in log (cmd)");
        }
    }

    // int length = snprintf(log_msg, 64, "Value of (reset, finish): (%d, %d).\n", reset, finish);
    // if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
    //     perror("Error writing in log (cmd)");

    // Write to log file:
    length = snprintf(log_msg, 64, "Exited succesfully.\n");
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Could not write exit msg.\n");

    // Terminate
    close(fd_log);
    close(fd_mx);
    close(fd_mz);
    endwin();
    return 0;
}
