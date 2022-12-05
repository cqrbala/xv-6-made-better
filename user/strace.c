#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("strace: wrong arguments\n");
        exit(1);
    }

    char *mask = argv[1];

    for (int i = 0; i < strlen(mask); i++)
    {
        if (mask[i] >= '0' && mask[i] <= '9')
        {
            // alright;
        }
        else
        {
            printf("strace: Invalid integer mask argument\n");
            exit(1);
        }
    }

    int integer_mask = atoi(mask);

    int pid = fork();
    if (pid == -1)
    {
        printf("strace: Error forking a child to run command\n");
        exit(1);
    }

    if (pid == 0)
    {
        trace(integer_mask);

        exec(argv[2], argv + 2);
        printf("strace: exec %s failed\n", argv[2]);
        exit(1);
    }
    wait(0);
    exit(0);
}