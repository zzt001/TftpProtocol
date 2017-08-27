
#include "tftp.h"
void create_ack(unsigned short ack_block, char* ack_buff ){
	unsigned short opcode = ACK;
	opcode = htons(opcode);
	memcpy(ack_buff,&opcode,2);
	memcpy(ack_buff+2,&ack_block,2);
}
char * create_request(unsigned short opcode, char* filename, char* mode){
    char *packet = (char*)calloc(BUF_SIZE,sizeof(char));
    unsigned short op_code = opcode;
    op_code = htons(op_code);
    memcpy(packet,&op_code,2);
    //store the filename
    strcpy(packet+2, filename);

    //store the mode
    strcpy(packet+2+strlen(filename)+1 , mode);

    return packet;

}
void create_data(unsigned short data_block, char* data_buff){
	unsigned short opcode = DATA;
	opcode = htons(opcode);
	memcpy(data_buff,&opcode, 2);
	data_block = htons(data_block);
	memcpy(data_buff+2,&data_block,2);

}

void create_Error(unsigned short errorCode, char* errorMessage, char* error_buff){
	unsigned short opcode = ERROR;
	opcode = htons(opcode);
	memcpy(error_buff,&opcode,2);
	errorCode = htons(errorCode);
	memcpy(error_buff+2,&errorCode,2);
	strcpy(error_buff+4,errorMessage);

}

void CatchAlarm(int ignored){
	tries += i;
}