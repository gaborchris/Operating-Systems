#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define BUFF_SIZE 1024
#define OTP_ENC_FILE "OTP_ENC_SK"

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

//Write socket number funcion
//Creates a file and writes the port it is using in order to block the wrong process from using it
void Write_Daemon_Socket(int socketnum){
	int FD;
	char number[16];//number to save 
	memset(number, '\0', sizeof(number)); 
	sprintf(number, "%d", socketnum); //put the socketnum into a string

	FD = open(OTP_ENC_FILE, O_WRONLY | O_TRUNC | O_CREAT, 0600); //open file for writing
	write(FD, number, sizeof(number)); //write the number to that file

}

//Encrypts the message using the key that it was given 
//will add a number to the character, with all encodings based on ASCII
//a space is considered 26
void Encrypt(char buffer[BUFF_SIZE], char key[BUFF_SIZE]){
	//printf("Here is the message: %s\n", buffer);
	//printf("Here is the key: %s\n", key);

	int i;
	int tok; //token to use for each key
	for(i=0; i<strlen(buffer); i++){ //loop through entire buffer
		if(key[i] == ' '){ //if space, set key to space value
			tok = key[i];
			tok -= 6; //sub 6 to make it 26
		}
		else{
			tok = key[i];
			tok -= 65; //A goes to zero, B goes to 1, etc
		}
		if(buffer[i] == ' '){
			buffer[i] = ((buffer[i] - 6 + tok)%27); //add the key and mod it by 27
		}
		else{
			buffer[i] = ((buffer[i] - 65 + tok)%27);
		}
		if(buffer[i] == 26){ //space
			buffer[i] += 6;  //space scenario
		} 
		else{
			buffer[i] += 65;  //set the chars back to ASCII char
		}
	}
}

//Does the child process that encrypts the message
//Gets the message and the key seperated by a @ and a $ at the end
//breaks the message up, then calls the encryption function
void Cipher(int establishedConnectionFD){
	char message[BUFF_SIZE];
	char buffer[BUFF_SIZE];
	char key[BUFF_SIZE];
	int charsRead;
	pid_t spawnpid = -5;
	spawnpid = fork();
	//fcntl(establishedConnectionFD, F_SETFL, O_NONBLOCK);
	switch(spawnpid){
		case 0:
			// Get the message from the client and display it
			memset(buffer, '\0', BUFF_SIZE);
			// Get the message from the client and display it
			charsRead = recv(establishedConnectionFD, buffer, BUFF_SIZE-1, 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");
			if (charsRead != BUFF_SIZE-1) error("ERROR buff size not correct");
	//		printf("CHILD: I received this from the client: \"%s\"\n", buffer);

			//find where the key in the message starts
			int keyLocation = strstr(buffer, "@") - buffer;
			//find the end of the message
			int terminalLocation = strstr(buffer, "$") - buffer;
			//set the end to NULL to get rid of $ char
			buffer[terminalLocation] = '\0';
//			printf("%s\n", buffer);

			memset(key, '\0', BUFF_SIZE);
			memset(message, '\0', BUFF_SIZE);
			//copy the key portion of buffer into the key string
			strncpy(key, (buffer + keyLocation + 1), sizeof(buffer));
			//copy the message portion of the buffer into the message string
			strncpy(message, buffer, keyLocation);


			//encode the message with the key
			Encrypt(message, key);

			// Send a Success message back to the client

			charsRead = send(establishedConnectionFD, message, BUFF_SIZE, 0); // Send success back
			if (charsRead < 0) error("ERROR writing to socket");
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
			exit(0);
			break;
		default: 
			break;
	}
}
int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, status;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;
	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	//write the file that holds the number for other processes to read
	Write_Daemon_Socket(portNumber);
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");

	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	while(1){
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");
		//	printf("SERVER: Connected Client at port %d\n", ntohs(clientAddress.sin_port));
		//call the function that listens on this socket with a child process
		Cipher(establishedConnectionFD);
		//clear up any processes that have finished
		waitpid(-1, &status, WNOHANG);
	}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
