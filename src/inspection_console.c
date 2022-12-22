#include "./../include/inspection_utilities.h"
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

#define SIZE_MSG 3
#define DT 25000 // Time in usec (40 Hz)

int finish = 0;
int reset = 0;

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
    char buf_pid[10];
    sprintf(buf_pid, "%d", pid);
    if (write(fd_master, buf_pid, 10) < 0) perror("Error writing to master fifo (cmd)");
    sleep(2);
    close(fd_master);

    // Log file:
    int fd_log = creat("./logs/ins.txt", 0666);
    char log_msg[64];

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    sa_exit.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0)
    {
        int length = snprintf(log_msg, 64, "Cannot catch SIGTERM.\n");
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (ins)");
    }

    // End-effector coordinates
    float ee_x = 0;
    float ee_z = 0;

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize User Interface
    init_console_ui();

    // Get PIDs:
    pid_t pid_mx, pid_mz, pid_cmd;
    if (argc != 4)
    {
        int length = snprintf(log_msg, 64, "Wrong number of arguments: pid_mx pid_mz pid_cmd.\n");
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (ins)");
        return 1;
    }
    else
    {
        sscanf(argv[1], "%d", &pid_mx);
        sscanf(argv[2], "%d", &pid_mz);
        sscanf(argv[3], "%d", &pid_cmd);
    }

    // Paths for fifos:
    char *worldx_fifo = "./tmp/worldx_ins";
    char *worldz_fifo = "./tmp/worldz_ins";

    // Create fifos:
    mkfifo(worldx_fifo, 0666);
    mkfifo(worldz_fifo, 0666);

    // sleep(1);

    // Open fifos:
    int fd_worldx = open(worldx_fifo, O_RDONLY);
    if (fd_worldx < 0)
        perror("Error opening worldx-ins fifo (ins)");
    int fd_worldz = open(worldz_fifo, O_RDONLY);
    if (fd_worldz < 0)
        perror("Error opening worldz-ins fifo (ins)");

    // Buffers for msgs:
    char buf[2];
    char position_buf[6];

    // Variables for inactivity time:
    struct timeval tv;

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Infinite loop
    while (!finish)
    {

        // Get mouse/resize commands in non-blocking mode...
        int cmd = getch();

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
        else if (cmd == KEY_MOUSE)
        {

            // Check which button has been pressed...
            if (getmouse(&event) == OK)
            {

                // STOP button pressed
                if (check_button_pressed(stp_button, &event))
                {

                    // Send RESET signal to both motors and cmd:
                    if (kill(pid_mx, SIGUSR1) < 0)
                    {
                        int length = snprintf(log_msg, 64, "Error sending signal to mx (ins): %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing to log (ins)");
                    }
                    if (kill(pid_mz, SIGUSR1) < 0)
                    {
                        int length = snprintf(log_msg, 64, "Error sending signal to mz (ins): %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing to log (ins)");
                    }
                    if (kill(pid_cmd, SIGUSR1) < 0)
                    {
                        int length = snprintf(log_msg, 64, "Error sending signal to cmd (ins): %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing to log (ins)");
                    }
                    reset = 0;

                    // Write command to log file:
                    int length = snprintf(log_msg, 64, "Pressed STOP button.\n");
                    if (write_log(fd_log, log_msg, length) < 0)
                        perror("Error writing in log (ins)");
                }

                // RESET button pressed
                else if (check_button_pressed(rst_button, &event))
                {

                    // Send RESET signal to both motors and cmd:
                    if (kill(pid_mx, SIGUSR2) < 0)
                    {
                        int length = snprintf(log_msg, 64, "Error sending signal to mx (ins): %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing to log (ins)");
                    }
                    if (kill(pid_mz, SIGUSR2) < 0)
                    {
                        int length = snprintf(log_msg, 64, "Error sending signal to mz (ins): %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing to log (ins)");
                    }
                    if (kill(pid_cmd, SIGUSR2) < 0)
                    {
                        int length = snprintf(log_msg, 64, "Error sending signal to cmd (ins): %d.\n", errno);
                        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                            perror("Error writing to log (ins)");
                    }
                    reset = 1;

                    // Write command to log file:
                    int length = snprintf(log_msg, 64, "Pressed RESET button.\n");
                    if (write_log(fd_log, log_msg, length) < 0)
                        perror("Error writing in log (ins)");
                }

                // EXIT button pressed
                else if (check_button_pressed(exit_button, &event)) break;
            }
        }

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_worldx, &rfds);
        FD_SET(fd_worldz, &rfds);

        // Set the timeout:
        tv.tv_sec = 0;
        tv.tv_usec = DT;

        retval = select(fd_worldz + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0 && errno != EINTR)
            perror("Error in select (insp)");
        else if (retval)
        {
            if (FD_ISSET(fd_worldx, &rfds))
            {
                if (read(fd_worldx, position_buf, 7) < 0)
                {
                    int length = snprintf(log_msg, 64, "Error reading from worldx-ins fifo: %d.\n", errno);
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing to log (ins)");
                }
                sscanf(position_buf, "%f", &ee_x);
            }
            if (FD_ISSET(fd_worldz, &rfds))
            {
                if (read(fd_worldz, position_buf, 7) < 0)
                {
                    int length = snprintf(log_msg, 64, "Error reading from worldz-ins fifo: %d.\n", errno);
                    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                        perror("Error writing to log (ins)");
                }
                sscanf(position_buf, "%f", &ee_z);
            }
        }

        if (ee_x <= 0 && ee_z <= 0 && reset)
        {
            if (kill(pid_cmd, SIGUSR2) < 0)
            {
                int length = snprintf(log_msg, 64, "Error sending signal to cmd (ins): %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (ins)");
            }
            reset = 0;
        }

        // To be commented in final version...
        // switch (cmd)
        // {
        //     case KEY_LEFT:
        //         ee_x--;
        //         break;
        //     case KEY_RIGHT:
        //         ee_x++;
        //         break;
        //     case KEY_UP:
        //         ee_y--;
        //         break;
        //     case KEY_DOWN:
        //         ee_y++;
        //         break;
        //     default:
        //         break;
        // }

        // Update UI
        update_console_ui(&ee_x, &ee_z);
    }

    // Send termination signal to command:
    if (kill(pid_cmd, SIGTERM) < 0)
    {
        int length = snprintf(log_msg, 64, "Error sending signal to cmd (ins): %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (ins)");
    }

    // Write to log file:
    int length = snprintf(log_msg, 64, "Exited succesfully.\n");
    if (write_log(fd_log, log_msg, length) < 0)
        printf("Could not write exit msg.\n");

    // Terminate
    close(fd_log);
    close(fd_worldx);
    close(fd_worldz);
    endwin();
    return 0;
}
