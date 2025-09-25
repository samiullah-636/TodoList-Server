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
void send_to_client(char* message,int sock)
{
send(sock, message, strlen(message), 0);
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
        "Status INTEGER ,"
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


int Add_tasks(int sock , int userid)
{
	char title[256];
	recv(sock, title, sizeof(title),0);

sqlite3 *db = open_db(DB_FILE);
if (!db) {
        fprintf(stderr,"Error opening DB.%s\n", sqlite3_errmsg(db));
        return 1;
   	 }


char *sql_insert = sqlite3_mprintf(
"INSERT INTO Tasks(User_id, Title, Status) VALUES ('%d', '%q','%d');",userid, title, 0);
char *err = NULL;
int rc = sqlite3_exec(db, sql_insert, 0, 0, &err);
	if(rc != SQLITE_OK){
	fprintf(stderr, "ERROR INSERTING TASK %s\n",err);
	sqlite3_free(err);
	sqlite3_free(sql_insert);
	close_db(db);
	return rc;
	}
	send_to_client("Task Inserted \n",sock);
	sqlite3_free(err);
	sqlite3_free(sql_insert);
	close_db(db);
	return SQLITE_OK;
} // Add_tasks ends here

void ListTasks(int sock, int userid)
{
char* status;
char Buffer[256];
char buffer[4096];
Buffer[0] = '\0';
buffer[0] = '\0';
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        fprintf(stderr,"Error opening DB.%s\n", sqlite3_errmsg(db));
   	 }
sqlite3_stmt *stmt;
const char *sql = "SELECT Task_id, Title, Status FROM Tasks WHERE User_id = ?;";
int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
if (rc != SQLITE_OK)
{
fprintf(stderr, "can't read from DB: %s\n", sqlite3_errmsg(db));
}
sqlite3_bind_int(stmt, 1, userid);
snprintf(Buffer,sizeof(Buffer),"%-5s%-30s%s\n", "Id", "Tasks", "Status");
strcat(Buffer, "----------------------------------------------------------\n");
send_to_client(Buffer,sock);
while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
{
int id = sqlite3_column_int(stmt, 0);
char* Task = (char *)sqlite3_column_text(stmt, 1);
if(!Task)
{
Task = "NULL";
}
int state = sqlite3_column_int(stmt, 2);
if(state == 0)
{
status = "Pending";
}
else
{
	status = "Done";
}	
char temp[256];
snprintf(temp, sizeof(temp), "%-5d%-30s%s\n", id, Task, status);
strcat(buffer, temp);
strcat(buffer, "----------------------------------------------------------\n");
}
send_to_client(buffer, sock);
	sqlite3_finalize(stmt);
    close_db(db);
} //ListTasks ends here.

void RemoveTask(int sock)
{
int taskid;
recv(sock, &taskid, sizeof(taskid), 0);
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        fprintf(stderr,"Error opening DB.%s\n", sqlite3_errmsg(db));
   	 }


char *sql = sqlite3_mprintf(
"DELETE FROM Tasks WHERE Task_id = '%d';",taskid);
char *err = NULL;
int rc = sqlite3_exec(db, sql, 0, 0, &err);
	if(rc != SQLITE_OK){
	fprintf(stderr, "ERROR REMOVING TASK %s\n",err);
	sqlite3_free(err);
	sqlite3_free(sql);
	close_db(db);
	}
	send_to_client("Task Removed \n",sock);
	sqlite3_free(err);
	sqlite3_free(sql);
	close_db(db);

}
void Mark_Status(int sock)
{
int taskid;
recv(sock, &taskid, sizeof(taskid), 0);
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        fprintf(stderr,"Error opening DB.%s\n", sqlite3_errmsg(db));
   	 }


char *sql = sqlite3_mprintf(
"UPDATE Tasks SET Status = '%d' WHERE Task_id = '%d';",1,taskid);
char *err = NULL;
int rc = sqlite3_exec(db, sql, 0, 0, &err);
	if(rc != SQLITE_OK){
	fprintf(stderr, "ERROR MARKING COMPLETE %s\n",err);
	sqlite3_free(err);
	sqlite3_free(sql);
	close_db(db);
	}
	send_to_client("Marked Done \n",sock);
	sqlite3_free(err);
	sqlite3_free(sql);
	close_db(db);


}
void Edit_Task(int sock)
{
int taskid;
char updated_task[256];
recv(sock, updated_task, sizeof(updated_task), 0);
recv(sock, &taskid, sizeof(taskid), 0);
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        fprintf(stderr,"Error opening DB.%s\n", sqlite3_errmsg(db));
   	 }


