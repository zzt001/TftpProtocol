#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "tftp.h"




void DieWithError(char *errorMessage);

int main(int argc, char *argv[]){
	int sock;         /*Socket*/
	struct sockaddr_in servAddr;   /*local address*/
	struct sockaddr_in ClntAddr;    /* client address */
	unsigned int cliAddrLen;   
	struct sockaddr_in fromAddr;    /* sender's address */
	unsigned int fromSize;    
	char receive_buffer[BUF_SIZE];  // Buffer for data
	unsigned short servPort;    //  server port
	int recvMsgSize;  // size of received message

	char filename[FILENAME_MAX];
	char* mode = MODE;
	char err_packet[ERROR_MAX];

	struct sigaction myAction;

	// we use serve to determine if server is currently serving client
	// and curr_clntAddr to store that client address
	/*int serve = 0;
	struct curr_clntAddr;*/

	if ( argc != 2 || strcmp(argv[1],"&")!=0){
		fprintf(stderr, "Usage: %s &\n", argv[0]);
		exit(0);
	}


	servPort = SERV_PORT;

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


    /* Set signal handler for alarm signal */ 
    myAction.sa_handler = CatchAlarm;

    if (sigfillset(&myAction.sa_mask) < O) {/* block everything in handler */ 
        fprintf(stderr, "sigfillset () failed") ;
        exit(1);
    }
    myAction.sa_flags = O;

    if (sigaction(SlGALRN, &myAction, O) < O) {
        fprintf(stderr,"sigaction() failed for SIGALP~") ;
    }








	while(1){
		fromSize = sizeof(fromAddr);
		// block until receive message from a client
		if((recvMsgSize = recvfrom(sock,receive_buffer,BUF_SIZE,0,(struct sockaddr*)&fromAddr,&fromSize)) < 0)
			DieWithError("recvfrom() failed");

		// if server serves no client, we store this client's address and flip the serve variable
		// else we reject this client since we only support one client any time
		/*if(!serve){
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

		}*/


		printf("Handling client %s\n",inet_ntoa(ClntAddr.sin_addr));

		unsigned short opcode;
		//get the opcode from packet
		memcpy(&opcode, receive_buffer, 2);
		opcode = ntohs(opcode);


		// we handle read request
		if(opcode == RRQ){
			/* we store current client address */

			memset(&ClntAddr,0,sizeof(ClntAddr));
			ClntAddr.sin_family = fromAddr.sin_family;
			ClntAddr.sin_port = fromAddr.sin_port;
			ClntAddr.sin_addr.s_addr = fromAddr.sin_addr.s_addr;

			//get filename
			strcpy(filename, receive_buffer+2);

			// check mode
			if(strcmp(mode, receive_buffer+2+strlen(filename)+1)!=0){

				create_Error((unsigned short)0, "Wrong request mode!", err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;

			}

			// check file exists or not
			if(File *file = fopen(filename, "rb") ==NULL){
				create_Error((unsigned short)1, tftp_errors[1], err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;
			}

			// read and transmit file

        	unsigned short packet_block = 1;
        	unsigned short ack_block ;
        	char buf[BUF_SIZE];
        	int sizeReadIn;
        	int done = 0;
        	int tries = 0;


        	//transfer file from server to client

        	/* send first block */
        	sizeReadIn = fread(buf+4, 1, 512, file);
            create_data(packet_block, buf);
            printf("Sending pck #u\n",packet_block);
            if (sendto(sock, buf, 4+sizeReadIn, 0, (struct sockaddr *)&ClntAddr, sizeof(ClntAddr)) != 4+sizeReadIn) {
                fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
                fclose(file);
                exit(1);
			}


        	while(!done) {

                // wait to receive ack
                fromSize = sizeof(fromAddr);

            	alarm(TIMEOUT_SECS);
            	while((receiveLen = recvfrom(sock, receive_buffer , BUF_SIZE , 0, (struct sockaddr *)&fromAddr,&fromSize))<0 || ClntAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr){
                /* Alarm went off */
                if (errno == EINTR) {
                    if (tries < MAXTRIES) {
                        printf("timed out, %d more tries...\n", MAXTRIES-tries);
                        printf("Resending pck #u\n",packet_block);
                        if(sendto(sock, buf, 4+sizeReadIn, 0,(struct sockaddr *)&ClntAddr, sizeof(ClntAddr))!= 4+sizeReadIn){
                            fprintf(stderr,"sendto() sent a different number of bytes than expected\n");
                            exit(1);
                            alarm(TIMEOUT_SECS);
                        }
                    }
                    else{
                        fprintf(stderr,"No response. Session ends");
                        exit(1);
                    }
                }
                // if we receive something from other client at this time, we reject and receive again
                else if(ClntAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr){
                		create_Error((unsigned short)1, tftp_errors[7], err_packet);
						sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
						continue;
                	}

            	}	
            	alarm(0);



	            //get opt code
	            unsigned short opcode;
	            memcpy(&opcode, receive_buffer, 2);
	            opcode = ntohs(opcode);
	            //if error
	            if(opcode == ERROR) {
	                create_Error((unsigned short)0, "Packet opcode field invalide. Request reject.", err_packet);
					sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&fromAddr,sizeof(fromAddr));
					break;
	            }
	            else if ( opcode == ACK ) {
	                memcpy(&ack_block, receive_buffer+2, 2);
	                ack_block = ntohs(ack_block);

                	printf("Received ACK packet %u\n", ack_block);

                
	                if (ack_block == packet_block) {
	       				// send next block#
	                	packet_block++;
	                	sizeReadIn = fread(buf+4, 1, 512, file);

	                	if(sizeReadIn == 0){
	                		printf("File successfully transmitted");
	                		break;
	                	}
	            		create_data(packet_block, buf);
	            		printf("Sending pck #u\n",packet_block);
	            		if (sendto(sock, buf, 4+sizeReadIn, 0, (struct sockaddr *)&ClntAddr, sizeof(ClntAddr)) != 4+sizeReadIn) {
	                		fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
	                		fclose(file);
	                		exit(1);
						}

	                }
	                else if (ack_block > packet_block) {
	                    fprintf(stderr, "unordered ACK received. Session terminated\n");
	                    fclose(file);
	                    exit(1);
	                }
	                else {
	                    printf("Received duplicate ack #%u. ignored.\n", ack_block);
	                }
	            }
	            else {
	                fprintf(stderr,"Unknown packet received. Session terminated\n");
	                exit(1);
	            }
	        }

        	printf("Total transmitting blocks: %u", packet_block);
        	fclose(file);


      




		}
		


		
		//handle write request
		else if ( opcode == WRQ ){
			//define variables

			//ack_packet in tftpclient.c
			char buf[BUF_SIZE];
			int done = 0;

			printf("Getting [Write request]\n");

			strcpy(filename, receive_buffer+2);

			// check mode
			if(strcmp(mode, receive_buffer+2+strlen(filename)+1)!=0){

				create_Error((unsigned short)0, "Wrong request mode!", err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;

			}

			// check file exists or not
			if (File *file = fopen(filename, "rb") ==NULL){
				create_Error((unsigned short)1, tftp_errors[1], err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;
			}

			strcpy(filename, receive_buffer+2);

			// check mode
			if(strcmp(mode, receive_buffer+2+strlen(filename)+1)!=0){

				create_Error((unsigned short)0, "Wrong request mode!", err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;

			}

			// check file exists or not
			if(File *file = fopen(filename, "rb") ==NULL){
				create_Error((unsigned short)1, tftp_errors[1], err_packet);
				sendto(sock,err_packet,ERROR_MAX,0,(struct sockaddr *)&ClntAddr,sizeof(ClntAddr));
				continue;
			}




			//transfer back ACK = 0 when receive the request
			create_ack(0, buf);
			if (sendto(sock, buf, BUF_SIZE, 0, (struct sockaddr *)&ClntAddr, sizeof(ClntAddr)) != BUF_SIZE) {
                fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
                fclose(file);
                exit(1);
            }
            printf("Sending ACK #0\n");

            alarm(TIMEOUT_SECS);
            while((receiveLen = recvfrom(sock, receive_buffer , BUF_SIZE , 0, (struct sockaddr *)&fromAddr,&fromSize))<0){
                /* Alarm went off */
                if (errno == EINTR) {
                    if (tries < MAXTRIES) {
                        printf("timed out, %d more tries...\n", MAXTRIES-tries);
                        if(sendto(sock, receive_buffer, BUF_SIZE,0,(struct sockaddr *)&ClntAddr, sizeof(servAddr))!= BUF_SIZE){
                            fprintf(stderr,"sendto() sent a different number of bytes than expected\n");
                            exit(1);
                            alarm(TIMEOUT_SECS);
                        }
                    }
                    else{
                        fprintf(stderr,"No response. Session ends");
                        exit(1);
                    }
                }
                else{
                    fprintf(stderr,"Rcvfrom() failed");
                    exit(1);
                }

            }
        	alarm(0);

        	FILE* file = fopen( filename, "wb" );
        	unsigned short next_block = 1;
        	unsigned short packet_block;
        	unsigned short ack_block = 1;

			//begin accepting the rest packets
			while(!done) {
				if(ClntAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr){
    				fprintf(stderr,"Error: received a packet from unknown source.\n");
    				fclose(file);
    				exit(1);
    			}

    			//get opcode code 
    			unsigned short opcode;
    			memcpy(&opcode, receive_buffer, 2);
    			opcode = ntohs(opcode);

    			//if receive data
    			if ( opcode == DATA ) {
    				memcpy(&packet_block, receive_buffer+2, 2);
    				packet_block = ntohs(packet_block);

    				if (packet_block > next_block) {
    					fprintf(stderr,"unordered data packet received. Session terminated\n");
    					fclose(file);
    					exit(1);
    				}

    				//duplicate packet received, we retransmit last ack message
    				if(packet_block != next_block){
    					printf("Received block #%u of data, which is duplicated.\n",packet_block);

    					ack_block = packet_block;
    				}
    				else {
    					//determine the data length
    					if (receiveLen - 4 > MAX_DATA_SIZE) {
    						fprintf(stderr,"data segment too big. Session terminated\n");
    						fclose(file);
    						exit(1);
    					}
    					printf("Received block #%u of data\n", packet_block);
    					fwrite(receive_buffer+4,1,receiveLen-4, file);

    					ack_block = next_block;
    					next_block++;

    					//if the block is the last one
    					if(receiveLen - 4 < MAX_DATA_SIZE) {
    						done = 1;
    					}
    				}


    				//send ack to sender
    				char ack_packet[4];
    				create_ack(ack_block, ack_packet);

    				printf("Sending ACK #%u", ack_block);

    				if(sendto(sock, ack_packet, ACK_LEN,0,(struct sockaddr *)&ClntAddr, sizeof(ClntAddr))!= ACK_LEN){
    					fprintf(stderr,"sendto() sent a different number of bytes than expected\n");
    				}

    				//wait to receive next block
    				fromSize = sizeof(fromAddr);

    				receiveLen = recvfrom(sock, receive_buffer , BUF_SIZE , 0, (struct sockaddr *)&fromAddr,&fromSize);

    			}
    			else {
    				fprintf(stderr,"Unknown packet received. Session terminated\n");
    				exit(1);
    			}

			}

			//when read ends
			printf("Total receiving blocks: %u",next_block-1);
    		fclose(file);


		}
		

		else{
			handleError((unsigned short)4, tftp_errors[4]);

		}







	}

}