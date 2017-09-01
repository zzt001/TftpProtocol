#ifndef TFTPHEADER_H
#define TFTPHEADER_H


#define SERV_PORT 12345 //61000+group # 
#define BUF_SIZE 516 // the maximum buffer size
#define TIMEOUT 10  
#define MAX_DATA_SIZE 512
#define MODE "octect\0"
/* all opcode mapping*/
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define SERV_LOCALHOST "127.0.0.1"
#define ERROR_MAX 133
#define TIMEOUT_SECS 2
#define WAIT_MAX_SEC 20
#define MAXTRIES 10




void create_ack(unsigned short ack_block, char* ack_buff);
char * create_request(unsigned short opcode, char* filename, char* mode);
void create_data(unsigned short data_block, char* data_buff);

void create_Error(unsigned short errorCode, char* errorMessage, char* error_buff);

#endif
