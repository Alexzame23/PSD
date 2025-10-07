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
	free(args);

	tSession sesion;
	tPlayer current = player1;
	unsigned int endOfGame = FALSE;	
	unsigned int code;

	receiveMsg(socket1, sesion.player1Name);
	receiveMsg(socket2, sesion.player2Name);

	printf("Initialising a new game with the players %s and %s\n", sesion.player1Name, sesion.player2Name);

	sendMsgToPlayer(socket1, sesion.player2Name);
	sendMsgToPlayer(socket2, sesion.player1Name);

	initSession(&sesion);	
	int currentSocket;
	int opponentSocket;
	unsigned int *currentBet;
	unsigned int *opponentBet;
	unsigned int firstStack;
	unsigned int secondStack;
	unsigned int card;
	int activeSocket, passiveSocket;
	tDeck *activeDeck;
	unsigned int activePoints, points1, points2;

	while(!endOfGame){
		currentSocket = (current == player1) ? socket1 : socket2;
    	opponentSocket = (current == player1) ? socket2 : socket1;
    	currentBet = (current == player1) ? &sesion.player1Bet : &sesion.player2Bet;
    	opponentBet = (current == player1) ? &sesion.player2Bet : &sesion.player1Bet;
    	firstStack = (current == player1) ? sesion.player1Stack : sesion.player2Stack;
    	secondStack = (current == player1) ? sesion.player2Stack : sesion.player1Stack;
		//Bet momento
		//Jug 1
		sendUi(currentSocket, TURN_BET);
		sendUi(currentSocket, firstStack);
		*currentBet = receiveUi(currentSocket);

		while(*currentBet < 1 || *currentBet > MAX_BET || *currentBet > firstStack){
			sendUi(currentSocket, TURN_BET);
			sendUi(currentSocket, firstStack);
			*currentBet = receiveUi(currentSocket);
		}
		sendUi(currentSocket, TURN_BET_OK);
		//Jug 2
		sendUi(opponentSocket, TURN_BET);
		sendUi(opponentSocket, secondStack);
		*opponentBet = receiveUi(opponentSocket);

		while(*opponentBet < 1 || *opponentBet > MAX_BET || *opponentBet > secondStack){
			sendUi(opponentSocket, TURN_BET);
			sendUi(opponentSocket, secondStack);
			*opponentBet = receiveUi(opponentSocket);
		}
		sendUi(opponentSocket, TURN_BET_OK);

		for(int i = 0; i < 2; i++){
            card = getRandomCard(&sesion.gameDeck);
            sesion.player1Deck.cards[sesion.player1Deck.numCards] = card;
            sesion.player1Deck.numCards++;
            
            card = getRandomCard(&sesion.gameDeck);
            sesion.player2Deck.cards[sesion.player2Deck.numCards] = card;
            sesion.player2Deck.numCards++;
        }

		for(int playerTurn = 0; playerTurn < 2; playerTurn++){
            // Determinar quién es el jugador activo y quién el pasivo en este turno
            
            if(playerTurn == 0){
                // En el primer turno, juega quien apostó primero
                activeSocket = currentSocket;
                passiveSocket = opponentSocket;
                activeDeck = (current == player1) ? &sesion.player1Deck : &sesion.player2Deck;
            } else {
                // En el segundo turno, juega el otro
                activeSocket = opponentSocket;
                passiveSocket = currentSocket;
                activeDeck = (current == player1) ? &sesion.player2Deck : &sesion.player1Deck;
            }
            
            // Calcular puntos iniciales del jugador activo
            activePoints = calculatePoints(activeDeck);
            
            // Informar a ambos jugadores del inicio del turno
            sendUi(activeSocket, TURN_PLAY);
            sendUi(activeSocket, activePoints);
            sendDeck(activeSocket, *activeDeck);
            
            sendUi(passiveSocket, TURN_PLAY_WAIT);
            sendUi(passiveSocket, activePoints);
            sendDeck(passiveSocket, *activeDeck);
            
            // Recibir la primera acción del jugador activo
            code = receiveUi(activeSocket);
            
            // Bucle mientras el jugador pida cartas
            while(code == TURN_PLAY_HIT){
                // Obtener una carta aleatoria del mazo de juego
                card = getRandomCard(&sesion.gameDeck);
                
                // Añadir la carta al deck del jugador activo
                activeDeck->cards[activeDeck->numCards] = card;
                activeDeck->numCards++;
                
                // Calcular los nuevos puntos
                activePoints = calculatePoints(activeDeck);
                
                // Determinar el código a enviar según si se pasó de 21
                if(activePoints > GOAL_GAME){
                    code = TURN_PLAY_OUT;
                } else {
                    code = TURN_PLAY;
                }
                
                // Enviar al jugador activo: código, puntos actualizados y deck actualizado
                sendUi(activeSocket, code);
                sendUi(activeSocket, activePoints);
                sendDeck(activeSocket, *activeDeck);
                
                // Enviar al jugador pasivo la misma información para que vea la jugada del rival
                sendUi(passiveSocket, TURN_PLAY_WAIT);
                sendUi(passiveSocket, activePoints);
                sendDeck(passiveSocket, *activeDeck);
                
                // Si el jugador se pasó de 21, termina su turno automáticamente
                if(activePoints > GOAL_GAME){
                    break;
                }
                
                // Recibir la siguiente acción del jugador
                code = receiveUi(activeSocket);
            }
            
    		if(playerTurn == 0){
        		if(code == TURN_PLAY_STAND){
					sendUi(activeSocket, TURN_PLAY_WAIT);
					sendUi(activeSocket, activePoints);
					sendDeck(activeSocket, *activeDeck);
				}
				sendUi(passiveSocket, TURN_PLAY_RIVAL_DONE);
    		}
        }

        //DETERMINAR GANADOR DE LA MANO Y ACTUALIZAR FICHAS
        
        points1 = calculatePoints(&sesion.player1Deck);
        points2 = calculatePoints(&sesion.player2Deck);
        
        // Lógica para determinar el ganador
        if(points1 > GOAL_GAME && points2 > GOAL_GAME){
            // Ambos se pasaron: EMPATE - cada uno mantiene sus fichas
            // No hay cambio en los stacks
        } 
        else if(points1 > GOAL_GAME){
            // Solo jugador 1 se pasó: GANA jugador 2
            sesion.player1Stack -= sesion.player1Bet;
            sesion.player2Stack += sesion.player1Bet;
        } 
        else if(points2 > GOAL_GAME){
            // Solo jugador 2 se pasó: GANA jugador 1
            sesion.player2Stack -= sesion.player2Bet;
            sesion.player1Stack += sesion.player2Bet;
        } 
        else if(points1 > points2){
            // Ninguno se pasó y jugador 1 tiene más puntos: GANA jugador 1
            sesion.player2Stack -= sesion.player2Bet;
            sesion.player1Stack += sesion.player2Bet;
        } 
        else if(points2 > points1){
            // Ninguno se pasó y jugador 2 tiene más puntos: GANA jugador 2
            sesion.player1Stack -= sesion.player1Bet;
            sesion.player2Stack += sesion.player1Bet;
        }
        // Si points1 == points2: EMPATE - no hay cambio en los stacks

        // VERIFICAR SI HAY UN GANADOR FINAL
        
        if(sesion.player1Stack == 0){
            // Jugador 1 se quedó sin fichas: pierde la partida
            sendUi(socket1, TURN_GAME_LOSE);
            sendUi(socket2, TURN_GAME_WIN);
            endOfGame = TRUE;
        } 
        else if(sesion.player2Stack == 0){
            // Jugador 2 se quedó sin fichas: pierde la partida
            sendUi(socket1, TURN_GAME_WIN);
            sendUi(socket2, TURN_GAME_LOSE);
            endOfGame = TRUE;
        }
        else {
            //PREPARAR LA SIGUIENTE MANO
            
            // Limpiar los decks de los jugadores para la próxima mano
            clearDeck(&sesion.player1Deck);
            clearDeck(&sesion.player2Deck);
            
            // Reinicializar el mazo de juego con todas las cartas
            initDeck(&sesion.gameDeck);
            
            // Cambiar el turno: quien apostó segundo ahora apostará primero
            current = getNextPlayer(current);
            
            // Resetear las apuestas a 0
            sesion.player1Bet = 0;
            sesion.player2Bet = 0;
        }

	}

	close(socket1);
    close(socket2);

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
