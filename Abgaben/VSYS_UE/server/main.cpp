#include "Mserver.h"

int main(int argc, char **argv)
{
  if( argc < 3 )
  {
    printf("Usage: %s Directory PORT\n" , argv[0]);
    exit(EXIT_FAILURE);
  }

  /*macht ein neues Verzeichnis auf mit angegeben Parameter 
   #include <sys/stat.h> und #include <sys/stat.h> erforderlich*/
  mkdir(argv[1], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  //Erstelle ein Serverobjekt
  MServer server;

  //Server startet mit übergebenen Parametern
  server.startserver(argv);
  
  return 0;
}



