#include "fileServer.h"
#include "server.h"



int calculateFileSize (char* fileName){

	struct stat st;
	int result;

		if (stat(fileName, &st) == -1){
			printf ("[calculateFileSize] Error while executing stat(%s)\n", fileName);
			result = -1;
		}
		else{
			result = st.st_size;
		}

	return(result);
}


int* getfilesize_1_svc(t_request *argp, struct svc_req *rqstp){

	static int result;

		result = calculateFileSize (argp->fileName);

	return(&result);
}


int *createfile_1_svc(t_request *argp, struct svc_req *rqstp){

	static int result;
	int fd;

		result = 0;
		fd = open(argp->fileName, O_WRONLY | O_TRUNC | O_CREAT, 0777);

		if (fd < 0){
			printf ("[createFile_1] Error while creating(%s)\n", argp->fileName);
			result = -1;
		}
		else
			close (fd);


	return (&result);
}
