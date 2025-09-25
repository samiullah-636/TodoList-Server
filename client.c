#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
void send_password(int sock)
{
	char password[1024];
 
	    printf("Enter Password: ");
            fgets(password, sizeof(password), stdin);
            password[strcspn(password, "\n")] = '\0';
		size_t plength = strlen(password);
//		printf("PASSWORD BYTES: %ld\n",plength);
            send(sock, password, strlen(password)+1, 0);
}

void send_username(int sock)
{
char username[1024];
            printf("Enter Username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';
		size_t length = strlen(username);
//		printf("USERNAME BYTES: %ld\n",length);
            send(sock, username, strlen(username)+1, 0);

}

void handle_mainmenu(int sock)
{
	char menu[1024];
	        // Clear the buffer to avoid garbage value
        memset(menu, 0, sizeof(menu));

        // Receive and print the menu from the server
        // Send ACK to server to request menu
send(sock, "ACK", 3, 0);

// Now receive the menu
ssize_t bytes_recv = recv(sock, menu, sizeof(menu), 0);
menu[bytes_recv] = '\0';
printf("%s", menu);
//printf("BUFFER BYTES: %ld\n", bytes_recv);
}

int handle_choice(int sock)
{        
	char input[1024];
         int choice;
	 fgets(input, sizeof(input), stdin);
	input[strcspn(input, "\n")] = 0;
	choice= atoi(input);
//	//printf("CHOICE: %d\n",choice);
//	//printf("CHOICE BYTES: %ld\n",sizeof(choice));
        send(sock, &choice, sizeof(choice), 0);
        return choice;
}

void handle_todoresponse(int sock)
{
char buffer[1024];
// Receive and display the result
 memset(buffer, 0, sizeof(buffer));
 ssize_t R_bytes_recv=recv(sock, buffer, sizeof(buffer), 0);
 printf("%s\n", buffer);
}

void handle_Addtask(int sock)
{
	printf("Enter Task: ");
	char task[256];
        fgets(task, sizeof(task), stdin);
        task[strcspn(task, "\n")] = '\0';
        send(sock, task, strlen(task)+1, 0);
	handle_todoresponse(sock);
	
}

void handle_Listtask(int sock)
{
	handle_todoresponse(sock);
	handle_todoresponse(sock);

}

int getTaskid(int sock)
{
	char input[1024];
         int taskid;
	 printf("Enter Id: ");
	 fgets(input, sizeof(input), stdin);
	input[strcspn(input, "\n")] = 0;
	taskid= atoi(input);
        send(sock, &taskid, sizeof(taskid), 0);
	handle_todoresponse(sock);
}

void handle_Edit_Task(int sock)
{
        printf("Enter New Task: ");
	char task[256];
        fgets(task, sizeof(task), stdin);
        task[strcspn(task, "\n")] = '\0';
        send(sock, task, strlen(task)+1, 0);

         getTaskid(sock);
}

void handle_todomenu(int sock)
{
char menu[1024];
	while(1)
	{
		        // Clear the buffer to avoid garbage value
        memset(menu, 0, sizeof(menu));

        // Receive and print the menu from the server
        // Send ACK to server to request menu
send(sock, "ACK", 3, 0);

// Now receive the menu
ssize_t bytes_recv = recv(sock, menu, sizeof(menu), 0);
menu[bytes_recv] = '\0';
printf("%s", menu);
//printf("BUFFER BYTES: %ld\n", bytes_recv);
int choice = handle_choice(sock);
switch(choice)
{
	case 1:
		handle_Addtask(sock);
		break;
	case 2:
		handle_Listtask(sock);
		break;
	case 3:
		getTaskid(sock);
		break;
	case 4:
		handle_Edit_Task( sock);
		break;
	case 5:
		getTaskid(sock);
		break;
	case 6:
		printf("Logging out ..\n");
		return;
	
}


	}
}
void handle_response(int sock)
{
char buffer[1024];
// Receive and display the result
 memset(buffer, 0, sizeof(buffer));
 ssize_t R_bytes_recv=recv(sock, buffer, sizeof(buffer), 0);
 printf("%s\n", buffer);
if (R_bytes_recv == 44)
{
	handle_todomenu(sock);
}
}
int main() {
    int sock;
    struct sockaddr_in server_address;
    int choice;
    int ack=1;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation failed\n");
        return -1;
    }

    // Set server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        printf("Connection failed\n");
        close(sock);
        return -1;
    }

    while (1) {
       handle_mainmenu(sock);

       choice = handle_choice(sock);
        
        if (choice==3)
        {
        printf("Exiting... \n");
        break;
        }

        else if (choice == 1 || choice == 2) {
		send_username(sock);
		send_password(sock);
        }

	handle_response(sock);

                    }

    close(sock);
    return 0;
}
