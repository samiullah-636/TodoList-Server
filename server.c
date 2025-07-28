#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define PORT 8080
#define USER_FILE "users.txt"
// Function to store address and info of each client
struct sockaddr_in* createAddress() {
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    address->sin_port = htons(PORT);
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    return address;
}

void send_to_client(char* message,int sock)
{
send(sock, message, strlen(message), 0);
}

bool user_exists(char* username)
{
FILE* file = fopen(USER_FILE,"r");
if (!file)
{
printf("File Doesn't Exists");
return false;
}

char existing_user[1024],existing_pass[1024];

while (fscanf(file, "%s %s", existing_user, existing_pass) != EOF) {
        if (strcmp(existing_user, username) == 0) {
            fclose(file);
            return true;  // User already exists
        }
    }

    fclose(file);
    return false;
}
//void login(char* username, char* password, int socket) {
  //  if (!user_exists(username)) {
    //    send_to_client("❌ User not found",socket);
     //   return;
   // }

    //if (!verify_password(username, password)) {
      //  send_to_client("❌ Wrong password",socket);
       // return;
   // }

    //char token[64];
    //generate_token(token);
   // time_t expiry = time(NULL) + 1800;  // 30 min expiry
   // save_session(username, token, expiry);

    //char response[128];
    //sprintf(response, "LOGIN_SUCCESS token=%s", token);
    //send_to_client(response);
//}

void Register(char* username , char* password, int sock)
{
if (user_exists(username))
{
send_to_client("Username Already Exists\n",sock);
return;
}
FILE* file = fopen(USER_FILE, "a");
if (!file) {
        printf("Error opening user file for writing.\n");
        return;
    }
fprintf(file, "%s %s\n", username, password);  // For now storing plain password
    fclose(file);

    send_to_client("User registered successfully!\n",sock);
    return;
}
// Function to send menu and receive user choice
void* RecieveData(void *str) {
int clientSocketFD = atoi((char *)str);
    int choice;
    char username[1024];
    char password[1024];
    char ack_buf[8];

    while (1) {
        // Wait for client to be ready (handshake ack for last round)
        memset(ack_buf, 0, sizeof(ack_buf));
        recv(clientSocketFD, ack_buf, sizeof(ack_buf), 0);

        // Send menu
        send_to_client("\n1. Register\n2. Login\n3. Exit\nEnter Your Choice: ", clientSocketFD);

        // Receive choice
        ssize_t bytes_recv = recv(clientSocketFD, &choice, sizeof(choice), 0);
        printf("[DEBUG] CHOICE BYTES: %ld\n", bytes_recv);
        printf("[DEBUG] CHOICE: %d\n", choice);

        if (choice == 2) {
            ssize_t U_bytes_recv = recv(clientSocketFD, username, sizeof(username), 0);
            ssize_t P_bytes_recv = recv(clientSocketFD, password, sizeof(password), 0);
            printf("[DEBUG] Username Received: %s\n", username);
            printf("[DEBUG] Username BYTES Received: %ld\n", U_bytes_recv);
            printf("[DEBUG] Password Received: %s\n", password);
            printf("[DEBUG] Password BYTES Received: %ld\n", P_bytes_recv);

            // Send login message
            send_to_client("Login Successfully.\n", clientSocketFD);

        } else if (choice == 1) {
            ssize_t Ubytes_recv = recv(clientSocketFD, username, sizeof(username), 0);
            printf("[DEBUG] Username Received: %s\n", username);
            printf("[DEBUG] Username BYTES Received: %ld\n", Ubytes_recv);
            ssize_t Pbytes_recv = recv(clientSocketFD, password, sizeof(password), 0);
            printf("[DEBUG] Password Received: %s\n", password);
            printf("[DEBUG] Password BYTES Received: %ld\n", Pbytes_recv);
            Register(username,password,clientSocketFD);
            // Send registration message
           // send_to_client("Registration Done.\n", clientSocketFD);

        } else if (choice == 3) {
            printf("Client disconnected by choice.\n");
            close(clientSocketFD);
            break;
        } else {
            send_to_client("Invalid option. Try again.\n", clientSocketFD);
        }
    }
}

// Function to create a new thread to handle client communication
void CreatingThread(int clientSocketFD) {
    char str[20];
    sprintf(str, "%d", clientSocketFD); 
    pthread_t thread;
    pthread_create(&thread, NULL, RecieveData, str); // Create a thread for each client
}

// Accepting connections from clients
void acceptingConnections(int serverSocketFD) {
    while (1) {
        struct sockaddr_in clientAddress;
        int clientAddressSize = sizeof(struct sockaddr_in);
        int clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, &clientAddressSize);

        if (clientSocketFD < 0) {
            printf("Client connection failed\n");
            continue;
        }

        // Create a new thread to handle the client
        CreatingThread(clientSocketFD);
    }
}

// Main function
int main() {
    int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFD < 0) {
        printf("Socket creation failed\n");
        exit(1);
    }

    struct sockaddr_in *serveraddress = createAddress();

    // Binding server
    int result = bind(serverSocketFD, (struct sockaddr *)serveraddress, sizeof(*serveraddress));
    if (result < 0) {
        printf("Bind failed\n");
        close(serverSocketFD);
        exit(1);
    }

    printf("Server listening on port %d\n", PORT);

    // Listening for clients
    int listenResult = listen(serverSocketFD, 10);  // Max 10 clients
    if (listenResult < 0) {
        printf("Listen failed\n");
        close(serverSocketFD);
        exit(1);
    }

    // Accept and handle client connections
    acceptingConnections(serverSocketFD);

    free(serveraddress);
    close(serverSocketFD);
}
