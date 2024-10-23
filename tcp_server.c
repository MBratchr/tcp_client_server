#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "utils.h"
#include <math.h>

#define numExternals 4 // Number of external processes
#define THRESHOLD 0.0001 // limit of convergence; its unnecessary to get any more specific

// Function to establish connections from external processes
int* establishConnectionsFromExternalProcesses() {
    int socket_desc;
    static int client_socket[numExternals];
    unsigned int client_size;
    struct sockaddr_in server_addr, client_addr;

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0) {
        printf("Error while creating socket\n");
        exit(0);
    }
    printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Couldn't bind to the port\n");
        exit(0);
    }
    printf("Done with binding\n");

    // Listen for clients:
    if(listen(socket_desc, 1) < 0) {
        printf("Error while listening\n");
        exit(0);
    }
    printf("\n\nListening for incoming connections.....\n\n");

    // Accept connections from all external processes
    int externalCount = 0;
    while(externalCount < numExternals) {
        client_socket[externalCount] = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
        if(client_socket[externalCount] < 0) {
            printf("Can't accept\n");
            exit(0);
        }
        printf("One external process connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        externalCount++;
    }
    printf("All external processes are now connected\n");
    return client_socket;
}

int main(void) {
    struct msg messageFromClient;
    int *client_socket = establishConnectionsFromExternalProcesses();
    float centralTemp = 0.0;  // Initial central temperature
    float previousCentralTemp = centralTemp;
    int stable = false;

    while (!stable) {
        float externalTemps[numExternals];

        // Receive temps from external processes
        printf("--------------------------------------------------------------------------\n");
        for(int i = 0; i < numExternals; i++) {
            if(recv(client_socket[i], (void *)&messageFromClient, sizeof(messageFromClient), 0) < 0) {
                printf("Couldn't receive\n");
                return -1;
            }
            externalTemps[i] = messageFromClient.T;
            printf("Temperature of External Process (%d) = %f\n", i, externalTemps[i]);
        }

        // Calculate new central temp
        float sumExternalTemps = 0.0;
        for(int i = 0; i < numExternals; i++) {
            sumExternalTemps += externalTemps[i];
        }
        centralTemp = (2.0 * centralTemp + sumExternalTemps) / 6.0;
        printf("New central temperature = %f\n", centralTemp);
        printf("--------------------------------------------------------------------------\n");

        // Send updated central temp to external processes
        struct msg updated_msg;
        updated_msg.T = centralTemp;
        for(int i = 0; i < numExternals; i++) {
            if (send(client_socket[i], (const void *)&updated_msg, sizeof(updated_msg), 0) < 0) {
                printf("Can't send\n");
                return -1;
            }
        }

        // Check if temp has stabilized
        if(fabs(centralTemp - previousCentralTemp) < THRESHOLD) {
            stable = true;
            printf("//////////////////////////////////////////////////////////////////////////\n");
            printf("System has stabilized! Stabilized temp: %f\n", centralTemp);
            printf("//////////////////////////////////////////////////////////////////////////\n");

        } else {
            previousCentralTemp = centralTemp;
        }
    }

    // Closing all sockets
    for(int i = 0; i < numExternals; i++) {
        close(client_socket[i]);
    }
    return 0;
}
