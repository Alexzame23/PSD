#include "serverGame.h"
#include <pthread.h>

void sendMsgToPlayer(int socketClient, char *message){	
	int l = strlen(message);
	if(send(socketClient, &l, sizeof(l), 0) < 0)
		showError("Error al enviar la longitud del mensaje");
	if(send(socketClient, message, l, 0) < 0)
		showError("Error al enviar el mensaje");
}

void sendUi(int socketClient, unsigned int code){
	if(send(socketClient, &code, sizeof(code), 0) < 0)
		showError("Error al enviar el code");
}

void sendDeck(int socketClient, tDeck deck){
	int l = deck.numCards * sizeof(unsigned int);
	if(send(socketClient, &deck.numCards, sizeof(deck.numCards), 0) < 0)
		showError("Error sending the size of the deck");
	if(send(socketClient, deck.cards, l, 0) < 0)
		showError("Error sending the deck");
}

void receiveMsg(int socketClient, char *message){
	int l;
	memset(message,0,STRING_LENGTH);
	if(recv(socketClient, &l, sizeof(l), 0) <= 0)
		showError("Error al recibir la longitud del mensaje");
	if(recv(socketClient, message, l, 0) <= 0)
		showError("Error al recibir el mensaje");
}

unsigned int receiveUi (int socket){
	unsigned int l;
	if(recv(socket,&l,sizeof(l),0) <= 0)
		showError("Error al recibir el code");
	return l;
}

tPlayer getNextPlayer (tPlayer currentPlayer){

	tPlayer next;

		if (currentPlayer == player1)
			next = player2;
		else
			next = player1;

	return next;
}

int currentSocket(tPlayer current, int s1, int s2){
	return (current == 0) ? s1 : s2;
}

void swapPlayers(tPlayer current, tDeck **deck, unsigned int **stack, int **s1, int **s2, tSession *sesion){
	if(current == 0){
		int *aux = *s1;
		*s1 = *s2;
		*s2 = aux;
		*deck = &sesion->player1Deck;
		*stack = &sesion->player1Stack;
	}
	else{
		int *aux = *s2;
		*s2 = *s1;
		*s1 = aux;
		*deck = &sesion->player2Deck;
		*stack = &sesion->player2Stack;
	}
}

void initDeck (tDeck *deck){

	deck->numCards = DECK_SIZE; 

	for (int i=0; i<DECK_SIZE; i++){
		deck->cards[i] = i;
	}
}

void clearDeck (tDeck *deck){

	// Set number of cards
	deck->numCards = 0;

	for (int i=0; i<DECK_SIZE; i++){
		deck->cards[i] = UNSET_CARD;
	}
}

void printSession (tSession *session){

		printf ("\n ------ Session state ------\n");

		// Player 1
		printf ("%s [bet:%d; %d chips] Deck:", session->player1Name, session->player1Bet, session->player1Stack);
		printDeck (&(session->player1Deck));

		// Player 2
		printf ("%s [bet:%d; %d chips] Deck:", session->player2Name, session->player2Bet, session->player2Stack);
		printDeck (&(session->player2Deck));

		// Current game deck
		if (DEBUG_PRINT_GAMEDECK){
			printf ("Game deck: ");
			printDeck (&(session->gameDeck));
		}
}

void initSession (tSession *session){

	clearDeck (&(session->player1Deck));
	session->player1Bet = 0;
	session->player1Stack = INITIAL_STACK;

	clearDeck (&(session->player2Deck));
	session->player2Bet = 0;
	session->player2Stack = INITIAL_STACK;

	initDeck (&(session->gameDeck));
}

unsigned int calculatePoints (tDeck *deck){

	unsigned int points;

		// Init...
		points = 0;

		for (int i=0; i<deck->numCards; i++){

			if (deck->cards[i] % SUIT_SIZE < 9)
				points += (deck->cards[i] % SUIT_SIZE) + 1;
			else
				points += FIGURE_VALUE;
		}

	return points;
}

unsigned int getRandomCard (tDeck* deck){

	unsigned int card, cardIndex, i;

		// Get a random card
		cardIndex = rand() % deck->numCards;
		card = deck->cards[cardIndex];

		// Remove the gap
		for (i=cardIndex; i<deck->numCards-1; i++)
			deck->cards[i] = deck->cards[i+1];

		// Update the number of cards in the deck
		deck->numCards--;
		deck->cards[deck->numCards] = UNSET_CARD;

	return card;
}


