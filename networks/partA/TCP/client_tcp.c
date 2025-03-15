#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 9090

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        read(sock, buffer, sizeof(buffer));
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
            send(sock, response, strlen(response), 0);

            if (strncmp(response, "no", 2) == 0)
            {
                break; // Exit loop if player chooses not to play again
            }
            else
            {
                // Reset for new game
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
            send(sock, move, strlen(move), 0);
        }
    }

    printf("Game over. Closing connection.\n");
    close(sock);
    return 0;
}
