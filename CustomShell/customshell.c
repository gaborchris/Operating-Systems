//*********************
//program: smallsh
//author: Christian Gabor
//date: 11/13/2017
//class: cs344
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>


#define MAX_LENGTH 2048
#define MAX_ARGUMENTS 512
#define MAX_PIDS 32

typedef int bool;
#define true 1
#define false 0

bool foreground_only_state; //The state for foreground or background is stored here

//**************************
//Function to change state of foreground_only_state for SIGTSTP
//toggles the state and displays message to UI
//**************************
void catchSIGTSTP(int signo){	//alternative function for used SIGTSTP
	if(foreground_only_state ^=1){
		char* message ="\nEntering foregound-only mode (& is now ignored)\n: "; 
		write(STDOUT_FILENO, message, 51);
		fflush(stdout);
	}
	else{
		char* message ="\nExiting foregound-only mode\n: "; 
		write(STDOUT_FILENO, message, 31);
		fflush(stdout);
	}
}
//*****************************
//convert to PID
//takes the input string and wherever the is $$, it replaces it with the shell PID
//loops through each occurence of this $$ and replaces it in arg
//*****************************
char* convert_PID(char* arg){
	static char buffer[1024];
	memset(buffer, '\0', sizeof(buffer));
	char* location; 	//where the $$ is found
	char processID[16];	//a temp string to hold PID

	sprintf(processID, "%d", getpid()); 	//put the numerical PID into a string
	while(strstr(arg, "$$")){ 		//loop until all $$ are dealt with
		location = strstr(arg, "$$"); 	//find the location of the $$
		strncpy(buffer, arg, location-arg);	 //copy the argument into the buffer minus the $$
		sprintf(buffer + (location - arg), "%s", processID);  	//put the new PID at the place where $$ was
		sprintf(buffer + strlen(buffer), "%s", location + 2); 	//print the remaining arg string into the buffer, starting where $$ ended

		strcpy(arg, buffer); 	//copy the buffer into the argument passed in
	}
}
//**************************
//Function to get user input for commands
//gets the line
//determines if the line has comments
//deals with string cadences
//uses strtok to break up the input line to commands and arguments
//changes instance of $$ to process ID of smallsh
//stores the outputs in the passed in string arrays and num_args int
void Get_Command_Line(char input_line[], char* args[], int* num_args){
	char* position;
	int count;
	int i;

	memset(input_line, '\0', sizeof(input_line));	//sets the user input lin

	printf(": " );	//the prompt for user input
	fflush(stdout);	//flush stdout

	fgets(input_line, MAX_LENGTH, stdin);	 //read the line from the user

	if((position=strchr(input_line, '\n')) != NULL)	//checks if the user input has a newline
		*position = '\0';	//if is has a newline, sets the newline to null
	i = 0;	
	do{		//handles comments
		if(input_line[i] == '#')
			input_line[i] = '\0'; 	//cut the line off here
		i++;
	}while(input_line[i-1] != '\0');

	count = 0;		//stores number of arguments
	char* token = strtok(input_line, " ");	//break the input line up into spaces
	while(token != NULL){		//keep reading until hitting NULL terminator
		if(strstr(token, "$$")){
			static char buffer[1024];
			memset(buffer, '\0', sizeof(buffer));
			strcpy(buffer, token);
			convert_PID(buffer);
			args[count] = malloc(strlen(buffer) * sizeof(char));	//set the size of this argument to the size of the token
			memset(args[count], '\0', (strlen(buffer) * sizeof(char)));	//set these to NULL
			strcpy(args[count], buffer);	//copy the token string into the argument array
		}
		else{
			args[count] = malloc(strlen(token) * sizeof(char));	//set the size of this argument to the size of the token
			memset(args[count], '\0', (strlen(token) * sizeof(char)));	//set these to NULL
			strcpy(args[count], token);	//copy the token string into the argument array
		}
		count++;	//increment arguments

		token = strtok(NULL, " ");	//get the next token
	}
	args[count] = malloc(sizeof(char));
	args[count] = NULL;
	*num_args = count;	//set the number of args to the count found in this process
}

