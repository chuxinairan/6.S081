#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define BUFFER_SIZE 10
#define MAX_ARG_SIZE 10

void run_program(char *argv[])
{
    if (fork() == 0)
    {
        if (exec(argv[0], argv) < 0)
        {
            fprintf(2, "xargs: too many arguments!!!");
        }
        exit(0);
    }
    else
    {
        wait(0);
    }
}

int main(int argc, char *argv[])
{
    char *arg_list[MAXARG];
    int current_arg_number = 0;
    for (int i = 1; i < argc; i++)
    {
        if (i == MAXARG)
        {
            fprintf(2, "xargs: too many arguments!!!");
        }
        arg_list[i - 1] = argv[i];
        current_arg_number++;
    }

    char buf[BUFFER_SIZE]; // sliding window
    char *buf_head;        // point to first argument inside window
    char *arg_head;        // point to the first char of every arguments
    char *read_head = buf; // comtinue writing since last read
    int arg_number = current_arg_number;
    while (1)
    {
        buf_head = buf;
        int count_read = read(0, read_head, BUFFER_SIZE);

        if (count_read == 0)
        {
            // if (read_head != buf)
            // {
            //     *read_head = 0;
            //     arg_list[arg_number++] = buf;
            //     arg_list[arg_number + 1] = 0;
            //     run_program(arg_list);
            // }
            break;
        }

        arg_head = buf_head;
        for (char *p = buf_head; *p != 0; p++)
        {
            if (*p == '\n')
            {
                if (arg_number == MAXARG)
                {
                    fprintf(2, "xargs: too many arguments!!!");
                }
                *p = 0;
                char *new_arg = malloc(sizeof(char) * (strlen(arg_head) + 1));
                new_arg[strlen(arg_head)] = 0;
                strcpy(new_arg, arg_head);
                arg_list[arg_number++] = new_arg;
                arg_list[arg_number + 1] = 0;
                run_program(arg_list);
                for(int j = current_arg_number; j < arg_number; j++)
                {
                    free(arg_list[j]);
                }
                arg_number = current_arg_number;
                arg_head = p + 1;
            }
            if (*p == ' ')
            {
                *p = 0;
                char *new_arg = malloc(sizeof(char) * (strlen(arg_head) + 1));
                new_arg[strlen(arg_head)] = 0;
                strcpy(new_arg, arg_head);
                arg_list[arg_number++] = new_arg;
                arg_head = p + 1;
            }
        }

        while (*arg_head != 0) // if remain chars without adding to list, put to the buffer head
        {
            *buf_head = *arg_head;
            buf_head++;
            arg_head++;
        }
        read_head = buf_head;
    }

    exit(0);
}