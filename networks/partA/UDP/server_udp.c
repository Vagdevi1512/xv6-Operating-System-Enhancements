#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>

#define PORT 8080

char board[3][3];
int player_turn = 1;

void reset_board()
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            board[i][j] = ' ';
        }
    }
}

void print_board_to_server()
{
    printf("Current Board State:\n");
    for (int i = 0; i < 3; i++)
    {
        printf(" %c | %c | %c\n", board[i][0], board[i][1], board[i][2]);
        if (i < 2)
        {
            printf("-----------\n");
        }
    }
}

void send_board_to_clients(int sockfd, struct sockaddr_in *addr1, struct sockaddr_in *addr2, socklen_t addr_len)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%c|%c|%c\n%c|%c|%c\n%c|%c|%c\n",
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);
    sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)addr1, addr_len);
    sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)addr2, addr_len);
}

int check_winner()
{
    for (int i = 0; i < 3; i++)
    {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return (board[i][0] == 'X') ? 1 : 2;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return (board[0][i] == 'X') ? 1 : 2;
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return (board[0][0] == 'X') ? 1 : 2;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return (board[0][2] == 'X') ? 1 : 2;

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (board[i][j] == ' ')
                return 0; // No winner yet
        }
    }
    return 3; // Draw
}

int is_valid_move(int row, int col)
{
    return row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ';
}

void update_board(int row, int col, char symbol)
{
    board[row][col] = symbol;
}

void handle_client_turn(int sockfd, struct sockaddr_in *client_addr, char symbol, struct sockaddr_in *opponent_addr, socklen_t addr_len)
{
    int row, col;
    char buffer[1024];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)client_addr, &addr_len);

        if (sscanf(buffer, "%d %d", &row, &col) == 2 && is_valid_move(row - 1, col - 1))
        {
            update_board(row - 1, col - 1, symbol);
            print_board_to_server(); // Print the board to the server
            break;                   // Exit the loop on valid move
        }
        else
        {
            sendto(sockfd, "Invalid move, try again.\n", 1024, 0, (struct sockaddr *)client_addr, addr_len);
            sendto(sockfd, "Your turn, ", 1024, 0, (struct sockaddr *)client_addr, addr_len);
        }
    }
}

void handle_play_again(int sockfd, struct sockaddr_in *client1_addr, struct sockaddr_in *client2_addr, socklen_t addr_len)
{
    char buffer1[1024], buffer2[1024];
    int play1 = 0, play2 = 0;

    // Ask player 1 if they want to play again
    sendto(sockfd, "Do you want to play again? (yes or no): ", 1024, 0, (struct sockaddr *)client1_addr, addr_len);
    recvfrom(sockfd, buffer1, sizeof(buffer1), 0, (struct sockaddr *)client1_addr, &addr_len);

    // Ask player 2 if they want to play again
    sendto(sockfd, "Do you want to play again? (yes or no): ", 1024, 0, (struct sockaddr *)client2_addr, addr_len);
    recvfrom(sockfd, buffer2, sizeof(buffer2), 0, (struct sockaddr *)client2_addr, &addr_len);

    // Convert responses to lowercase for easier comparison
    for (int i = 0; buffer1[i]; i++)
        buffer1[i] = tolower(buffer1[i]);
    for (int i = 0; buffer2[i]; i++)
        buffer2[i] = tolower(buffer2[i]);

    // Check responses from both players
    if (strstr(buffer1, "yes") != NULL)
        play1 = 1;
    if (strstr(buffer2, "yes") != NULL)
        play2 = 1;

    // If either player chooses 'no', both players exit
    if (play1 == 0 || play2 == 0)
    {
        sendto(sockfd, "One player chose not to play again. Game over.\n", 1024, 0, (struct sockaddr *)client1_addr, addr_len);
        sendto(sockfd, "One player chose not to play again. Game over.\n", 1024, 0, (struct sockaddr *)client2_addr, addr_len);
        close(sockfd);
        exit(0); // Exit the server process
    }
    else
    {
        reset_board(); // Reset the board for a new game if both say "yes"
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client1_addr, client2_addr;
    socklen_t addr_len = sizeof(server_addr);
    int winner = 0;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Setup the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Receive first player connection
    recvfrom(sockfd, NULL, 0, 0, (struct sockaddr *)&client1_addr, &addr_len);
    printf("Player 1 connected.\n");

    // Receive second player connection
    recvfrom(sockfd, NULL, 0, 0, (struct sockaddr *)&client2_addr, &addr_len);
    printf("Player 2 connected.\n");

    reset_board();

    while (1)
    {
        while (winner == 0)
        {
            send_board_to_clients(sockfd, &client1_addr, &client2_addr, addr_len);

            if (player_turn == 1)
            {
                sendto(sockfd, "Your turn (X): ", 1024, 0, (struct sockaddr *)&client1_addr, addr_len);
                sendto(sockfd, "Waiting for Player 1 (X)...\n", 1024, 0, (struct sockaddr *)&client2_addr, addr_len);
                handle_client_turn(sockfd, &client1_addr, 'X', &client2_addr, addr_len);
            }
            else
            {
                sendto(sockfd, "Your turn (O): ", 1024, 0, (struct sockaddr *)&client2_addr, addr_len);
                sendto(sockfd, "Waiting for Player 2 (O)...\n", 1024, 0, (struct sockaddr *)&client1_addr, addr_len);
                handle_client_turn(sockfd, &client2_addr, 'O', &client1_addr, addr_len);
            }

            winner = check_winner();
            player_turn = (player_turn == 1) ? 2 : 1;
        }

        send_board_to_clients(sockfd, &client1_addr, &client2_addr, addr_len);

        if (winner == 1)
        {
            sendto(sockfd, "Player 1 Wins!\n", 1024, 0, (struct sockaddr *)&client1_addr, addr_len);
            sendto(sockfd, "Player 1 Wins!\n", 1024, 0, (struct sockaddr *)&client2_addr, addr_len);
        }
        else if (winner == 2)
        {
            sendto(sockfd, "Player 2 Wins!\n", 1024, 0, (struct sockaddr *)&client1_addr, addr_len);
            sendto(sockfd, "Player 2 Wins!\n", 1024, 0, (struct sockaddr *)&client2_addr, addr_len);
        }
        else
        {
            sendto(sockfd, "It's a Draw!\n", 1024, 0, (struct sockaddr *)&client1_addr, addr_len);
            sendto(sockfd, "It's a Draw!\n", 1024, 0, (struct sockaddr *)&client2_addr, addr_len);
        }

        handle_play_again(sockfd, &client1_addr, &client2_addr, addr_len);

        winner = 0; // Reset for next game
    }

    return 0;
}
