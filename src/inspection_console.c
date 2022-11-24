#include "./../include/inspection_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#define SIZE_STOP sizeof(char)
#define RTIME 20

// End-effector coordinates
float ee_x, ee_y;
float v_x, v_y;

const char stop[] = "0";
const char inc[] = "1";

int sr_function(int fd_mx, int fd_mz, int r) {
    if (r) {
        ee_x = 0;
        ee_y = 0;
    }
    v_x = 0;
    v_y = 0;
    if (write(fd_mx, stop, SIZE_STOP) != SIZE_STOP) {
        perror("Error writing in ins-mx fifo");
        return -1;
    } 
    if (write(fd_mz, stop, SIZE_STOP) != SIZE_STOP) {
        perror("Error writing in ins-mz fifo");
        return -1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize User Interface 
    init_console_ui();

    // Paths for fifos:
    char * mx_fifo = "./tmp/ins_mx";
    char * mz_fifo = "./tmp/ins_mz";
    char * watch_fifo = "./tmp/watch_ins";

    // Create fifos:
    if (mkfifo(mx_fifo, S_IRUSR | S_IWUSR) < 0) perror("Error while creating ins-mx fifo");
    if (mkfifo(mz_fifo, S_IRUSR | S_IWUSR) < 0) perror("Error while creating ins-mz fifo");
    if (mkfifo(watch_fifo, S_IRUSR | S_IWUSR) < 0) perror("Error while creating watch-ins fifo");

    // Open fifos:
    int fd_mx = open(mx_fifo, O_RDWR);
    if (fd_mx < 0) perror("Error opening ins-mx fifo");
    int fd_mz = open(mz_fifo, O_RDWR);
    if (fd_mz < 0) perror("Error opening ins-mz fifo");
    int fd_watch = open(watch_fifo, O_RDWR);
    if (fd_watch < 0) perror("Error opening watch-ins fifo");

    // Buffers for msgs:
    char buf[10];

    // Variables for inactivity time:
    struct timeval tv;
    float timeout = 0.2;
    float in_time = 0;

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
                    mvprintw(LINES - 1, 1, "STP button pressed");
                    if (sr_function(fd_mx, fd_mz, 0) < 0) perror("Error in sr_function");
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // RESET button pressed
                else if(check_button_pressed(rst_button, &event)) {
                    mvprintw(LINES - 1, 1, "RST button pressed");
                    if (sr_function(fd_mx, fd_mz, 1) < 0) perror("Error in sr_function");
                    refresh();
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Reset inactivity timer:
                in_time = 0;
                if (write(fd_watch, stop, SIZE_STOP) != SIZE_STOP) perror("Error writing in ins-watch fifo");
            }
        } 

        // Create the set of read fds:
        FD_ZERO(&rfds);
        FD_SET(fd_mx, &rfds);
        FD_SET(fd_mz, &rfds);
        FD_SET(fd_watch, &rfds);

        // Set the timeout:
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        retval = select(fd_watch + 1, &rfds, NULL, NULL, &tv);
        if (retval < 0) perror("Error in select");
        else if (retval) {
            if (FD_ISSET(fd_watch, &rfds)){
                if (read(fd_watch, buf, 10*sizeof(char)) < 0) perror("Error reading from cmd-mx fifo");
                if (sr_function(fd_mx, fd_mz, 1) < 0) perror("Error in sr_function");
            } else {
                if (FD_ISSET(fd_mx, &rfds)){
                    if (read(fd_mx, buf, 10*sizeof(char)) < 0) perror("Error reading from cmd-mx fifo");
                    sscanf(buf, "%f", v_x);
                }
                if (FD_ISSET(fd_mz, &rfds)){
                    if (read(fd_mz, buf, 10*sizeof(char)) < 0) perror("Error reading from cmd-mx fifo");
                    sscanf(buf, "%f", v_y);
                }
            }
        } 
        else
        {
            in_time += timeout;
        }

        // Add velocities to ee pos:
        ee_x += v_x;
        ee_y += v_y;

        if (in_time > RTIME){
            if (write(fd_watch, inc, SIZE_STOP) != SIZE_STOP) perror("Error writing in ins-watch fifo");
        }

        // Get values from motors:

        
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
