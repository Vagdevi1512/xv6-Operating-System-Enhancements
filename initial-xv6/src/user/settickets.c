// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user.h"

// int main() {
//     uint64 newTicketNum = settickets();
//     if (newTicketNum == -1) {
//         fprintf(2, "could not change tickets to 2 for process with pid %d\n", getpid());
//     }
//     // Continue with the rest of the program...
//     printf("%d\n", newTicketNum);
//     exit(0);
// }

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
    // Default number of tickets
    uint64 numTickets = 1;

    // Check if an argument is provided
    if (argc > 1) {
        numTickets = atoi(argv[1]); // Convert argument to integer
    }

    // Set the number of tickets
    uint64 newTicketNum = settickets(numTickets);
    if (newTicketNum == -1) {
        fprintf(2, "could not change tickets to %d for process with pid %d\n", numTickets, getpid());
    } else {
        fprintf(1, "successfully changed tickets to %d for process with pid %d\n", newTicketNum, getpid());
    }

    // Continue with the rest of the program...
    // You can perform additional tasks here as needed

    exit(0);
}
