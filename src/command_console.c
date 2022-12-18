#include "./../include/command_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define SIZE_MSG 3
#define DT 25000 // Time in usec (40 Hz)

int finish = 0;
int reset = 0;

void signal_handler(int sig) {
    reset = !reset;
}

void handler_exit(int sig) {
    finish = 1;
}

int main(int argc, char const *argv[])
{

    // Signal handler:
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR2, &sa, NULL) < 0) printf("Could not catch SIGUSR2\n");

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    sa_exit.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0) printf("Could not catch SIGTERM.\n");
    if (sigaction(SIGHUP, &sa_exit, NULL) < 0) printf("Could not catch SIGHUP.\n");

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize User Interface 
    init_console_ui();

    // Log file:
    int fd_log = creat("./logs/cmd.txt", 0666);
    char log_msg[64];

    // Paths for fifos:
    char * mx_fifo = "./tmp/cmd_mx";
    char * mz_fifo = "./tmp/cmd_mz";
    char * watch_fifo = "./tmp/watch_cmd";

    // Create fifos:
    mkfifo(mx_fifo, 0666);
    mkfifo(mz_fifo, 0666);
    mkfifo(watch_fifo, 0666);

    sleep(1);

    // Open fifos:
    int fd_mx = open(mx_fifo, O_WRONLY);
    if (fd_mx < 0) perror("Error opening cmd-mx fifo");
    int fd_mz = open(mz_fifo, O_WRONLY);
    if (fd_mz < 0) perror("Error opening cmd-mz fifo");
    int fd_watch = open(watch_fifo, O_WRONLY);
    if (fd_watch < 0) perror("Error opening watch-cmd fifo");

    // Buffers for fifo communication:
    const char inc[] = "01";
    const char stop[] = "00";
    const char dec[] = "-1";
    
    // Infinite loop
    while(!finish)
	{	
        // Get mouse/resize commands in non-blocking mode...
        int cmd = getch();

        // Time structure for sleeps:
        struct timespec tim;
        tim.tv_sec = 0;
        tim.tv_nsec = DT * 1000;

        // If user resizes screen, re-draw UI
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }
        // Else if mouse has been pressed
        else if(cmd == KEY_MOUSE && !reset) {

            // Check which button has been pressed...
            if(getmouse(&event) == OK) {

                // Vx-- button pressed
                if(check_button_pressed(vx_decr_btn, &event)) {

                    // Write command to motor:
                    if (write(fd_mx, dec, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mx fifo");

                    // Write command to log file:
                    time_t now = time(NULL);
                    struct tm *timenow = localtime(&now);
                    int length = strftime(log_msg, 64, "[%H:%M:%S]: Pressed velocity decrease of motor X.\n", timenow);
                    if (write(fd_log, log_msg, length) != length) perror("Error writing in log");
                    
                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-watch fifo");
                }

                // Vx++ button pressed
                else if(check_button_pressed(vx_incr_btn, &event)) {

                    // Write command to motor:
                    if (write(fd_mx, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mx fifo");

                    // Write command to log file:
                    time_t now = time(NULL);
                    struct tm *timenow = localtime(&now);
                    int length = strftime(log_msg, 64, "[%H:%M:%S]: Pressed velocity increase of motor X.\n", timenow);
                    if (write(fd_log, log_msg, length) != length) perror("Error writing in log");

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-watch fifo");
                }

                // Vx stop button pressed
                else if(check_button_pressed(vx_stp_button, &event)) {
                    
                    // Write command to motor:
                    if (write(fd_mx, stop, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mx fifo");

                    // Write command to log file:
                    time_t now = time(NULL);
                    struct tm *timenow = localtime(&now);
                    int length = strftime(log_msg, 64, "[%H:%M:%S]: Pressed velocity stop of motor X.\n", timenow);
                    if (write(fd_log, log_msg, length) != length) perror("Error writing in log");

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-watch fifo");
                }

                // Vz-- button pressed
                else if(check_button_pressed(vz_decr_btn, &event)) {

                    // Write command to motor:
                    if (write(fd_mz, dec, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mz fifo");

                    // Write command to log file:
                    time_t now = time(NULL);
                    struct tm *timenow = localtime(&now);
                    int length = strftime(log_msg, 64, "[%H:%M:%S]: Pressed velocity decrease of motor Z.\n", timenow);
                    if (write(fd_log, log_msg, length) != length) perror("Error writing in log");

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-watch fifo");
                }

                // Vz++ button pressed
                else if(check_button_pressed(vz_incr_btn, &event)) {

                    // Write command to motor:
                    if (write(fd_mz, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mz fifo");
                    
                    // Write command to log file:
                    time_t now = time(NULL);
                    struct tm *timenow = localtime(&now);
                    int length = strftime(log_msg, 64, "[%H:%M:%S]: Pressed velocity increase of motor Z.\n", timenow);
                    if (write(fd_log, log_msg, length) != length) perror("Error writing in log");

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-watch fifo");
                }

                // Vz stop button pressed
                else if(check_button_pressed(vz_stp_button, &event)) {

                    // Write command to motor:
                    if (write(fd_mz, stop, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mz fifo");
                    
                    // Write command to log file:
                    time_t now = time(NULL);
                    struct tm *timenow = localtime(&now);
                    int length = strftime(log_msg, 64, "[%H:%M:%S]: Pressed velocity stop of motor Z.\n", timenow);
                    if (write(fd_log, log_msg, length) != length) perror("Error writing in log");

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-watch fifo");
                }
            }
        }

        refresh();
        if (nanosleep(&tim, NULL) < 0) perror("Error sleeping");
	}

    // Write to log file:
    time_t now = time(NULL);
    struct tm *timenow = localtime(&now);
    int length = strftime(log_msg, 64, "[%H:%M:%S]: Exited succesfully.\n", timenow);
    if (write(fd_log, log_msg, length) != length) printf("Could not write the msg.\n");

    // Terminate
    close(fd_log);
    close(fd_mx);
    close(fd_mz);
    close(fd_watch);
    endwin();
    return 0;
}
