#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/syscallcount.h"
#include "user.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: syscount <mask> <command> [args...]\n");
        exit(0);
    }

    int mask = atoi(argv[1]); // Convert mask argument to integer
    getSysCount(mask);

    int pid = fork();
    if (pid < 0)
    {
        printf("Fork failed.\n");
        exit(0);
    }
    else if (pid == 0)
    {
        // Child process: Execute the specified command
        exec(argv[2], &argv[2]);
        printf("Exec failed.\n");
        exit(0);
    }
    else
    {
        // Parent process: Wait for the child to finishat
        int status;
        wait(&status);

        // Get the total syscall count for the parent and child processes
        int count = getSysCount(mask);

        // List of syscall names for printing
        char *syscall_names[] = {
            "fork", "exit", "wait", "pipe", "read", "kill", "exec", "fstat",
            "chdir", "dup", "getpid", "sbrk", "sleep", "uptime", "open", "write",
            "mknod", "unlink", "link", "mkdir", "close"};

        // Identify the syscall name from the mask
        int syscall_index = 0;
        for (int i = 0; i < 31; i++)
        {
            if (mask & (1 << i))
            {
                syscall_index = i;
                break;
            }
        }

        // Output the result
        printf("PID %d called %s %d times.\n", pid, syscall_names[syscall_index - 1], count);
    }

    exit(0);
}
