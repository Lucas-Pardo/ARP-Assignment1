#include "./../include/command_utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE_MSG 2*sizeof(char)+1

// TODO write status changes to a log file in ./logs

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
    mkfifo(mx_fifo, 0666);
    mkfifo(mz_fifo, 0666);
    mkfifo(watch_fifo, 0666);

    // Buffers for fifo communication:
    const char inc[] = "01";
    const char stop[] = "00";
    const char dec[] = "-1";
    
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

                // Vx-- button pressed
                if(check_button_pressed(vx_decr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Speed Increased");
                    refresh();
                    int fd_mx = open(mx_fifo, O_WRONLY);
                    if (fd_mx < 0) perror("Error opening cmd-mx fifo");
                    if (write(fd_mx, dec, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mx fifo");
                    close(fd_mx);
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vx++ button pressed
                else if(check_button_pressed(vx_incr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Speed Decreased");
                    refresh();
                    int fd_mx = open(mx_fifo, O_WRONLY);
                    if (fd_mx < 0) perror("Error opening cmd-mx fifo");
                    if (write(fd_mx, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mx fifo");
                    close(fd_mx);
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vx stop button pressed
                else if(check_button_pressed(vx_stp_button, &event)) {
                    mvprintw(LINES - 1, 1, "Horizontal Motor Stopped");
                    refresh();
                    int fd_mx = open(mx_fifo, O_WRONLY);
                    if (fd_mx < 0) perror("Error opening cmd-mx fifo");
                    if (write(fd_mx, stop, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mx fifo");
                    close(fd_mx);
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vz-- button pressed
                else if(check_button_pressed(vz_decr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Speed Increased");
                    refresh();
                    int fd_mz = open(mz_fifo, O_WRONLY);
                    if (fd_mz < 0) perror("Error opening cmd-mz fifo");
                    if (write(fd_mz, dec, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mz fifo");
                    close(fd_mz);
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vz++ button pressed
                else if(check_button_pressed(vz_incr_btn, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Speed Decreased");
                    refresh();
                    int fd_mz = open(mz_fifo, O_WRONLY);
                    if (fd_mz < 0) perror("Error opening cmd-mz fifo");
                    if (write(fd_mz, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mz fifo");
                    close(fd_mz);
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Vz stop button pressed
                else if(check_button_pressed(vz_stp_button, &event)) {
                    mvprintw(LINES - 1, 1, "Vertical Motor Stopped");
                    refresh();
                    int fd_mz = open(mz_fifo, O_WRONLY);
                    if (fd_mz < 0) perror("Error opening cmd-mz fifo");
                    if (write(fd_mz, stop, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-mz fifo");
                    close(fd_mz);
                    sleep(1);
                    for(int j = 0; j < COLS; j++) {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }

                // Send ALIVE signal to watchdog:
                int fd_watch = open(watch_fifo, O_WRONLY);
                if (fd_watch < 0) perror("Error opening watch-cmd fifo");
                if (write(fd_watch, inc, SIZE_MSG) != SIZE_MSG) perror("Error writing in cmd-watch fifo");
                close(fd_watch);
            }
        }

        refresh();
	}

    // Terminate
    endwin();
    return 0;
}
