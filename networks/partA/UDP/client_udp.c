#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t addr_len = sizeof(serv_addr);
    char buffer[1024] = {0};

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Send a message to "connect" (just a ping to the server)
    sendto(sockfd, NULL, 0, 0, (struct sockaddr *)&serv_addr, addr_len);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&serv_addr, &addr_len);
        printf("%s", buffer);

        if (strstr(buffer, "Wins") || strstr(buffer, "Draw"))
        {
            continue; // Don't request a move after the game ends.
        }
        if (strstr(buffer, "Game over"))
        {
            break; // Exit the loop when the game is over
        }

        if (strstr(buffer, "Do you want to play again?"))
        {
            char response[4];
            printf("Enter 'yes' or 'no': ");
            scanf("%s", response);
            sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&serv_addr, addr_len);

            if (strncmp(response, "no", 2) == 0)
            {
                break; // Exit loop if player chooses not to play again
            }
            else
            {
                printf("New game will start soon.\n");
                continue; // Wait for the next game setup from the server
            }
        }

        if (strstr(buffer, "Your turn"))
        {
            int row, col;
            printf("Enter your move (row and col): ");
            scanf("%d %d", &row, &col);

            char move[10];
            snprintf(move, sizeof(move), "%d %d", row, col);
            sendto(sockfd, move, strlen(move), 0, (struct sockaddr *)&serv_addr, addr_len);
        }
    }

    printf("Game over. Closing connection.\n");
    close(sockfd);
    return 0;
}
