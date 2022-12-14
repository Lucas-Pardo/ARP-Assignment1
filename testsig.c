#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>

int reset = 0;

// Current velocity:
float v = 0;

void signal_handler(int sig) {
    if (sig == SIGUSR1) { 
        printf("Stopped.\n");
        reset = 0;
        v = 0;
    } else if (sig == SIGUSR2) {
        printf("Reseting position.\n");
        reset = 1;
    }
}

int main(int argc, char **argv){

    // Signal handler:
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &signal_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) printf("Could not catch SIGUSR1\n");
    if (sigaction(SIGUSR2, &sa, NULL) < 0) printf("Could not catch SIGUSR2\n");

    while(1) {
        printf("Waiting for signal.\n");
        pause();
        printf("Got signal.\n");
    }
    printf("After the while loop.\n");
    return 0;
}