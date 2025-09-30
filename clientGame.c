#include "clientGame.h"

void sendMsgToServer(int socket, char* message){
	int l = strlen(message);
	if(send(socket,&l,sizeof(l),0) < 0)
		showError("Error al enviar la longitud del mensaje");
	if(send(socket,message,l,0) < 0)
		showError("Error al enviar el mensaje");
}

unsigned int receiveUi (int socket){
	unsigned int l;
	if(recv(socket,&l,sizeof(l),0) <= 0)
		showError("Error al recibir el code");
	return l;
}

void receiveMsg(int socket, char *message){

	int l;
	memset(message,0,STRING_LENGTH);
	if(recv(socket, &l, sizeof(l), 0) <= 0)
		showError("Error al recibir la longitud del mensaje");
	if(recv(socket, message, l, 0) <= 0)
		showError("Error al recibir el mensaje");
}

void sendUi(int socket, unsigned int code){

	if(send(socket, &code, sizeof(code), 0) < 0)
		showError("Error al enviar el code");
}

unsigned int readBet (){

	int isValid, bet=0;
	tString enteredMove;
 
		// While player does not enter a correct bet...
		do{

			// Init...
			bzero (enteredMove, STRING_LENGTH);
			isValid = TRUE;

			printf ("Enter a value:");
			fgets(enteredMove, STRING_LENGTH-1, stdin);
			enteredMove[strlen(enteredMove)-1] = 0;

			// Check if each character is a digit
			for (int i=0; i<strlen(enteredMove) && isValid; i++)
				if (!isdigit(enteredMove[i]))
					isValid = FALSE;

			// Entered move is not a number
			if (!isValid)
				printf ("Entered value is not correct. It must be a number greater than 0\n");
			else
				bet = atoi (enteredMove);

		}while (!isValid);

		printf ("\n");

	return ((unsigned int) bet);
}

unsigned int readOption (){

	unsigned int bet;

		do{		
			printf ("What is your move? Press %d to hit a card and %d to stand\n", TURN_PLAY_HIT, TURN_PLAY_STAND);
			bet = readBet();
			if ((bet != TURN_PLAY_HIT) && (bet != TURN_PLAY_STAND))
				printf ("Wrong option!\n");			
		} while ((bet != TURN_PLAY_HIT) && (bet != TURN_PLAY_STAND));

	return bet;
}

int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	unsigned int port;					/** Port number (server) */
	struct sockaddr_in server_address;	/** Server address structure */
	char* serverIP;						/** Server IP */
	unsigned int endOfGame;				/** Flag to control the end of the game */
	tString playerName;					/** Name of the player */
	unsigned int code;					/** Code */


	// Check arguments!
	if (argc != 3){
		fprintf(stderr,"ERROR wrong number of arguments\n");
		fprintf(stderr,"Usage:\n$>%s serverIP port\n", argv[0]);
		exit(0);
	}

	// Get the server address
	serverIP = argv[1];

	// Get the port
	port = atoi(argv[2]);

	socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(socketfd < 0){
		showError("Error while creating the socket");
	}

	serverIP = argv[1];

	memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(serverIP);
	server_address.sin_port = htons(port);

	if(connect(socketfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
		showError("Error while stablishing connection");
	}

	do{
		memset(playerName, 0, STRING_LENGTH);
		printf("Enter player name:");
		fgets(playerName, STRING_LENGTH-1, stdin);
	}while(strlen(playerName) <= 2);

	sendMsgToServer(socketfd, playerName);

	endOfGame = FALSE;

	printf("Creating the play room\n\n");

	receiveMsg(socketfd, playerName);
	printf("You are playing againts %s\n\n", playerName);
	printf("Game starts!\n\n");

	unsigned int stack;
	unsigned int bet;

	code = receiveUi(socketfd);

	do{
		switch (code){
			case TURN_BET:
				stack = receiveUi(socketfd);
				printf("--- BET STAGE ---\n");
                printf("You have %u chips. Introduce your bet (1-%d): \n", stack, MAX_BET);
				bet = readBet();
				sendUi(socketfd, bet);
				code = receiveUi(socketfd);
				if(code == TURN_BET_OK){
					printf("Your bet was registered correctly\n\n");
					code = receiveUi(socketfd);

				}
				break;
			case TURN_BET_OK:
				printf("Your bet was registered correctly\n\n");
				break;
			case TURN_PLAY:
				
				break;
			case TURN_PLAY_OUT:

				break;
			case TURN_PLAY_RIVAL_DONE:
				printf("Your rival finish their turn. Now it's your turn\n\n");
				break;
			case TURN_PLAY_WAIT:

				break;
			case TURN_GAME_WIN:
				printf("Congratulations, you win!!! You're an expert gambling addict!!\n\n");
				break;
			case TURN_GAME_LOSE:
				printf("Noob\n\n");
				endOfGame = TRUE;
				break;
			default:
				break;
		}

	}while(!endOfGame);

	if(close(socketfd) == -1){
		showError("Error while closing the client socket");
	}
		
}
