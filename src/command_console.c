#include "./../include/command_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define RTIME 20

int main(int argc, char const *argv[])
{
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize User Interface 
    init_console_ui();

    // Paths for fifos:
    char * mx_fifo = "./tmp/cmd_mx";
    char * mz_fifo = "./tmp/cmd_mz";
    char * watch_fifo = "./tmp/watch_cmd";

    // Create fifos:
    if (mkfifo(mx_fifo, S_IWUSR) < 0) perror("Error while creating cmd-mx fifo");
    if (mkfifo(mz_fifo, S_IWUSR) < 0) perror("Error while creating cmd-mz fifo");
    if (mkfifo(watch_fifo, S_IWUSR) < 0) perror("Error while creating watch-cmd fifo");

    // Open fifos:
    int fd_mx = open(mx_fifo, O_WRONLY);
    if (fd_mx < 0) perror("Error opening cmd-mx fifo");
    int fd_mz = open(mz_fifo, O_WRONLY);
    if (fd_mz < 0) perror("Error opening cmd-mz fifo");
    int fd_watch = open(watch_fifo, O_WRONLY);
    if (fd_watch < 0) perror("Error opening watch-cmd fifo");

    // Buffers for fifo communication:
    const char inc[] = "1";
    size_t s_inc = sizeof(char);
    const char stop[] = "0";
    size_t s_stop = sizeof(char);
    const char dec[] = "-1";
    size_t s_dec = 2 * sizeof(char);

    // Inactivity time variables:
    float timeout = 0.2;
    float in_time = 0;
    
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

                // Vx++ button pressed
                if(check_button_pressed(vx_decr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Speed Increased");
                    refresh();
                    if (write(fd_mx, inc, s_inc) != s_inc) perror("Error writing in cmd-mx fifo");
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vx-- button pressed
                else if(check_button_pressed(vx_incr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Speed Decreased");
                    refresh();
                    if (write(fd_mx, dec, s_dec) != s_dec) perror("Error writing in cmd-mx fifo");
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vx stop button pressed
                else if(check_button_pressed(vx_stp_button, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Motor Stopped");
                    refresh();
                    if (write(fd_mx, stop, s_stop) != s_stop) perror("Error writing in cmd-mx fifo");
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vz++ button pressed
                else if(check_button_pressed(vz_decr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Speed Increased");
                    refresh();
                    if (write(fd_mz, inc, s_inc) != s_inc) perror("Error writing in cmd-mz fifo");
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vz-- button pressed
                else if(check_button_pressed(vz_incr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Speed Decreased");
                    refresh();
                    if (write(fd_mz, dec, s_dec) != s_dec) perror("Error writing in cmd-mz fifo");
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vz stop button pressed
                else if(check_button_pressed(vz_stp_button, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Motor Stopped");
                    refresh();
                    if (write(fd_mz, stop, s_stop) != s_stop) perror("Error writing in cmd-mz fifo");
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Reset inactivity timer:
                in_time = 0;
                if (write(fd_watch, stop, s_stop) != s_stop) perror("Error writing in cmd-watch fifo");
            }
        } else {
            sleep(timeout);
            in_time += timeout;
        }

        if (in_time > RTIME){
            if (write(fd_watch, inc, s_inc) != s_inc) perror("Error writing in cmd-watch fifo");
        }

        refresh();
	}

    // Terminate
    endwin();
    return 0;
}
