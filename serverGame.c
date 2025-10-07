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
/*
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
}*/

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

void sendGameState(int socket, unsigned int code, unsigned int points, tDeck deck){
    sendUi(socket, code);
    sendUi(socket, points);
    sendDeck(socket, deck);
}

void handlePlayerBet(int socket, unsigned int stack, unsigned int *bet){
    sendUi(socket, TURN_BET);
    sendUi(socket, stack);
    *bet = receiveUi(socket);
    
    // Validar que la apuesta sea correcta
    while(*bet < 1 || *bet > MAX_BET || *bet > stack){
        sendUi(socket, TURN_BET);
        sendUi(socket, stack);
        *bet = receiveUi(socket);
    }
    
    sendUi(socket, TURN_BET_OK);
}

void dealInitialCards(tSession *session){
	unsigned int card;
    for(int i = 0; i < 2; i++){
        card = getRandomCard(&session->gameDeck);
        session->player1Deck.cards[session->player1Deck.numCards] = card;
        session->player1Deck.numCards++;
        
        card = getRandomCard(&session->gameDeck);
        session->player2Deck.cards[session->player2Deck.numCards] = card;
        session->player2Deck.numCards++;
    }
}

void playPlayerTurn(int activeSocket, int passiveSocket, tDeck *activeDeck, tDeck *gameDeck, unsigned int isFirstTurn){
    unsigned int activePoints = calculatePoints(activeDeck);
    unsigned int code;
	unsigned int card;
    
    // Informar a ambos jugadores del inicio del turno
    sendGameState(activeSocket, TURN_PLAY, activePoints, *activeDeck);
    sendGameState(passiveSocket, TURN_PLAY_WAIT, activePoints, *activeDeck);
    
    // Recibir la primera acción del jugador
    code = receiveUi(activeSocket);
    
    // Bucle mientras el jugador pida cartas
    while(code == TURN_PLAY_HIT){
        // Obtener una carta aleatoria y añadirla al deck del jugador
        card = getRandomCard(gameDeck);
        activeDeck->cards[activeDeck->numCards] = card;
        activeDeck->numCards++;
        
        // Calcular los nuevos puntos
        activePoints = calculatePoints(activeDeck);
        
        // Determinar el código según si se pasó de 21
        unsigned int codeToSend = (activePoints > GOAL_GAME) ? TURN_PLAY_OUT : TURN_PLAY;
        
        // Enviar información actualizada a ambos jugadores
        sendGameState(activeSocket, codeToSend, activePoints, *activeDeck);
        sendGameState(passiveSocket, TURN_PLAY_WAIT, activePoints, *activeDeck);
        
        // Si el jugador se pasó de 21, terminar el turno
        if(activePoints > GOAL_GAME){
            break;
        }
        
        // Recibir la siguiente acción del jugador
        code = receiveUi(activeSocket);
    }
    
    // Solo enviar mensajes de cambio de turno después del primer turno
    if(isFirstTurn){
        if(code == TURN_PLAY_STAND){
            sendGameState(activeSocket, TURN_PLAY_WAIT, activePoints, *activeDeck);
        }
        // Informar al jugador pasivo que el rival terminó
        sendUi(passiveSocket, TURN_PLAY_RIVAL_DONE);
    }
}

unsigned int updateChipsAndCheckWinner(tSession *session){
    unsigned int points1 = calculatePoints(&session->player1Deck);
    unsigned int points2 = calculatePoints(&session->player2Deck);
    
    // Determinar ganador y actualizar fichas según las reglas del juego
    if(points1 > GOAL_GAME && points2 > GOAL_GAME){
        // Empate: ambos se pasaron, no hay cambio en fichas
    } 
    else if(points1 > GOAL_GAME){
        // Gana jugador 2: jugador 1 se pasó
        session->player1Stack -= session->player1Bet;
        session->player2Stack += session->player1Bet;
    } 
    else if(points2 > GOAL_GAME){
        // Gana jugador 1: jugador 2 se pasó
        session->player2Stack -= session->player2Bet;
        session->player1Stack += session->player2Bet;
    } 
    else if(points1 > points2){
        // Gana jugador 1: tiene más puntos sin pasarse
        session->player2Stack -= session->player2Bet;
        session->player1Stack += session->player2Bet;
    } 
    else if(points2 > points1){
        // Gana jugador 2: tiene más puntos sin pasarse
        session->player1Stack -= session->player1Bet;
        session->player2Stack += session->player1Bet;
    }
    // Si points1 == points2: empate, no hay cambio en fichas
    
    // Verificar si hay un ganador final (alguien se quedó sin fichas)
    return (session->player1Stack == 0 || session->player2Stack == 0);
}