char *sql = sqlite3_mprintf(
"UPDATE Tasks SET Title = '%q' WHERE Task_id = '%d';",updated_task,taskid);
char *err = NULL;
int rc = sqlite3_exec(db, sql, 0, 0, &err);
	if(rc != SQLITE_OK){
	fprintf(stderr, "ERROR EDITING TASK %s\n",err);
	sqlite3_free(err);
	sqlite3_free(sql);
	close_db(db);
	}
	send_to_client("Operation Successful \n",sock);
	sqlite3_free(err);
	sqlite3_free(sql);
	close_db(db);


}
void Todo_Menu(int sock,int userid)
{
int choice;
char ack_buf[8];
while(1)
{
memset(ack_buf, 0, sizeof(ack_buf));
recv(sock, ack_buf, sizeof(ack_buf), 0);
send_to_client("[1].Add  [2].List  [3].Complete  [4].Edit  [5].Remove  [6].Logout \n> ",sock);
ssize_t bytes_recv = recv(sock, &choice, sizeof(choice), 0);
        if (bytes_recv <= 0) {
            printf("Client disconnected unexpectedly.\n");
            break;
        }
switch (choice)
   { 
   case 1:
	Add_tasks(sock, userid);
	break;
   case 2:
	ListTasks(sock, userid);
	break;
   case 3:
	Mark_Status(sock);
	break;
   case 4:
	Edit_Task(sock);
	break;
   case 5:
	RemoveTask(sock);
	break;
   case 6:
	return;
   default:
	send_to_client("Invalid option :(\n",sock);
	break;
   }
}
}
bool user_exists(char* username){
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        fprintf(stderr,"Error opening DB.%s\n", sqlite3_errmsg(db));
        return 1;
   	 }
sqlite3_stmt *stmt;
const char *sql = "SELECT username FROM Users;";
int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
if (rc != SQLITE_OK)
{
fprintf(stderr, "can't read username from DB: %s\n", sqlite3_errmsg(db));
return 1;
}
while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
{
	char *existing_username = (char *)sqlite3_column_text(stmt, 0);
	if (strcmp(existing_username, username) == 0)
	{
		sqlite3_finalize(stmt);
	close_db(db);
	return true;
	}
}
    close_db(db);
    return false;
} //User exists function ends here

bool verify_password(char* username, char* password)
{
char hash_hex[HASH_HEX_LEN];
sha256_hash(password,hash_hex);
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        fprintf(stderr,"Error opening DB.%s\n", sqlite3_errmsg(db));
        return 1;
   	 }
sqlite3_stmt *stmt;
const char *sql = "SELECT password FROM Users WHERE username = ?;";
int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
if (rc != SQLITE_OK)
{
fprintf(stderr, "can't read from DB: %s\n", sqlite3_errmsg(db));
return 1;
}
sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
{
	char *db_password = (char *)sqlite3_column_text(stmt, 0);
	if (strcmp(db_password, hash_hex) == 0)
	{
		sqlite3_finalize(stmt);
	close_db(db);
	return true;
	}
}
    close_db(db);
    return false;
}

int getuserID(const char* username)
{
	int userid;
sqlite3 *db = open_db(DB_FILE);
if (!db) {
        fprintf(stderr,"Error opening DB.%s\n", sqlite3_errmsg(db));
        return 1;
   	 }
sqlite3_stmt *stmt;
const char *sql = "SELECT User_id FROM Users WHERE username = ?;";
int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
if (rc != SQLITE_OK)
{
fprintf(stderr, "can't read from DB: %s\n", sqlite3_errmsg(db));
return 1;
}
sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
{
	userid = sqlite3_column_int(stmt, 0);
		
}
sqlite3_finalize(stmt);
    close_db(db);
    return userid;


}

// verify_password function ends here
void login(char* username, char* password, int sock) {
    if (!user_exists(username)) {
        send_to_client("❌ User not found",sock);
        return;
    }

    if (!verify_password(username, password)) {
        send_to_client("❌ Wrong password",sock);
        return;
    }
    else
    {
	    int userid = getuserID(username);
	    send_to_client("✅ Login successful!\n✨ Welcome aboard ✨",sock);
	    Todo_Menu(sock,userid);

    }
    //char token[64];
    //generate_token(token);
   // time_t expiry = time(NULL) + 1800;  // 30 min expiry
   // save_session(username, token, expiry);

    //char response[128];
    //sprintf(response, "LOGIN_SUCCESS token=%s", token);
    //send_to_client(response);
}

void Register(char* username , char* password, int sock)
{
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
if (user_exists(username))
{
send_to_client("Username Already Exists\n",sock);
return;
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
        send_to_client("[1].Register  [2].Login  [3].Exit \n> ", clientSocketFD);

        // Receive choice
        ssize_t bytes_recv = recv(clientSocketFD, &choice, sizeof(choice), 0);
        if (bytes_recv <= 0) {
            printf("Client disconnected unexpectedly.\n");
            break;
        }
        //printf("[DEBUG] CHOICE BYTES: %ld\n", bytes_recv);
        //printf("[DEBUG] CHOICE: %d\n", choice);

        if (choice == 2) {
            ssize_t U_bytes_recv = recv(clientSocketFD, username, sizeof(username), 0);
            ssize_t P_bytes_recv = recv(clientSocketFD, password, sizeof(password), 0);
          //  printf("[DEBUG] Username Received: %s\n", username);
            //printf("[DEBUG] Username BYTES Received: %ld\n", U_bytes_recv);
            //printf("[DEBUG] Password Received: %s\n", password);
           /// printf("[DEBUG] Password BYTES Received: %ld\n", P_bytes_recv);

            // Send login message
            login(username,password,clientSocketFD);

        } else if (choice == 1) {
            ssize_t Ubytes_recv = recv(clientSocketFD, username, sizeof(username), 0);
            //printf("[DEBUG] Username Received: %s\n", username);
            //printf("[DEBUG] Username BYTES Received: %ld\n", Ubytes_recv);
            ssize_t Pbytes_recv = recv(clientSocketFD, password, sizeof(password), 0);
            //printf("[DEBUG] Password Received: %s\n", password);
            //printf("[DEBUG] Password BYTES Received: %ld\n", Pbytes_recv);
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
