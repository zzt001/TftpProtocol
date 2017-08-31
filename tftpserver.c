#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include "tftp.h"

// number of timeout
int tries = 0;

//some declaration
void CatchAlarm(int ignored);


void DieWithError(char *errorMessage);

char *tftp_errors[]={
	"Not defined, see error message (if any).",
	"File not found.",
	"Access violation.",
	"Disk full or allocation exceeded.",
	"Illegal TFTP operation.",
	"Unknown transfer ID.",
	"File already exists",
	"Server is busy serving another client. Please wait."
};



int main(int argc, char *argv[]){
	int sock;         /*Socket*/
	struct sockaddr_in servAddr;   /*local address*/
	struct sockaddr_in ClntAddr;    /* store current_serving client address */
	unsigned int cliAddrLen;   
	struct sockaddr_in fromAddr;    /* source address */
	unsigned int fromSize;    
	char receive_buffer[BUF_SIZE];  // Buffer for data
	unsigned short servPort;    //  server port
	int recvMsgSize;  // size of received message
    unsigned short opcode;
	char filename[FILENAME_MAX];
	char* mode = MODE;
	char err_packet[ERROR_MAX];
	struct sigaction myAction;
    char* operand  = "&";
    unsigned short next_block;
    unsigned short last_block;
    unsigned short ack_block;
    unsigned short receive_block;
    char buf[BUF_SIZE];
    int sizeReadIn;
    int done = 0;
    

	// we use serve to determine if server is currently serving client
	// and curr_clntAddr to store that client address
	/*int serve = 0;
	struct curr_clntAddr;*/
	if ( argc != 1 ){
		fprintf(stderr, "Usage: %s \n", argv[0]);
        
		exit(1);
    }
    

	servPort = SERV_PORT;

	if ((sock = socket(PF_INET,SOCK_DGRAM, IPPROTO_UDP ))< 0)
		DieWithError("socket() failed");

	// construct local address structure
	memset(&servAddr, 0 , sizeof(servAddr));
	servAddr.sin_family = AF_INET; //internet address family
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servAddr.sin_port = htons(servPort);

	//bind to the local address
	if(bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr))<0)
		DieWithError("bind() failed");


    /* Set signal handler for alarm signal */ 
    myAction.sa_handler = CatchAlarm;

    if (sigfillset(&myAction.sa_mask) < 0) {/* block everything in handler */ 
        fprintf(stderr, "sigfillset () failed") ;
        exit(1);
    }
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0) {
        fprintf(stderr,"sigaction() failed for SIGALP~") ;
    }


    





	while(1){
		printf("--------------------Server is running-----------------\n\n\n");
		fromSize = sizeof(fromAddr);
		// receive request msg from client
		if((recvMsgSize = recvfrom(sock,receive_buffer,BUF_SIZE,0,(struct sockaddr*)&fromAddr,&fromSize)) < 0)
			DieWithError("recvfrom() failed");



		//get the opcode from packet
		memcpy(&opcode, receive_buffer, 2);
		opcode = ntohs(opcode);


		// we handle read request
		if(opcode == RRQ){
            
            //keep count the transmitting blocks
            unsigned long total=0;
            
            
			/* we store current client address */
			memset(&ClntAddr,0,sizeof(ClntAddr));
			ClntAddr.sin_family = fromAddr.sin_family;
			ClntAddr.sin_port = fromAddr.sin_port;
			ClntAddr.sin_addr.s_addr = fromAddr.sin_addr.s_addr;

			printf("Handling client %s with [Read request]\n",inet_ntoa(ClntAddr.sin_addr));
			//get filename
			strcpy(filename, receive_buffer+2);

			// check mode
			if(strcmp(mode, receive_buffer+2+strlen(filename)+1)!=0){

				create_Error((unsigned short)0, "Wrong request mode!", err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;

			}
			FILE *file = fopen(filename,"rb");
			// check file exists or not
			if(file ==NULL ){
				create_Error((unsigned short)1, tftp_errors[1], err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;
			}

			// read and transmit file
            last_block = 1;

            

        	//transfer file from server to client

        	/* send first block */
        	sizeReadIn = fread(buf+4, 1, 512, file);
            create_data(last_block, buf);
            printf("Sending pck #%u\n",last_block);
            if (sendto(sock, buf, 4+sizeReadIn, 0, (struct sockaddr *)&ClntAddr, sizeof(ClntAddr)) != 4+sizeReadIn) {
                fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
                fclose(file);
                exit(1);
			}
            total = 1;


        	while(!done) {

                // wait to receive ack
                fromSize = sizeof(fromAddr);

            	alarm(TIMEOUT_SECS);
            	while(( recvfrom(sock, receive_buffer , BUF_SIZE , 0, (struct sockaddr *)&fromAddr,&fromSize))<0 ){
                /* Alarm went off */
                if (errno == EINTR) {
                    if (tries < MAXTRIES) {
                        printf("timed out, %d more tries...\n", MAXTRIES-tries);
                        printf("Resending pck #%u\n",last_block);
                        if(sendto(sock, buf, 4+sizeReadIn, 0,(struct sockaddr *)&ClntAddr, sizeof(ClntAddr))!= 4+sizeReadIn){
                            fprintf(stderr,"sendto() sent a different number of bytes than expected\n");
                            exit(1);
                        }
                        alarm(TIMEOUT_SECS);
                    }
                    else{
                        printf("Client no response. This Session end.\n");
	                    create_Error((unsigned short)0, "Client No response.", err_packet);
						sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
                        break;
                    }
                }
                // if we receive something from other client at this time, we reject and receive again
                else {
                    DieWithError("Recv() failed\n");
                }
            	}
                if(ClntAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr || ClntAddr.sin_port != fromAddr.sin_port){
                    printf("Receiving another client's request, ignored\n");
                    create_Error((unsigned short)7, tftp_errors[7], err_packet);
                    sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
                    continue;
                }
            	alarm(0);
            	tries=0;



	            //get opt code
	            memcpy(&opcode, receive_buffer, 2);
	            opcode = ntohs(opcode);
	            //if error
	            if(opcode == ERROR) {
	                create_Error((unsigned short)0, "Packet opcode field invalid. Request reject.", err_packet);
					sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
					break;
	            }
	            else if ( opcode == ACK ) {
	                memcpy(&receive_block, receive_buffer+2, 2);
	                receive_block = ntohs(receive_block);

                	printf("Received ACK packet %u\n", receive_block);

                
	                if (receive_block == last_block) {
	       				// send next block#
	                	last_block++;
	                	sizeReadIn = fread(buf+4, 1, 512, file);

	                	if(sizeReadIn == 0){
	                		printf("File successfully transmitted\n");
                            
	                		break;
	                	}
	            		create_data(last_block, buf);
	            		printf("Sending pck #%u\n",last_block);
	            		if (sendto(sock, buf, 4+sizeReadIn, 0, (struct sockaddr *)&ClntAddr, sizeof(ClntAddr)) != 4+sizeReadIn) {
	                		fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
	                		fclose(file);
	                		exit(1);
						}
                        total++;

	                }
	                else if (receive_block > last_block) {
                        // unordered ack packet, end session
                        create_Error((unsigned short)5, tftp_errors[5], err_packet);
                        sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
                        break;
	                }
	                else {
	                    printf("Received duplicate ack #%u. ignored.\n", receive_block);
	                }
	            }
	            else {
                    //illegal operation
                    create_Error((unsigned short)4, tftp_errors[4], err_packet);
                    sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
                    break;
	            }
	        }

        	printf("Total transmitting blocks: %lu", total);
        	fclose(file);


      




		}
		


		
		//handle write request
		else if ( opcode == WRQ ){
			//store client address
			memset(&ClntAddr,0,sizeof(ClntAddr));
			ClntAddr.sin_family = fromAddr.sin_family;
			ClntAddr.sin_port = fromAddr.sin_port;
			ClntAddr.sin_addr.s_addr = fromAddr.sin_addr.s_addr;

			printf("Handling client %s with [Write request]\n",inet_ntoa(ClntAddr.sin_addr));

            unsigned long total = 0;
            

			strcpy(filename, receive_buffer+2);

			// check mode
			if(strcmp(mode, receive_buffer+2+strlen(filename)+1)!=0){

				create_Error((unsigned short)0, "Wrong request mode!", err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;

			}

			//check already existence
			FILE *file = fopen(filename,"rb");
			// check file exists or not
			if (file !=NULL){
				create_Error((unsigned short)6, tftp_errors[6], err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;
			}
			fclose(file);



			//transfer back ACK = 0 when receive the request
			create_ack((unsigned short)0, buf);
			printf("Sending ACK #0\n");
			if (sendto(sock, buf, BUF_SIZE, 0, (struct sockaddr *)&ClntAddr, sizeof(ClntAddr)) != BUF_SIZE) {
                fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
                fclose(file);
                exit(1);
            }

   

        	//open for writing in
        	file = fopen( filename, "wb" );
        	next_block = 1;
   
        	ack_block = 1;

			//begin accepting packets
			while(!done) {

				// if we no longer receive from the client, we end this session
				alarm(WAIT_MAX_SEC );
            	while((recvMsgSize = recvfrom(sock, receive_buffer , BUF_SIZE , 0, (struct sockaddr *)&fromAddr,&fromSize))<0){
                /* Alarm went off */
	                if (errno == EINTR) {
	                    printf("Client no response. Session end.\n");
	                    create_Error((unsigned short)0, "Client No response.", err_packet);
						sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
						break;
	                    
	                }
                    else {
                        DieWithError("Rev() failed");
                    
                	}

            	}
                if(ClntAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr || ClntAddr.sin_port != fromAddr.sin_port){
                    create_Error((unsigned short)7, tftp_errors[7], err_packet);
                    sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
                    continue;
                }

        		alarm(0);
                


    			//get opcode code 
    			unsigned short opcode;
    			memcpy(&opcode, receive_buffer, 2);
    			opcode = ntohs(opcode);

    			//if receive data
    			if ( opcode == DATA ) {
    				memcpy(&receive_block, receive_buffer+2, 2);
    				receive_block = ntohs(receive_block);

    				if (receive_block > next_block) {
    					create_Error((unsigned short)5, tftp_errors[5], err_packet);
						sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
						break;
    				}

    				//duplicate packet received, we retransmit last ack message
    				if(receive_block != next_block){
    					printf("Received block #%u of data, which is duplicated.\n",receive_block);

    					ack_block = receive_block;
    				}
    				else {
    					//determine the data length
    					if (recvMsgSize - 4 > MAX_DATA_SIZE) {
    						create_Error((unsigned short)1, tftp_errors[3], err_packet);
							sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
							break;
    					}
    					printf("Received block #%u of data\n", receive_block);
    					fwrite(receive_buffer+4,1,recvMsgSize-4, file);

    					ack_block = next_block;
    					next_block++;
                        total++;
    					//if the block is the last one
    					if(recvMsgSize - 4 < MAX_DATA_SIZE) {
    						done = 1;
    					}
    				}


    				//send ack to sender
    		
    				create_ack(ack_block, buf);

    				printf("Sending ACK #%u\n", ack_block);

    				if(sendto(sock, buf, BUF_SIZE,0,(struct sockaddr *)&ClntAddr, sizeof(ClntAddr))!= BUF_SIZE){
    					fprintf(stderr,"sendto() sent a different number of bytes than expected\n");
    				}

    			}
                else {
                    //illegal operation
                    create_Error((unsigned short)4, tftp_errors[4], err_packet);
                    sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
                    break;
                }

			}

			//when read ends
			printf("Total receiving blocks: %lu\n",total);
    		fclose(file);


		}
		

		else{
			create_Error((unsigned short)4, tftp_errors[4],err_packet);

		}


         done =0;


        printf("\n\n");



	}

}
void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    tries += 1;
}
void DieWithError(char *errorMessage){
	perror (errorMessage) ;
	exit(1);
}
