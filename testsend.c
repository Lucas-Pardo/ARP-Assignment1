#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>

int main(int argc, char ** argv) {

    int cmd, pid;

    sscanf(argv[1], "%d", &pid);


    while((cmd = getchar()) != 'q') {

        switch (cmd)
        {
        case '1':
            printf("Sending SIGUSR1.\n");
            kill(pid, SIGUSR1);
            break;
        case '2':
            printf("Sending SIGUSR2.\n");
            kill(pid, SIGUSR2);
            break;        
        default:
            break;
        }

    }
    kill(pid, SIGINT);
    return 0;
}