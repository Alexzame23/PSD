/** Length for buffer */
const MAXSIZE = 4096;

/** Length for tString */
const STRING_LENGTH = 128;
  
/** Type for file names */
typedef char tString [STRING_LENGTH];
 
struct t_request {
	tString fileName;
};
 
 
program FILESERVER {
  version FILESERVER_VER{
		int getFileSize (t_request) = 1;		
		int createFile (t_request) = 2;
  } = 1;
} = 9966;

