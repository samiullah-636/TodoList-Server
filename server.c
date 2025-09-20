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
#include <sqlite3.h>
#include <openssl/sha.h>


#define PORT 8080
#define DB_FILE "Todo_Server.db"
#define HASH_HEX_LEN (SHA256_DIGEST_LENGTH*2+1)

// Function to store address and info of each client
struct sockaddr_in* createAddress() {
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    address->sin_port = htons(PORT);
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    return address;
}


void bytes_to_hex(const unsigned char *value, size_t valueLen, char *hexValue)
  {
static const char hex[] = "0123456789abcdef";
for (size_t i=0; i<valueLen; i++)
    {
      hexValue[i*2] = hex[(value[i] >> 4) & 0xF];
      hexValue[i*2 + 1] = hex[value[i] & 0xF];
    }
  hexValue[valueLen*2] = '\0';
  }
 

void sha256_hash(const char *password , char *passOutput)
  {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)password, 
    strlen(password), hash);
    bytes_to_hex(hash, SHA256_DIGEST_LENGTH, passOutput);
  }



sqlite3 *open_db(const char *filename){
sqlite3 *db = NULL;
if (sqlite3_open(filename , &db) != SQLITE_OK){
	fprintf(stderr, "Oops!, Can't open Database: %s\n", sqlite3_errmsg(db));
	sqlite3_close(db);
	return NULL;
}
return db;
} //open_db ends here

void close_db(sqlite3 *db)
{
if(db) sqlite3_close(db);
} //close_db ends here


int create_table(sqlite3 *db) {
    char *err_msg = 0;

    const char *sql_users =
        "CREATE TABLE IF NOT EXISTS Users ("
        "User_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "Username TEXT UNIQUE NOT NULL,"
        "Password TEXT NOT NULL"
        ");";

    const char *sql_tasks =
        "CREATE TABLE IF NOT EXISTS Tasks ("
        "Task_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "User_id INTEGER NOT NULL,"
        "Title TEXT NOT NULL,"
        "Description TEXT,"
        "Status TEXT DEFAULT 'pending',"
        "Deadline DATETIME,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(User_id) REFERENCES Users(User_id)"
        ");";

    int rc = sqlite3_exec(db, sql_users, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (Users): %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    rc = sqlite3_exec(db, sql_tasks, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error (Tasks): %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    return SQLITE_OK;
}


int insert_user(sqlite3 *db, const char *username, 
const char *password)
{
char *sql_insert = sqlite3_mprintf(
"INSERT INTO Users(Username, Password) VALUES ('%q', '%q');",username, password);
char *err = NULL;
int rc = sqlite3_exec(db, sql_insert, 0, 0, &err);
	if(rc != SQLITE_OK){
	fprintf(stderr, "ERROR INSERTING DATA :( %s\n",err);
	sqlite3_free(err);
	return rc;
	}
	return SQLITE_OK;
} // insert_user ends here


void send_to_client(char* message,int sock)
{
send(sock, message, strlen(message), 0);
}

bool user_exists(char* username){
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        printf("Error opening DB for writing.\n");
        return;
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
//if (user_exists(username))
//{
//send_to_client("Username Already Exists\n",sock);
//return;
//}
char hash_hex[HASH_HEX_LEN];
sha256_hash(password,hash_hex);
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        printf("Error opening DB for writing.\n");
        return;
    }
if (create_table(db) != SQLITE_OK)
{
	close_db(db);
        printf("Error creating tables.\n");
}
if (insert_user(db, username, hash_hex) != SQLITE_OK)
{
printf("Insert failed \n");
}
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
    printf("Client Connected :) \n");
    while (1) {
        // Wait for client to be ready (handshake ack for last round)
        memset(ack_buf, 0, sizeof(ack_buf));
        recv(clientSocketFD, ack_buf, sizeof(ack_buf), 0);

        // Send menu
        send_to_client("\n1. Register\n2. Login\n3. Exit\nEnter Your Choice: ", clientSocketFD);

        // Receive choice
        ssize_t bytes_recv = recv(clientSocketFD, &choice, sizeof(choice), 0);
        if (bytes_recv <= 0) {
            printf("Client disconnected unexpectedly.\n");
            break;
        }
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
