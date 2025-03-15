#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 65432
#define BUFFER_SIZE 1024
#define CHUNK_SIZE 3

typedef struct
{
    int seq_num;
    int total_chunks;
    char data[CHUNK_SIZE + 1];
} DataChunk;

void *receive_data(void *socket_desc)
{
    int sock = *(int *)socket_desc;
    DataChunk chunk;
    char *received_message = NULL;
    int received_chunks = 0;

    while (1)
    {
        int bytes_received = recv(sock, &chunk, sizeof(chunk), 0);
        if (bytes_received <= 0)
        {
            printf("Server disconnected.\n");
            break;
        }

        if (received_message == NULL)
        {
            received_message = malloc(chunk.total_chunks * CHUNK_SIZE + 1);
            memset(received_message, 0, chunk.total_chunks * CHUNK_SIZE + 1);
        }

        memcpy(received_message + (chunk.seq_num - 1) * CHUNK_SIZE, chunk.data, CHUNK_SIZE);
        received_chunks++;

        printf("Received chunk %d/%d: %s\n", chunk.seq_num, chunk.total_chunks, chunk.data);

        if (received_chunks == chunk.total_chunks)
        {
            printf("Complete message: %s\n", received_message);
            free(received_message);
            received_message = NULL;
            received_chunks = 0;
        }
    }
    close(sock);
    return NULL;
}

void start_client()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }

    printf("Client connected to the server.\n");

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_data, (void *)&sock);

    while (1)
    {
        char buffer[BUFFER_SIZE];
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        int len = strlen(buffer);
        int total_chunks = (len + CHUNK_SIZE - 1) / CHUNK_SIZE;
        if (strcmp(buffer, "/exit") == 0)
        {
            break;
        }
        for (int i = 0; i < total_chunks; i++)
        {
            DataChunk chunk;
            chunk.seq_num = i + 1;
            chunk.total_chunks = total_chunks;
            strncpy(chunk.data, buffer + i * CHUNK_SIZE, CHUNK_SIZE);
            chunk.data[CHUNK_SIZE] = '\0';

            // if ((i + 1) % 3 == 0)
            // {
            //     printf("Skipping chunk %d/%d (not sending)\n", chunk.seq_num, chunk.total_chunks);
            //     continue;
            // }

            send(sock, &chunk, sizeof(chunk), 0);
        }

        // for (int i = 0; i < total_chunks; i++)
        // {
        //     if ((i + 1) % 3 == 0)
        //     {
        //         DataChunk chunk;
        //         chunk.seq_num = i + 1;
        //         chunk.total_chunks = total_chunks;
        //         strncpy(chunk.data, buffer + i * CHUNK_SIZE, CHUNK_SIZE);
        //         chunk.data[CHUNK_SIZE] = '\0';

        //         send(sock, &chunk, sizeof(chunk), 0);
        //         printf("Retransmitted chunk %d/%d: %s\n", chunk.seq_num, chunk.total_chunks, chunk.data);
        //         sleep(1);
        //     }
        // }
        if (strcmp(buffer, "/exit") == 0)
        {
            break;
        }
    }

    close(sock);
}

int main()
{
    start_client();
    return 0;
}
