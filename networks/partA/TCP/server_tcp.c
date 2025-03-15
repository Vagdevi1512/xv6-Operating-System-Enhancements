#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>

#define PORT 9090

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

void print_board()
{
    printf(" %c | %c | %c\n", board[0][0], board[0][1], board[0][2]);
    // printf("-----------\n");
    printf(" %c | %c | %c\n", board[1][0], board[1][1], board[1][2]);
    // printf("-----------\n");
    printf(" %c | %c | %c\n", board[2][0], board[2][1], board[2][2]);
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

void send_board_to_clients(int client1, int client2)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%c|%c|%c\n%c|%c|%c\n%c|%c|%c\n",
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);
    send(client1, buffer, sizeof(buffer), 0);
    send(client2, buffer, sizeof(buffer), 0);
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

void handle_client_turn(int client, char symbol, int opponent)
{
    int row, col;
    char buffer[1024];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        recv(client, buffer, sizeof(buffer), 0);

        if (sscanf(buffer, "%d %d", &row, &col) == 2 && is_valid_move(row - 1, col - 1))
        {
            update_board(row - 1, col - 1, symbol);
            print_board_to_server(); // Print the board to the server
            break;                   // Exit the loop on valid move
        }
        else
        {
            send(client, "Invalid move, try again.\n", 1024, 0);
            send(client, "Your turn, ", 1024, 0);
        }
    }
}

void handle_play_again(int client1, int client2)
{
    char buffer1[1024], buffer2[1024];
    int play1 = 0, play2 = 0;

    // Ask player 1 if they want to play again
    send(client1, "Do you want to play again? (yes or no): ", 1024, 0);
    recv(client1, buffer1, sizeof(buffer1), 0);

    // Ask player 2 if they want to play again
    send(client2, "Do you want to play again? (yes or no): ", 1024, 0);
    recv(client2, buffer2, sizeof(buffer2), 0);

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
        send(client1, "One player chose not to play again. Game over.\n", 1024, 0);
        send(client2, "One player chose not to play again. Game over.\n", 1024, 0);
        close(client1);
        close(client2);
        exit(0); // Exit the server process
    }
    else
    {
        reset_board(); // Reset the board for a new game if both say "yes"
    }
}

int main()
{
    int server_fd, client1, client2;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1; // Option for setting SO_REUSEADDR

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    if ((client1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    printf("Player 1 connected.\n");

    if ((client2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    printf("Player 2 connected.\n");

    reset_board();

    int winner = 0;

    while (1)
    { // Infinite loop to allow for replay
        while (winner == 0)
        {
            send_board_to_clients(client1, client2);

            if (player_turn == 1)
            {
                send(client1, "Your turn (X): ", 1024, 0);
                send(client2, "Waiting for Player 1 (X)...\n", 1024, 0);
                handle_client_turn(client1, 'X', client2);
            }
            else
            {
                send(client2, "Your turn (O): ", 1024, 0);
                send(client1, "Waiting for Player 2 (O)...\n", 1024, 0);
                handle_client_turn(client2, 'O', client1);
            }

            winner = check_winner();
            player_turn = (player_turn == 1) ? 2 : 1;
        }

        send_board_to_clients(client1, client2);

        if (winner == 1)
        {
            send(client1, "Player 1 Wins!\n", 1024, 0);
            send(client2, "Player 1 Wins!\n", 1024, 0);
        }
        else if (winner == 2)
        {
            send(client1, "Player 2 Wins!\n", 1024, 0);
            send(client2, "Player 2 Wins!\n", 1024, 0);
        }
        else
        {
            send(client1, "It's a Draw!\n", 1024, 0);
            send(client2, "It's a Draw!\n", 1024, 0);
        }

        // Ask if they want to play again
        handle_play_again(client1, client2);

        winner = 0; // Reset winner for the next game
    }

    return 0;
}
