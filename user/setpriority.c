#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("setpriority: insufficient arguments\n");
        exit(1);
    }

    char *priority_str = argv[1];
    char *pid_str = argv[2];

    for (int i = 0; i < strlen(priority_str); i++)
    {
        if (priority_str[i] >= '0' && priority_str[i] <= '9')
        {
            // alright;
        }
        else
        {
            printf("setpriority: Invalid priority argument\n");
            exit(1);
        }
    }
    for (int i = 0; i < strlen(pid_str); i++)
    {
        if (pid_str[i] >= '0' && pid_str[i] <= '9')
        {
            // alright;
        }
        else
        {
            printf("setpriority: Invalid pid argument\n");
            exit(1);
        }
    }

    int int_priority = atoi(priority_str);
    int int_pid = atoi(pid_str);

    if ((int_priority < 0) || (int_priority > 100))
    {
        printf("setpriority: Invalid priority range\n");
        exit(1);
    }

    int pid = fork();
    if (pid == -1)
    {
        printf("setpriority: Error forking a child\n");
        exit(1);
    }

    if (pid == 0)
    {
        int x = set_priority(int_priority, int_pid);
        if (x == -1)
        {
            printf("setpriority: Process with pid %d not found\n", int_pid);
        }
        exit(1);
    }
    wait(0);
    exit(0);
}