//**************************
//Frees memory allocated to the strings in the argument array 
void Free_Args(char* args[], int num_args){
	int i;
	for(i = 0; i<num_args; i++){	//free up the memory used to store the arguments
		free(args[i]);
	}
}

//**************************
//Checks if the string entered to it is the exit command
//returns true if argument is "exit"
//returns false otherwise
bool Check_Exit_Command(char* argument){
	if(strcmp(argument, "exit") == 0){
		return true;
	}
	return false;
}

//**************************
//When the shell exits, this nukes all of the PIDs of background processes
//Then it exits
void Exit_Routine(int PIDs[]){
	int i;
	for(i = 0; i<MAX_PIDS; i++){	//Look through each PID in the array
		if(PIDs[i] != 0){	//if it is not zero, kill this one
			kill(PIDs[i], SIGKILL);
		}
	}
	exit(0);	//exit normally
}

//**************************
//Checks if the string entered to it is the CD command
//returns true if argument is "CD"
//returns false otherwise
bool Check_CD_Command(char* argument){
	if(strcmp(argument, "cd") == 0){
		return true;
	}
	return false;
}

//**************************
//Runs the CD command
//If no destination is specified, go to $HOME
//Otherwise go to the location set by user
void CD_Command(char* args[], int num_args){
	if(num_args==1){
		chdir(getenv("HOME"));	//chdir to $HOME, specified by environment
	}
	else if(num_args==2){
		int ret = chdir(args[1]);	//otherwise go to the loction put in args[1]
		if(ret != 0){	//error handling
			printf("Could not find directory: %s\n", args[1]);
			fflush(stdout);
		}
	}
	else{	//error handling if too many args
		printf("Invalid number of arguments entered\n");
		fflush(stdout);
	}
}

//**************************
//Runs the status command
//takes and input from last foreground process run
//checks for exit status or siganled status
void Status_Command(int status){
	if(WIFEXITED(status)){	//only does this if exited
		printf("exit value %d\n", WEXITSTATUS(status)); //print the exit status number
		fflush(stdout);
	}
	else if(WIFSIGNALED(status)){	//does this if it was signalled
		printf("terminated by signal %d\n", WTERMSIG(status)); //print the signal that killed it
		fflush(stdout);
	}
}

//**************************
//Checks if the string entered to it is the status command
//returns true if argument is "status"
//returns false otherwise
bool Check_Status_Command(char* argument){
	if(strcmp(argument, "status") == 0){
		return true;
	}
	return false;
}

//**************************
//Check if the line entered by the user is not empty
//returns true or false
bool Line_Not_Empty(char* args){
	if(args == NULL)	//checks if the entire line is NULL
		return false;
	if(args[0] == '\0')	//checks if the first char is NULL
		return false;
	return true;	//otherwise the line is not NULL or empty
}

//**************************
//Get the input location for file redirection
//returns the location of the argument that input redirects to
int Get_Input_Location(char* args[], int num_args){
	int i;
	for(i=1; i<num_args - 1; i++){	//check each argument past the first one and second to last
		if(strcmp(args[i], "<")==0){	//if the < symbol is present, the next arg is the one for redirection
			return (i + 1);	//returns location
		}
	}
	return -1;	//otherwise returns a negative value
}

//**************************
//Get the output location for file redirection
//returns the location of the argument that output redirects to
int Get_Output_Location(char* args[], int num_args){
	int i;
	for(i=1; i<num_args-1; i++){ //check each argument past the first one and second to last
		if(strcmp(args[i], ">")==0){ //if its the > symbol it is output
			return (i + 1);	//return location after > symbol
		}
	}
	return -1; //otherwise return negative
}

//**************************
//Checks if an argument is the & symbol
//returns true or false if symbol passed in is &
bool Is_Background(char* arg){
	if(strcmp(arg, "&") == 0){
		return true;
	}
	return false;
}

