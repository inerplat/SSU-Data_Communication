#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

//PROTOCOL_HEADER.data_useage
#define SENDCTRL 0x80000000
#define SENDSERV 0x40000000
#define SENDROBO 0x20000000
#define RECVCTRL 0x08000000
#define RECVSERV 0x04000000
#define RECVROBO 0x02000000

#define INIT 0x00000001
#define QUIT 0x00000002
#define LIST 0x00000003

#define INT 0
#define CHAR 1
#define STRING 2
#define WAV 3

struct DATA_INFO {
	unsigned int data_type;
	unsigned int data_count;
};

struct PROTOCOL_HEADER {
	unsigned int data_useage;
	unsigned int header_count;
};

struct PROTOCOL_DATA {
	unsigned int data_size;
	void *data;
};

struct TCP_DATA {
	struct PROTOCOL_HEADER Header;
	struct DATA_INFO *Data_info;
	struct PROTOCOL_DATA *Data;
};

void protocol_data_print(struct TCP_DATA *Tcp_data);
int tcp_write_header(int fd, const struct TCP_DATA *Tcp_data);
int tcp_write(int fd, const struct TCP_DATA *Tcp_data);
int tcp_read(int clnt_sock, struct TCP_DATA *Tcp_data);
void tcp_free(struct TCP_DATA *tcp_data);