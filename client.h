#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


void showError (char* msg);

void receiveFile (char *host, char* src, char* dst);

void sendFile (char *host, char* src, char* dst);
