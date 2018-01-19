//Chris Gabor
//11-26-17
//CS344
//Keygen.c
//Generates random characters between A-Z and a space and prints them out
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define NUMCHAR 27 //number of possible chars to print out

int i; //global counter for easy looping
//Set an array of ASCII letters between A-Z (based on NUMCHAR) and a space
void Set_ASCII(char charDict[NUMCHAR]){
	charDict[0] = ' '; //zero is a space
	int a = 65; //ASCII value of 'A'
	for(i = 1; i<NUMCHAR; i++){ //set the rest of the array to ASCII values
		charDict[i] = a;
		a++;
	}
}
//Generate random letters from the ASCII array for number passed in
void Gen_Rands(int num, char letters[NUMCHAR]){
	int j;
	for(i = 0; i<num; i++){ 	//loop from 0 to number passed in
		j = rand()%27; 	//random number between 0 and 26
		printf("%c", letters[j]); //print the index of ASCII values from 0-26
	}
	printf("\n"); //print a newline
}
int main(int argc, char* argv[]){

	if(argc != 2){ //make sure the user puts in the right format
		fprintf(stderr, "Use format: keygen [keylength]\n");
		return 1; 	//return 1 for stderr
	}

	char encryptionChars[NUMCHAR]; 	//an array to hold the char dictionary
	Set_ASCII(encryptionChars); 	//populate the array

	srand(time(NULL)); 	//used for random num generation

	Gen_Rands(atoi(argv[1]), encryptionChars); 	//generate random numbers

	return 0;
}
