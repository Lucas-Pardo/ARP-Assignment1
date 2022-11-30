#include "./../include/inspection_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#define SIZE_MSG 3
#define DT 25000 // Time in usec = 40 Hz

// TODO write status changes to a log file in ./logs

// End-effector coordinates
float ee_x, ee_y;
float v_x, v_y;
int stopped = 0;

int sr_function(int pid_mx, int pid_mz, int r) {
    if (r) {
        ee_x = 0;
        ee_y = 0;
    }
    v_x = 0;
    v_y = 0;
    // Send interrupt signal to both motors:
    if (kill(pid_mx, SIGINT) < 0) return -1;
    if (kill(pid_mz, SIGINT) < 0) return -1;
    if (stopped) {
        stopped = 0;
    } else {
        stopped = 1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
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

    // Paths for fifos:
    char * watch_fifo = "./tmp/watch_ins";
    char * world_fifo = "./tmp/world_ins";

    // Create fifos:
    mkfifo(watch_fifo, 0666);
    mkfifo(world_fifo, 0666);

    sleep(1);

    // Open fifos:
    int fd_watch = open(watch_fifo, O_WRONLY);
    if (fd_watch < 0) perror("Error opening watch-ins fifo");
    int fd_world = open(world_fifo, O_RDONLY);
    if (fd_world < 0) perror("Error opening world-ins fifo");

    // Buffers for msgs:
    char buf[2];
    char v_buf[6];
    int m;

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
                    // mvprintw(LINES - 1, 1, "STP button pressed");
                    // if (sr_function(pid_mx, pid_mz, 0) < 0) perror("Error in sr_function");
                    // refresh();
                    // sleep(1);
                    // for(int j = 0; j < COLS; j++) {
                    //     mvaddch(LINES - 1, j, ' ');
                    // }

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in ins-watch fifo");
                }

                // RESET button pressed
                else if(check_button_pressed(rst_button, &event)) {
                    // mvprintw(LINES - 1, 1, "RST button pressed");
                    // if (sr_function(pid_mx, pid_mz, 1) < 0) perror("Error in sr_function");
                    // refresh();
                    // sleep(1);
                    // for(int j = 0; j < COLS; j++) {
                    //     mvaddch(LINES - 1, j, ' ');
                    // }

                    // Send ALIVE signal to watchdog:
                    if (write(fd_watch, "01", SIZE_MSG) != SIZE_MSG) perror("Error writing in ins-watch fifo");
                }
            }
        } 

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_world, &rfds);

        // Set the timeout:
        tv.tv_sec = 0;
        tv.tv_usec = DT;

        retval = select(fd_world + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0) perror("Error in select");
        else if (retval) {
            if (FD_ISSET(fd_world, &rfds)){
                if (read(fd_world, buf, SIZE_MSG) < 0) perror("Error reading from world-ins fifo");
                if (read(fd_world, v_buf, 7) < 0) perror("Error reading from world-ins fifo");
                sscanf(buf, "%d", &m);
                if (m) {
                    sscanf(v_buf, "%f", &v_y);
                } else {
                    sscanf(v_buf, "%f", &v_x);
                }
            }
        } 

        // Add velocities to ee pos:
        ee_x += v_x * DT / 1e6;
        ee_y += v_y * DT / 1e6;

        if (stopped) {
            mvprintw(LINES - 1, 1, "Motors stopped.");
        } else {
            mvprintw(LINES - 1, 1, "Motors working.");
        }
        refresh();
        
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
        update_console_ui(&ee_x, &ee_y);
	}
    

    // Terminate
    endwin();
    return 0;
}