void prepareNextHand(tSession *session, tPlayer *current){
    // Limpiar los decks de ambos jugadores
    clearDeck(&session->player1Deck);
    clearDeck(&session->player2Deck);
    
    // Reinicializar el mazo de juego con todas las cartas
    initDeck(&session->gameDeck);
    
    // Cambiar el turno: quien apostó segundo ahora apostará primero
    *current = getNextPlayer(*current);
    
    // Resetear las apuestas a cero
    session->player1Bet = 0;
    session->player2Bet = 0;
}


void *gameThread(void* args){
    // Extraer argumentos y liberar memoria
    tThreadArgs* threadArgs = (tThreadArgs*) args;
    int socket1 = threadArgs->socketPlayer1;
    int socket2 = threadArgs->socketPlayer2;
    free(args);

    // Inicializar variables de la sesión de juego
    tSession sesion;
    tPlayer current = player1;
    unsigned int endOfGame = FALSE;
	int currentSocket;
	int opponentSocket;
	unsigned int *currentBet;
	unsigned int *opponentBet;
	unsigned int firstStack;
	unsigned int secondStack;
	tDeck *firstPlayerDeck;
	tDeck *secondPlayerDeck;

    // Fase inicial
    receiveMsg(socket1, sesion.player1Name);
    receiveMsg(socket2, sesion.player2Name);

    printf("Initialising a new game with the players %s and %s\n", 
           sesion.player1Name, sesion.player2Name);

    sendMsgToPlayer(socket1, sesion.player2Name);
    sendMsgToPlayer(socket2, sesion.player1Name);

    // Inicializar la sesión
    initSession(&sesion);

    // Bucle principal del juego
    while(!endOfGame){
        // Determinar sockets y variables según quién tiene el turno actual
        currentSocket = (current == player1) ? socket1 : socket2;
        opponentSocket = (current == player1) ? socket2 : socket1;
        currentBet = (current == player1) ? &sesion.player1Bet : &sesion.player2Bet;
        opponentBet = (current == player1) ? &sesion.player2Bet : &sesion.player1Bet;
        firstStack = (current == player1) ? sesion.player1Stack : sesion.player2Stack;
        secondStack = (current == player1) ? sesion.player2Stack : sesion.player1Stack;

        // Fase de apuestas
        handlePlayerBet(currentSocket, firstStack, currentBet);
        handlePlayerBet(opponentSocket, secondStack, opponentBet);

        // Repartir dos cartas iniciales a cada jugador
        dealInitialCards(&sesion);

        // Fase de juego
        firstPlayerDeck = (current == player1) ? &sesion.player1Deck : &sesion.player2Deck;
        secondPlayerDeck = (current == player1) ? &sesion.player2Deck : &sesion.player1Deck;
        
        playPlayerTurn(currentSocket, opponentSocket, firstPlayerDeck, &sesion.gameDeck, TRUE);
        playPlayerTurn(opponentSocket, currentSocket, secondPlayerDeck, &sesion.gameDeck, FALSE);

        // Determinar ganador de la mano y actualizar fichas
        if(updateChipsAndCheckWinner(&sesion)){
            // Hay un ganador final
            if(sesion.player1Stack == 0){
                sendUi(socket1, TURN_GAME_LOSE);
                sendUi(socket2, TURN_GAME_WIN);
            } else {
                sendUi(socket1, TURN_GAME_WIN);
                sendUi(socket2, TURN_GAME_LOSE);
            }
            endOfGame = TRUE;
        } else {
            // Preparar la siguiente mano
            prepareNextHand(&sesion, &current);
        }
    }

    // Cerrar los sockets al finalizar la partida
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
