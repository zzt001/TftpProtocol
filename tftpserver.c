#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define DATAMAX 512

void DieWithError(char *errorMessage);

int main(int argc, char *argv[]){
	int sock;         /*Socket*/
	struct sockaddr_in servAddr;   /*local address*/
	struct sockaddr_in ClntAddr;    /* client address */
	unsigned int cliAddrLen;   // length of incoming message
	char buffer[DATAMAX];  // Buffer for data
	unsigned short servPort;    //  server port
	int recvMsgSize;  // size of received message

	// we use serve to determine if server is currently serving client
	// and curr_clntAddr to store that client address
	int serve = 0;
	struct curr_clntAddr;

	if ( argc != 2 || argv[1] != '&'){
		fprintf(stderr, "Usage: %s &\n", argv[0]);
		exit(0);
	}


	servPort = 61004;

	if ((sock = socket(PF_INET,SOCK_DGRAM, IPPROTO_UDP ))< 0)
		DieWithError("socket() failed");

	// construct local address structure
	memset(&servAddr, 0 , sizeof(servAddr));
	servAddr.sin_family = AF_INET; //internet address family
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY) 
	servAddr.sin_port = htons(servPort);

	//bind to the local address
	if(bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr))<0)
		DieWithError("bind() failed");

	while(1){
		cliAddrLen = sizeof(ClntAddr);
		// block until receive message from a client
		if((recvMsgSize = recvfrom(sock,Buffer,DATAMAX,0,(struct sockaddr*)&ClntAddr,&cliAddrLen)) < 0)
			DieWithError("recvfrom() failed");

		// if server serves no client, we store this client's address and flip the serve variable
		// else we reject this client since we only support one client any time
		if(!serve){
			serve = 1;
			//copy clntaddress to local
			memset(&curr_clntAddr,0,sizeof(curr_clntAddr));
			curr_clntAddr.sin_family = ClntAddr.sin_family;
			curr_clntAddr.sin_port = ClntAddr.sin_port;
			curr_clntAddr.sin_addr.s_addr = ClntAddr.sin_addr.s_addr;

		}
		else{
			if(curr_clntAddr.sin_port != ClntAddr.sin_port || curr_clntAddr.sin_addr.s_addr != ClntAddr.sin_addr.s_addr)
				DieWithError("server is currently serving someone else. please wait!");

		}
		

		printf("Handling client %s\n",inet_ntoa(ClntAddr.sin_addr));






	}

}