//**************************
//Redirects STDOUT using teh dup2 function
//uses the location passed into it
//checks that the file can be opened
void Redirect_STDOUT(char* arg){
	int targetFD = open(arg, O_WRONLY | O_CREAT | O_TRUNC, 0644); //open the location passed in for writing only, creat if it doesn't already exist
	if(targetFD == -1){
		printf("Cannot open %s for output", arg); 	 //error handling
		fflush(stdout);
		perror("");
		exit(1);
	}
	dup2(targetFD, 1); 	//set STDOUT to the file opened
}

//**************************
//Redirects STDIN using the dup2 function
//uses the location passed into it
//checks that the file can be opened
void Redirect_STDIN(char* arg){
	int targetFD = open(arg, O_RDONLY); //open the file named by passed in arg for read only
	if(targetFD == -1){	//error handling
		printf("Cannot open %s for input: ", arg); 
		fflush(stdout);
		perror("");
		exit(1);
	}
	dup2(targetFD, 0); //redirect input from the file opened for reading
}

//**************************
//Adds a PID to the PIDs array passed in
//Returns true if it was able to store one
//False otherwise
bool Add_PIDs(int PIDs[], int spawnpid){
	int i;
	for(i = 0; i<MAX_PIDS; i++){	 //look through each available space in the PID array
		if(PIDs[i] == 0){ //if it found an available space
			PIDs[i] = spawnpid;  //store the PID
			return true; //then return
		}
	}
	return false; 	//if looked through all PIDs and found no space, return false
}

//**************************
//Runs a non built in command
//Takes in the values from args
//determines background process
//determines input or output locations
//Stores background processes in the PID array
//Forks a child and then Execs from path environment
//waits for foreground children
//does not wait for background children before returning
//returns the exit method of the foreground child
int Run_Command(char* args[], int num_args, int PIDs[]){
	pid_t spawnpid = -5;
	int childExitMethod = -5;
	int input_location;
	int output_location;	
	bool background;

	if((background = Is_Background(args[num_args - 1])) == true){ //set background bool and check if true
		args[num_args-1] = NULL;	//ignore the & symbol for exec
		num_args -= 1;	//ignore this argument
		if(foreground_only_state){ //if the shell is in the FG state, it sets background to false
			background = false;
		}
	}
	else{ //if the background symbol wasn't the last argument, set this bool to false
		background = false;
	}
	spawnpid = fork(); 	//spawn a new process
	switch(spawnpid){
		case -1: 	//error handling
			perror("Error during spawnpid!\n");
			exit(1);
			break;
		case 0:		//this is what the new child sees
			signal(SIGTSTP, SIG_IGN); 	//ignores SIGTSTP signals
			if((output_location = Get_Output_Location(args, num_args))!= -1){	 //get the argument for redirecting output, only returns if valid
				Redirect_STDOUT(args[output_location]); //set new stdout to specified argument
				args[output_location] = NULL; 	//now ignore this argument for exec
				args[output_location - 1] = NULL; 	//now ignore this argument for exec
				num_args -= 2;
			}
			else if(background){ 		//otherwise, if the location wasn't specified and it is a background process, STDOUT goes to dev/null
				Redirect_STDOUT("/dev/null");
			}
			if((input_location = Get_Input_Location(args, num_args))!= -1){ //get arugment for iput, only returns if valid
				Redirect_STDIN(args[input_location]); //set new stdin to specified argument
				args[input_location] = NULL; 	//now ignore this argument for exec
				args[input_location - 1] = NULL; 	//now ignore this argument for exec
				num_args -= 2;
			}
			else if(background){ //otherise, if the location was not specified, set the input to dev/null
				Redirect_STDIN("/dev/null");
			}

			if(!background){		//if the process isn't run in the background, set SIGINT to be able to terminate the FG child procress
				signal(SIGINT, SIG_DFL); //SIGINT set to defaults
			}

			execvp(args[0], args); 	//exec a child using the first argument and the refined args list
			perror(args[0]); 	//should not run unless there was an error with exec
			exit(1); 	//exits 1
			break;
	}
	//only parent will see this folling code before the funciton ends
	if(background){ //does this if the background bool was set
		if(!Add_PIDs(PIDs, spawnpid)){ 	//Will add PIDs to the array and check if successful at the same time
			printf("Too many background processes! Running in foreground instead...\n"); //by default, if too many BG processes are running, will just do in foreground
			fflush(stdout); 	
			waitpid(spawnpid, &childExitMethod, 0); //wait for this FG child progress to finish
			return childExitMethod;  	//returns the exit method of this process
		}
		else{
			printf("background pid is %d\n", spawnpid); //otherwise prints out the BG process that started up (usually the default action)
			fflush(stdout);
			//notice, no waitpid on the child, which will run it in the background from the users perspective
		}
	}
	else{ 	//if the process wasn't for the background
		waitpid(spawnpid, &childExitMethod, 0); 	//wait on this process to run before allowing user input
		if(WIFSIGNALED(childExitMethod)){ 	//if this was termined while running, notiify the user
			printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
		}
	}
	return childExitMethod; //return the exit method of the foreground process for the status command
}
//**************************
//Checks on the background children
//Waits on them to clean them up but immediately returns if they aren't done
//loops through each one in the PID array passed in
//Sets the PID array to zero where the child has terminated
void Check_Children(int PIDs[]){ 
	int exitMethod;
	int i;
	for(i = 0; i<MAX_PIDS; i++){ //loop through the PID array
		if(PIDs[i] != 0){ //if one of them is not empty, check this one
			if(waitpid(PIDs[i], &exitMethod, WNOHANG)){ 	//wait on this but return right away if not done
				if(WIFEXITED(exitMethod)){ 	//if it is done, check if it exited
					printf("background pid %d is done: exit value %d\n", PIDs[i], WEXITSTATUS(exitMethod)); //print the exit status of it
					PIDs[i] = 0; //set the PID space back to zero so another PID can use this
				}
				else if(WIFSIGNALED(exitMethod)){ //check if the process was termined by a signal
					printf("background pid %d is done: terminated by signal %d\n", PIDs[i], WTERMSIG(exitMethod)); //print the terminating signal
					PIDs[i] = 0; //set the PID space back to zero for use by another PID later
				}
			}
		}
	}
}

