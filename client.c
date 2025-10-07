#include "fileServer.h"
#include "client.h"

#define DEBUG 1

void showError (char* msg){

	printf ("[Client] %s\n", msg);
	exit (1);
}


/**
 * Copy a file from server side to client side
 */
void receiveFile (char *host, char* src, char* dst){
	
}


/**
 * Copy a file from client side to server side
 */
void sendFile (char *host, char* src, char* dst){

}


int main (int argc, char *argv[]){

	// Check arguments
	if(argc != 5) {
		printf("usage: %s serverIP [sendFile|receiveFile] sourceFile destinationFile\n", argv[0]);
		exit(1);
	}

	// Check mode
	else{

		// Send file to server
		if (strcmp (argv[2], "sendFile") == 0){
			sendFile (argv[1], argv[3], argv[4]);
		}

		// Receive file from server
		else if (strcmp (argv[2], "receiveFile") == 0){
			receiveFile (argv[1], argv[3], argv[4]);
		}

		// Wrong mode!
		else{
			printf("Wrong mode!\nusage: %s serverIP [sendFile|receiveFile] sourceFile destinationFile\n", argv[0]);
			exit(1);
		}
	}
}
