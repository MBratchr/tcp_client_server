#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.h"
#include <math.h>

int main(int argc, char *argv[]) {
    int socket_desc;
    struct sockaddr_in server_addr;
    struct msg the_message;
    
    // Command-line input arguments (user provided)
    int externalIndex = atoi(argv[1]);
    float externalTemp = atof(argv[2]);

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        printf("Unable to create socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Send connection request to server:
    if (connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");

    while (1) {
        // Send initial temperature to the server:
        the_message = prepare_message(externalIndex, externalTemp);
        if (send(socket_desc, (const void *)&the_message, sizeof(the_message), 0) < 0) {
            printf("Unable to send message\n");
            return -1;
        }

        // Receive updated temperature from the central process:
        if (recv(socket_desc, (void *)&the_message, sizeof(the_message), 0) < 0) {
            printf("Error while receiving server's msg\n");
            return -1;
        }
        printf("Updated central temperature = %f\n", the_message.T);

        // Recalculate external temperature
        externalTemp = (3.0 * externalTemp + 2.0 * the_message.T) / 5.0;
        printf("Updated external temperature = %f\n", externalTemp);
    }

    // Close the socket:
    close(socket_desc);
    return 0;
}