int main(){
	char input_line[MAX_LENGTH];
	char* args[MAX_ARGUMENTS];	
	int PIDs[MAX_PIDS];
	int num_args;
	int commandExitMethod;
	int dummyVar;
	bool builtin;
	int length;
	foreground_only_state = false; //initizliae this to false so that the shell starts in normal mode

	int i;
	for(i = 0; i<MAX_PIDS; i++){ //initialize the PID array
		PIDs[i] = 0;
	}

	struct sigaction SIGTSTP_action = {0}; //signal handler for the SIGTSTP

	SIGTSTP_action.sa_handler = catchSIGTSTP; //runs this function as the signal handler
	SIGTSTP_action.sa_flags = SA_RESTART; //set this to allow the SIGTSTP signal to be changed later

	sigaction(SIGTSTP, &SIGTSTP_action, NULL); //make SIGTSTP do the catchSIGTSTP funciton when it recieves this signal

	signal(SIGINT, SIG_IGN); //ignore SIGINT signals in the main smallsh program

	while(1){
		Check_Children(PIDs);  //check the children for completion
		Get_Command_Line(input_line, args, &num_args); //get the next command from the user
		if(Line_Not_Empty(args[0])){ //only do this if the first command wasn't null or the line was not empty
			if(Check_Exit_Command(args[0])){ //check if the exit was called
				Free_Args(args, num_args); //free up the args
				Exit_Routine(PIDs); //kill all processes and exit
			}
			else if(Check_CD_Command(args[0])){	 //check if the CD command was called
				CD_Command(args, num_args); //run the CD command 
			}
			else if(Check_Status_Command(args[0])){ //check if the status command was called
				Status_Command(commandExitMethod); //run the status command with the last FG process
			}
			else{
				dummyVar = Run_Command(args, num_args, PIDs); //run the enviroment command and also store this in the dummy variable
				if(dummyVar != -5){ //if it returned a valid value that isn't -5 then set this as the new foreground exit method
					commandExitMethod = dummyVar;
				}
			}
		}
		Free_Args(args, num_args); //free up any arguments that were made during the last command input
	}
	return 0;
}