void *gameThread(void* args){
	tThreadArgs* threadArgs = (tThreadArgs*) args;
	int socket1 = threadArgs->socketPlayer1;
	int socket2 = threadArgs->socketPlayer2;
	int *currentS = &socket1;
	int *otherS = &socket2;
	free(args);
	tSession sesion;
	tPlayer current = player1;
	tDeck *currentDeck = &sesion.player1Deck;
	unsigned int *currentStack = &sesion.player1Stack;
	unsigned int endOfGame = FALSE;	
	unsigned int code;

	receiveMsg(socket1, sesion.player1Name);
	receiveMsg(socket2, sesion.player2Name);

	printf("Initialising a new game with the players %s and %s\n", sesion.player1Name, sesion.player2Name);

	sendMsgToPlayer(socket1, sesion.player2Name);
	sendMsgToPlayer(socket2, sesion.player1Name);

	initSession(&sesion);	

	//**Bet Stage**
	//Player 1
	sendUi(*currentS, TURN_BET);
	sendUi(*currentS, sesion.player1Stack);
	sesion.player1Bet = receiveUi(*currentS);
	while(sesion.player1Bet < 1 || sesion.player1Bet > MAX_BET || sesion.player1Stack<sesion.player1Bet){
		sendUi(*currentS, TURN_BET);
		sesion.player1Bet = receiveUi(*currentS);
	}
	sendUi(*currentS, TURN_BET_OK);
	//Player 2
	sendUi(*otherS, TURN_BET);
	sendUi(*otherS, sesion.player2Stack);
	sesion.player2Bet = receiveUi(*otherS);
	while(sesion.player2Bet < 1 || sesion.player2Bet > MAX_BET || sesion.player2Stack<sesion.player2Bet){
		sendUi(*otherS, TURN_BET);
		sesion.player2Bet = receiveUi(*otherS);
	}
	sendUi(*otherS, TURN_BET_OK);

	for(int i = 0; i < 2; i++){
    unsigned int card = getRandomCard(&sesion.gameDeck);
    sesion.player1Deck.cards[sesion.player1Deck.numCards] = card;
    sesion.player1Deck.numCards++;
    
    card = getRandomCard(&sesion.gameDeck);
    sesion.player2Deck.cards[sesion.player2Deck.numCards] = card;
    sesion.player2Deck.numCards++;
	}
	
	//**Play stage**
	for(int i = 0; i < 2; ++i){
		sendUi(*currentS, TURN_PLAY);
		sendUi(*otherS, TURN_PLAY_WAIT);

		sendUi(*currentS, calculatePoints(currentDeck));
		sendDeck(*currentS, *currentDeck);

		code = receiveUi(*currentS);

		while(code == TURN_PLAY_HIT){

		}

		sendUi(*otherS, TURN_PLAY_RIVAL_DONE);
		current = getNextPlayer(current);
		swapPlayers(current, &currentDeck, &currentStack, &currentS, &otherS, &sesion);
	}
	return NULL;	
}



int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	struct sockaddr_in serverAddress;	/** Server address structure */
	unsigned int port;					/** Listening port */
	struct sockaddr_in player1Address;	/** Client address structure for player 1 */
	struct sockaddr_in player2Address;	/** Client address structure for player 2 */
	int socketPlayer1;					/** Socket descriptor for player 1 */
	int socketPlayer2;					/** Socket descriptor for player 2 */
	unsigned int clientLength;			/** Length of client structure */
	tThreadArgs *threadArgs; 			/** Thread parameters */
	pthread_t threadID;					/** Thread ID */


	// Seed
	srand(time(0));

	// Check arguments
	if (argc != 2) {
		fprintf(stderr,"ERROR wrong number of arguments\n");
		fprintf(stderr,"Usage:\n$>%s port\n", argv[0]);
		exit(1);
	}

	socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	//Creamos el descriptor del socket

	if(socketfd < 0)										//Comprobamos que se creo correctamente
		showError("Error while opening socket");

	memset(&serverAddress, 0, sizeof(serverAddress));		//Iniciamos vacia la estructura del servidor

	port = atoi(argv[1]);

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);

	if(bind(socketfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
		showError("Error while binding");
	}

	if(listen(socketfd, 150) < 0){
		showError("Error while listening");
	}

	clientLength = sizeof(struct sockaddr_in);

	while(1){

		socketPlayer1 = accept(socketfd, (struct sockaddr *)&player1Address, &clientLength);
		if(socketPlayer1 < 0){
			showError("Error while opening socket");
		}

		socketPlayer2 = accept(socketfd, (struct sockaddr *)&player2Address, &clientLength);
		if(socketPlayer2 < 0){
			showError("Error while opening socket");
		}

		threadArgs = (tThreadArgs*)malloc(sizeof(tThreadArgs));
        if (threadArgs == NULL) {
            showError("Error while saving memory for the thread");
        }

		threadArgs->socketPlayer1 = socketPlayer1;
		threadArgs->socketPlayer2 = socketPlayer2;

		if(pthread_create(&threadID, NULL, gameThread, (void*)threadArgs) != 0){
			showError("Error creating the game thread");
		}

		pthread_detach(threadID);
			
	}	

	return 0;	
}
