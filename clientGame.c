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

void receiveDeck(int socket, tDeck *deck){
    // Primero recibimos cuántas cartas vienen en el deck
    if(recv(socket, &deck->numCards, sizeof(deck->numCards), 0) <= 0)
        showError("Error receiving the number of cards");
    
    // Ahora recibimos exactamente esa cantidad de cartas
    int l = deck->numCards * sizeof(unsigned int);
    
    if(recv(socket, deck->cards, l, 0) <= 0)
        showError("Error receiving the cards");
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
	tString rivalName;


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

	memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(serverIP);
	server_address.sin_port = htons(port);

	if(connect(socketfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
		showError("Error while stablishing connection");
	}

	do{
        memset(playerName, 0, STRING_LENGTH);
        printf("Enter player name: ");
        fgets(playerName, STRING_LENGTH-1, stdin);
        playerName[strlen(playerName)-1] = '\0';
    }while(strlen(playerName) <= 1);

    // Enviar el nombre al servidor
    sendMsgToServer(socketfd, playerName);

    //RECEPCIÓN DEL NOMBRE DEL RIVAL
    printf("Creating the play room\n\n");
    receiveMsg(socketfd, rivalName);
    printf("You are playing against %s\n\n", rivalName);
    printf("Game starts!\n\n");

    endOfGame = FALSE;

    //BUCLE PRINCIPAL DEL JUEGO
    unsigned int stack;
    unsigned int bet;
    unsigned int points;
    tDeck deck;
    unsigned int option;

    while(!endOfGame){
        // Recibir el código que indica el estado actual del juego
        code = receiveUi(socketfd);

        // Actuar según el código recibido
        switch(code){
            case TURN_BET:
                {
                    // Es nuestro turno para apostar
                    stack = receiveUi(socketfd);
                    
                    printf("--- BET STAGE ---\n");
                    printf("You have %u chips. Introduce your bet (1-%d): ", stack, MAX_BET);
                    
                    // Leer la apuesta del jugador usando la función auxiliar
                    bet = readBet();
                    
                    // Enviar la apuesta al servidor
                    sendUi(socketfd, bet);
                    
                    printf("\n");
                }
                break;

            case TURN_BET_OK:
                // El servidor confirmó que nuestra apuesta es correcta
                printf("Your bet was registered correctly\n\n");
                break;

            case TURN_PLAY:
                {
                    // Es nuestro turno para jugar (pedir carta o plantarnos)
                    points = receiveUi(socketfd);
                    receiveDeck(socketfd, &deck);
                    
                    printf("--- YOUR TURN ---\n");
                    printf("Your current points: %u\n", points);
                    printf("Your cards:\n");
                    printFancyDeck(&deck);
                    
                    // Leer la acción del jugador
                    option = readOption();
                    
                    // Enviar la acción al servidor
                    sendUi(socketfd, option);
                    
                    printf("\n");
                }
                break;

            case TURN_PLAY_OUT:
                {
                    // Nos hemos pasado de 21 puntos
                    points = receiveUi(socketfd);
                    receiveDeck(socketfd, &deck);
                    
                    printf("--- BUSTED! ---\n");
                    printf("You exceeded 21 points with %u points\n", points);
                    printf("Your final cards:\n");
                    printFancyDeck(&deck);
                    printf("\n");
                }
                break;

            case TURN_PLAY_WAIT:
                {
                    // Debemos esperar mientras el rival juega
                    points = receiveUi(socketfd);
                    receiveDeck(socketfd, &deck);
                    
                    printf("--- WAITING FOR RIVAL ---\n");
                    printf("Rival's current points: %u\n", points);
                    printf("Rival's cards:\n");
                    printFancyDeck(&deck);
                    printf("\n");
                }
                break;

            case TURN_PLAY_RIVAL_DONE:
                // El rival ha terminado su turno
                printf("Your rival has finished their turn. Now it's your turn\n\n");
                break;

            case TURN_GAME_WIN:
                printf("========================================\n");
                printf("   CONGRATULATIONS, YOU WIN!\n");
                printf("   You're an expert gambling addict!!\n");
                printf("========================================\n\n");
                endOfGame = TRUE;
                break;

            case TURN_GAME_LOSE:
                printf("========================================\n");
                printf("   GAME OVER - YOU LOSE\n");
                printf("   Better luck next time!\n");
                printf("========================================\n\n");
                endOfGame = TRUE;
                break;

            default:
                //printf("Unknown code received: %u\n", code);
                break;
        }
    }

    //CIERRE DEL SOCKET
    if(close(socketfd) == -1){
        showError("Error while closing the client socket");
    }

	return 0;
		
}
