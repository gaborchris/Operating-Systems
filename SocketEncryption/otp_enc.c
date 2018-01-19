#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

#define BUFF_SIZE 1024
typedef int bool;
#define true 1
#define false 0

#define OTP_ENC_FILE "OTP_ENC_SK"
void error(const char *msg) { perror(msg); exit(2); } // Error function used for reporting issues

//check key file size to ensure it is large enough for the message
//if it is not big enough, exit 1
void Check_File_Size(int FDkey, int msize, char* filename){
	//set the start of the file
	lseek(FDkey, 0, SEEK_SET);
	int keysize; 
	//find the end of the file
	keysize = lseek(FDkey, 0, SEEK_END)+1;
	//compare if this is less than the size of the message
	if(keysize < msize){
			fprintf(stderr, "Error: key '%s' is too short\n", filename);
			exit(1);
	}
}
//Check for bad chars
//if any of the chars are outside of the 26 capital chars or space it will print and error and exit
void Check_Bad_Chars(int FD){
	//go to beginning of file
	lseek(FD, 0, SEEK_SET);
	int size, i, num;
	char c;
	//find the last char to check
	size = lseek(FD, 0, SEEK_END)-1;
	//go back to beginning of file
	lseek(FD, 0, SEEK_SET);
	for(i = 0; i<size; i++){
		//check each char
		read(FD, &c, sizeof(char)*1);
//			printf("%d - %d: %c %d\n", size,i,c,c);
//			//if the ascii value is illegal, flag this
		if((c < 65 || c > 90) && c != 32){
			fprintf(stderr, "otp_enc error: input contains bad characters\n");
			exit(1);
		}

	}
}

//Check the port based on where the daemon was supposed to write the file
//this is defined at the top of both files
void Check_Port(int socketnum){
	int FD;
	int port;
	char number[16];
	memset(number, '\0', sizeof(number));

	//open the designated daemon file
	FD = open(OTP_ENC_FILE, O_RDONLY);
	//go to beginning of file
	lseek(FD, 0, SEEK_SET);
	//read the port number in the file
	read(FD, number, sizeof(number));
	//change the port to an int
	port = atoi(number);
	//check if the ports don't match up
	if(socketnum != port){
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", socketnum);
		exit(2);
	}

}

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[BUFF_SIZE];
	char readbuffer[(BUFF_SIZE/2 - 2)];

	if (argc < 4) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	Check_Port(portNumber);
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	//open this as the message file
	int FD = open(argv[1], O_RDONLY);
	//check if the chars are bad
	Check_Bad_Chars(FD);
	lseek(FD, 0, SEEK_SET);
	//find out how big this message is
	int size;
	size = lseek(FD, 0, SEEK_END)+1;

	//make a string that is as long as the full message to send
	char fullMessage[size];
	memset(fullMessage, '\0', sizeof(fullMessage));
	lseek(FD, 0, SEEK_SET);

	//open the second arg as the key
	int FDkey = open(argv[2], O_RDONLY);
	lseek(FDkey, 0, SEEK_SET);
	//check the size of the key
	Check_File_Size(FDkey,size, argv[2]);
	lseek(FDkey, 0, SEEK_SET);
	//make an end of file flag
	bool eof = false;
	//loop while eof hasn't been found
	while(eof == false){
		memset(readbuffer, '\0', sizeof(readbuffer));
		read(FD, readbuffer, sizeof(readbuffer));
		if(strstr(readbuffer, "\n") != NULL){
			int termChar = strstr(readbuffer, "\n") - readbuffer;
			readbuffer[termChar] = '\0';
			eof = true;
		}

		socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
		if (socketFD < 0) error("CLIENT: ERROR opening socket");

		// Connect to server
		if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
			error("CLIENT: ERROR connecting");

		// Send message to server
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
		//copy the readbuffer onto the message to send in socket
		strcat(buffer, readbuffer);
		//put a @ symbol to designate the end of the message
		strcat(buffer, "@");

		memset(readbuffer, '\0', sizeof(readbuffer));
		//read the key into the read buffer
		read(FDkey, readbuffer, sizeof(readbuffer));

		//add the key onto the buffer to send to the socket
		strcat(buffer, readbuffer);
		//put a $ to signify the end of the message 
		strcat(buffer, "$");
		//send the message over
		charsWritten = send(socketFD, buffer, BUFF_SIZE, 0); // Write to the server
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		if (charsWritten < sizeof(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

		// Get return message from server
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
		charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
		if (charsRead < 0) error("CLIENT: ERROR reading from socket");
		//		printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
		//copy the message recieved from the socket and add that onto the end of the full message
		strcat(fullMessage, buffer);

		close(socketFD); // Close the socket
	}
	close(FD);

	close(FDkey);

	//add a newline to the message
	strcat(fullMessage, "\n");
	//print out the full message
	printf(fullMessage);
	return 0;
}

