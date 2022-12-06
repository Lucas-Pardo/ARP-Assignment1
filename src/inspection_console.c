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

#define SIZE_MSG 3
#define DT 25000 // Time in usec = 40 Hz

// TODO write status changes to a log file in ./logs

int sr_function(int pid_mx, int pid_mz, int r) {
    if (r) {
        // Send RESET signal to both motors:
        if (kill(pid_mx, SIGUSR2) < 0) return -1;
        if (kill(pid_mz, SIGUSR2) < 0) return -1;
    } else {
        // Send STOP signal to both motors:
        if (kill(pid_mx, SIGUSR1) < 0) return -1;
        if (kill(pid_mz, SIGUSR1) < 0) return -1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{

    // End-effector coordinates
    float ee_x, ee_z;

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize User Interface 
    init_console_ui();

    // Get PIDs:
    int pid_mx, pid_mz;
    if (argc != 3) {
        printf("Wrong number of arguments: pid_mx pid_mz");
        return -1;
    } else {
        sscanf(argv[1], "%d", &pid_mx);
        sscanf(argv[2], "%d", &pid_mz);
    }

    // Log file:
    int fd_log = creat("./logs/ins.txt", 0666);

    // Paths for fifos:
    char * watch_fifo = "./tmp/watch_ins";
    char * worldx_fifo = "./tmp/worldx_ins";
    char * worldz_fifo = "./tmp/worldz_ins";

    // Create fifos:
    mkfifo(watch_fifo, 0666);
    mkfifo(worldx_fifo, 0666);
    mkfifo(worldz_fifo, 0666);

    sleep(1);

    // Open fifos:
    int fd_watch = open(watch_fifo, O_WRONLY);
    if (fd_watch < 0) perror("Error opening watch-ins fifo");
    int fd_worldx = open(worldx_fifo, O_RDONLY);
    if (fd_worldx < 0) perror("Error opening worldx-ins fifo");
    int fd_worldz = open(worldz_fifo, O_RDONLY);
    if (fd_worldz < 0) perror("Error opening worldz-ins fifo");

    // Buffers for msgs:
    char buf[2];
    char position_buf[6];

    // Variables for inactivity time:
    struct timeval tv;

    // Variables for the select():
    fd_set rfds;
    int retval;

    // Infinite loop
    while(TRUE)
	{	

        // Get mouse/resize commands in non-blocking mode...
        int cmd = getch();

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
        else if(cmd == KEY_MOUSE) {

            // Check which button has been pressed...
            if(getmouse(&event) == OK) {

                // STOP button pressed
                if(check_button_pressed(stp_button, &event)) {

                    // Write command to motors:
                    if (sr_function(pid_mx, pid_mz, 0) < 0) printf("Error in sr_function");

                    // Write command to log file:
                    time_t now = time(NULL);
                    struct tm *timenow = localtime(&now);
                    char log_msg[64];
                    int length = strftime(log_msg, 64, "[%H:%M:%S]: Pressed STOP button.\n", timenow);
                    if (write(fd_log, log_msg, length) != length) perror("Error writing in log");

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in ins-watch fifo");
                }

                // RESET button pressed
                else if(check_button_pressed(rst_button, &event)) {

                    // Write command to motors:
                    if (sr_function(pid_mx, pid_mz, 1) < 0) printf("Error in sr_function");

                    // Write command to log file:
                    time_t now = time(NULL);
                    struct tm *timenow = localtime(&now);
                    char log_msg[64];
                    int length = strftime(log_msg, 64, "[%H:%M:%S]: Pressed RESET button.\n", timenow);
                    if (write(fd_log, log_msg, length) != length) perror("Error writing in log");

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in ins-watch fifo");
                }
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
        if (retval < 0) perror("Error in select");
        else if (retval) {
            if (FD_ISSET(fd_worldx, &rfds)){
                if (read(fd_worldx, position_buf, 7) < 0) perror("Error reading from worldx-ins fifo");
                sscanf(position_buf, "%f", &ee_x);
            }
            if (FD_ISSET(fd_worldz, &rfds)){
                if (read(fd_worldz, position_buf, 7) < 0) perror("Error reading from worlz-ins fifo");
                sscanf(position_buf, "%f", &ee_z);
            }
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
    

    // Terminate
    endwin();
    return 0;
}
