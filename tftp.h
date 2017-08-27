#ifndef TFTPHEADER_H
#define TFTPHEADER_H


#define SERV_PORT 6132 //6100+group # 
#define BUF_SIZE 1024 // the maximum buffer size
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
#define ACK_LEN
#define FILENAME_MAX 128

char * tftp_errors[]={
	"Not defined, see error message (if any).",
	"File not found.",
	"Access violation.",
	"Disk full or allocation exceeded.",
	"Illegal TFTP operation.",
	"Unknown transfer ID.",
	"File already exists",
	"No such user."
};

char * create_ack(unsigned short ack_block);
char * create_request(unsigned short opcode);
char * create_data(unsigned short data_block, char* data);
char * create_error(unsigned short errorCode);

#endif