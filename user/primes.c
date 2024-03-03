#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void create_right(int p[])
{
    if (fork() == 0) {

        int process_prime;
        int number_to_test;
        int pipe_to_child[2];
        int hasChild = 0;

        //close useless file descriptors
        close(p[1]);

        //get prime from parent
        read(p[0], &process_prime, 4);
        printf("prime %d\n", process_prime); 
        
        
        pipe(pipe_to_child);
        read(p[0], &number_to_test, 4);
        if (number_to_test % process_prime != 0) 
        {
            hasChild = 1;
            write(pipe_to_child[1], &number_to_test, 4);
            create_right(pipe_to_child);
        }
        
        //wait data 
        while (read(p[0], &number_to_test, 4))  
        {  
            if (number_to_test % process_prime != 0) 
            {
                write(pipe_to_child[1], &number_to_test, 4);
            }
        }

        close(pipe_to_child[1]);   //close the write-end to child
        if (hasChild == 1) {
            wait(0);
        } else {
            close(pipe_to_child[0]);
        }
        close(p[0]);
        exit(0);
    } 
}

int main(int argc, char *argv[])
{
    int p[2];
    int prime_2 = 2;
    pipe(p);
    
    write(p[1], &prime_2, 4);
    create_right(p);

    close(p[0]);
    for (int i=3; i <= 35; i++) {
        write(p[1], &i, 4);
    }
    close(p[1]);  //close write-end to show no data can send
    wait(0);
    exit(0);
}