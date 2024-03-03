#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p2c[2];
    int c2p[2];

    pipe(p2c);
    pipe(c2p);
    
    write(p2c[1], "1", 1);  //write to child
    if (fork() == 0)
    {
        close(0);
        dup(p2c[0]);  //receive from parent
        int pid = getpid();
        printf("%d: received ping\n",pid);

        write(c2p[1], "1", 1);  //write to parent
    } else {
        wait(0);  //wait for child

        close(0);
        dup(c2p[0]);  //receive from child
        int pid = getpid();
        printf("%d: received pong\n",pid);
    }

    close(p2c[0]);
    close(p2c[1]);  //close write-end, prevent blocking in read-end
    close(c2p[0]);
    close(c2p[1]);

    exit(0);
}