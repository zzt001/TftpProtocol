#include    <stdio.h>
#include    <sys/socket.h>
#include    <arpa/inet.h>
#include 	<strings.h>
#include 	<stdlib.h>
#include 	<string.h>
#include 	<unistd.h>
#include    "tftp.h"
	

void handleError(char *errorPacket);

int main(int argc, char*argv[]){
	int sock;
	struct sockaddr_in servAddr;
	struct sockaddr_in fromAddr;
	unsigned short servPort = SERV_PORT;
	unsigned int fromSize;
	char *servIP;
	char MsgBuffer[BUF_SIZE]; //buffer for receiving packet
	char filename[FILENAME_MAX];
	char * opRead = "-r";
	char *opWrite = "-w";
	char *packet;
	char* mode = MODE;
	int packetLen;
	int receiveLen;
	int done =0;

	/* check command line argument*/
	if(argc <3 || strcmp(argv[1],opRead)!=0 || strcmp(argv[1],opWrite)!=0 ){
		fprintf(stderr,"Usage: %s <-r/-w> <filename>\n", argv[0]);
		exit(1);
	}
	if(strchr(argv[2],'/') != NULL){
		fprintf(stderr,"Request reject. Filename should not be full pathname!\n");
		exit(1);
	}
	filename = argv[2];
	if(strlen(filename) > FILENAME_MAX){
		fprintf(stderr,"Filename too long. Must be within 128\n");
		exit(1);
	}


	/* create and intialize udp socket */

	servIP = SERV_LOCALHOST;
	if((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		fprintf(stderr,"socket() failed\n");
		exit(1);
	}

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(servIP);
	servAddr.sin_port = htons(servPort);



	/* check the write \ read request */
	// if it's read request
    if (strcmp(argv[1],opRead)==0){
	/* add opcode field 
	   need to convert to network short
	*/
    	//create read request
	   packet = create_request(RRQ,filename,mode);

    	// send the request
    	printf("Sending [Read request]\n");
    	if(sendto(sock, packet, packetLen,0,(struct sockaddr *)&servAddr, sizeof(servAddr))!= packetLen){
    		fprintf(stderr,"sendto() sent a different number of bytes than expected\n");
    	}

    	FILE* file = fopen( filename, "wb" );
    	unsigned short next_block = 1;
    	unsigned short packet_block;
    	unsigned short ack_block=1;
    	// transfer file from server to client
    	while(!done){
    		fromSize = sizeof(fromAddr);
    		receiveLen = recvfrom(sock, MsgBuffer , BUF_SIZE , 0, (struct sockaddr *)&fromAddr,&fromSize);
    		if(servAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr){
    			fprintf(stderr,"Error: received a packet from unknown source.\n");
    			fclose(filename);
    			exit(1);
    		}
    		// get opcode
    		unsigned short opcode;
    		memcpy(&opcode, MsgBuffer, 2);
    		opcode = ntohs(opcode);
    		// if error 
    		if(opcode == ERROR) {
    			handleError(MsgBuffer);
    			fclose(filename);
    			exit(1);
    		}

    		else if( opcode == DATA ){
    			// get block number of this packet
    			memcpy(&packet_block,MsgBuffer+2,2);
    			packet_block = ntohs(packet_block);

    			if(packet_block > next_block){
    				fprintf(stderr,"unordered data packet received. Session terminated\n");
    				fclose(filename);
    				exit(1);
    			}
    			// duplicate packet received, we retransmit last ack message
    			if(packet_block != next_block){
    					printf("Received block #%u of data, which is duplicated.\n",packet_block);

    					ack_block = packet_block;
    			}
    			else{
    				// determine the data length
    				if(receiveLen-4 >MAX_DATA_SIZE) {
    					fprintf(stderr,"data segment too big. Session terminated\n");
    					fclose(filename);
    					exit(1);
    				}
    				printf("Received block #%u of data.\n",packet_block);
    				fwrite(MsgBuffer+4,1,receiveLen-4, file);

    				ack_block = next_block;
    				next_block++;


    				//if the block is the last one
    				if(receiveLen-4 < MAX_DATA_SIZE){
    					done = 1;
    				}
    			}


    			//send ack to sender
    			char *ack_packet = create_ack(ack_block);
    			printf("Sending Ack #%u",ack_block);

    			if(sendto(sock, ack_packet, ACK_LEN,0,(struct sockaddr *)&servAddr, sizeof(servAddr))!= ACK_LEN){
    				fprintf(stderr,"sendto() sent a different number of bytes than expected\n");
    			}





    		}
    		else{
    			fprintf(stderr,"Unknown packet received. Session terminated\n");
    			exit(1);
    		}

    	}

    	
    	//when read ends
    	printf("Total transmitting blocks: %u",next_block-1);
    	fclose(file);


    }


    //write request
    else{
    	packet = create_request(WRQ,filename,mode);

    }


    // send the request





}