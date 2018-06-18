#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "protocol.h"

int tcp_write_header(int fd, const struct TCP_DATA *Tcp_data)
{
	int i;
	if(write(fd, (void *)&Tcp_data->Header.data_useage, sizeof(Tcp_data->Header.data_useage)) < 0)return -1;
	if(write(fd, (void *)&Tcp_data->Header.header_count, sizeof(Tcp_data->Header.header_count)) < 0)return -1;
	for (i = 0; i < Tcp_data->Header.header_count; i++)
	{
		if(write(fd, (void *)&Tcp_data->Data_info[i].data_type, sizeof(Tcp_data->Data_info[i].data_type)) < 0)return -1;
		if(write(fd, (void *)&Tcp_data->Data_info[i].data_count, sizeof(Tcp_data->Data_info[i].data_count)) < 0)return -1;
	}
	return 0;
}

int tcp_write(int fd, const struct TCP_DATA *Tcp_data)
{
	int i, j, k, f;
	int file_len;
	char buffer[1024];
	if(write(fd, (void *)&Tcp_data->Header.data_useage, sizeof(Tcp_data->Header.data_useage)) < 0)return -1;
	if(write(fd, (void *)&Tcp_data->Header.header_count, sizeof(Tcp_data->Header.header_count)) < 0)return -1;
	for (i = 0; i < Tcp_data->Header.header_count; i++)
	{
		if(write(fd, (void *)&Tcp_data->Data_info[i].data_type, sizeof(Tcp_data->Data_info[i].data_type)) < 0)return -1;
		if(write(fd, (void *)&Tcp_data->Data_info[i].data_count, sizeof(Tcp_data->Data_info[i].data_count)) < 0)return -1;
	}

	for (j = 0, k = 0; j < Tcp_data->Header.header_count; j++)
	{
		for (i = 0; i < Tcp_data->Data_info[j].data_count; i++, k++)
		{
			if (Tcp_data->Data_info[j].data_type != WAV)
			{
				if(write(fd, (void *)&Tcp_data->Data[k].data_size, sizeof(Tcp_data->Data[k].data_size)) < 0) return -1;
				if(write(fd, Tcp_data->Data[k].data, Tcp_data->Data[k].data_size) < 0)return -1;
			}
			else if (Tcp_data->Data_info[j].data_type == WAV)
			{
				FILE *file = fopen((char *)Tcp_data->Data[k].data,"rb");
				fseek(file, 0, SEEK_END);
				file_len = ftell(file);
				fseek(file, 0, SEEK_SET);
				if(write(fd,(void *)&file_len,sizeof(int)) < 0)return -1;
				for (f = 0; f + sizeof(buffer) < file_len; f += sizeof(buffer))
				{
					fread((void *)buffer, sizeof(char), sizeof(buffer), file);
					if(write(fd, (void *)buffer, sizeof(buffer)) < 0)return -1;
				}
				for (; f < file_len; f++)
				{
					fread((void *)buffer, sizeof(char), 1, file);
					if(write(fd, (void *)buffer, sizeof(char)) < 0)return -1;
				}
				fclose(file);
			}
		}
	}
	return 0;
}

int tcp_read(int clnt_sock, struct TCP_DATA *Tcp_data, char *file_name) 
{
	int i, j, k;
	int buffer_len, read_len, file_len;
	int data_cnt;
	char buffer[1024];
	FILE *wav_output;
	buffer_len = 0;
	while (buffer_len < sizeof(Tcp_data->Header.data_useage))
	{
		if((read_len = read(clnt_sock, &Tcp_data->Header.data_useage, sizeof(Tcp_data->Header.data_useage) - buffer_len)) < 0)return -1;
		buffer_len += read_len;
	}

	buffer_len = 0;
	while (buffer_len < sizeof(Tcp_data->Header.header_count))
	{
		if((read_len = read(clnt_sock, &Tcp_data->Header.header_count, sizeof(Tcp_data->Header.header_count) - buffer_len)) < 0)return -1;
		buffer_len += read_len;
	}

	if (Tcp_data->Header.header_count > 0)
	{
		Tcp_data->Data_info = (struct DATA_INFO*)malloc(sizeof(struct DATA_INFO)*Tcp_data->Header.header_count);
		data_cnt = 0;
		for (i = 0; i < Tcp_data->Header.header_count; i++)
		{
			buffer_len = 0;
			while (buffer_len < sizeof(Tcp_data->Data_info[i].data_type))
			{
				if((read_len = read(clnt_sock, &Tcp_data->Data_info[i].data_type, sizeof(Tcp_data->Data_info[i].data_type) - buffer_len)) < 0)return -1;
				buffer_len += read_len;
			}
			buffer_len = 0;
			while (buffer_len < sizeof(Tcp_data->Data_info[i].data_count))
			{
				if((read_len = read(clnt_sock, &Tcp_data->Data_info[i].data_count, sizeof(Tcp_data->Data_info[i].data_count) - buffer_len)) < 0)return -1;
				buffer_len += read_len;
			}
			data_cnt += Tcp_data->Data_info[i].data_count;
		}

		Tcp_data->Data = (struct PROTOCOL_DATA*)malloc(sizeof(struct PROTOCOL_DATA)*data_cnt);

		for (j = 0, k = 0; j < Tcp_data->Header.header_count; j++)
		{
			for (i = 0; i < Tcp_data->Data_info[j].data_count; i++, k++)
			{
				buffer_len = 0;
				while (buffer_len < sizeof(int))
				{
					if((read_len = read(clnt_sock, &Tcp_data->Data[k].data_size, sizeof(int) - buffer_len)) < 0)return -1;
					buffer_len += read_len;
				}
				if (Tcp_data->Data_info[j].data_type != WAV)
				{
					buffer_len = 0;
					Tcp_data->Data[k].data = (char *)malloc(Tcp_data->Data[k].data_size);
					while (buffer_len < Tcp_data->Data[k].data_size)
					{
						if((read_len = read(clnt_sock, Tcp_data->Data[k].data, Tcp_data->Data[k].data_size - buffer_len)) < 0) return -1;
						buffer_len += read_len;
					}
				}
				else if (Tcp_data->Data_info[j].data_type == WAV)
				{
					wav_output = fopen(file_name, "wb");
					buffer_len = 0;
					while (buffer_len < Tcp_data->Data[k].data_size)
					{
						if((read_len = read(clnt_sock, buffer, (sizeof(buffer) < Tcp_data->Data[k].data_size - buffer_len ? sizeof(buffer) : Tcp_data->Data[k].data_size - buffer_len))) < 0)return -1;
						fwrite((void *)buffer, sizeof(char), read_len, wav_output);
						buffer_len += read_len;
					}
					fclose(wav_output);
				}
			}
		}
	}
	return 0;
}

void tcp_free(struct TCP_DATA *tcp_data)
{
	int i, j, data_cnt;
	data_cnt = 0;
	for (i = 0; i < tcp_data->Header.header_count; i++)
	{
		if (tcp_data->Data_info[i].data_type != WAV)
		{
			for (j = 0; j < tcp_data->Data_info[i].data_count; j++, data_cnt++)
				free(tcp_data->Data[data_cnt].data);
		}
		else
			data_cnt += tcp_data->Data_info[i].data_count;
	}
	free(tcp_data->Data);
	free(tcp_data->Data_info);
}

void protocol_data_print(struct TCP_DATA *Tcp_data)
{
	int i, j, k;
	printf("========================\n%X ", Tcp_data->Header.data_useage);
	switch (Tcp_data->Header.data_useage & 0xf0000000)
	{
	case SENDCTRL:
		printf("controller->");
		break;
	case SENDSERV:
		printf("server->");
		break;
	case SENDROBO:
		printf("robot->");
		break;
	}
	switch (Tcp_data->Header.data_useage & 0x0f000000)
	{
	case RECVCTRL:
		printf("controller ");
		break;
	case RECVSERV:
		printf("server ");
		break;
	case RECVROBO:
		printf("robot ");
		break;
	}
	switch (Tcp_data->Header.data_useage & 0x00ffffff)
	{
	case INIT:
		printf("INIT");
		break;
	case QUIT:
		printf("QUIT");
		break;
	case LIST:
		printf("LIST");
		break;
	case LINK:
		printf("LINK");
		break;
	case FIN:
		printf("FIN");
		break;
	case CMDV:
		printf("Commend voice");
		break;
	case CMDN:
		printf("Commend number");
		break;
	}
	printf("\n%u\n",Tcp_data->Header.header_count);
	
	for (j = 0, k = 0; j < Tcp_data->Header.header_count; j++)
	{
		for (i = 0; i < Tcp_data->Data_info[j].data_count; i++, k++)
		{
			switch (Tcp_data->Data_info[j].data_type)
			{
			case INT:
				printf("INT : %d\n", *((int *)Tcp_data->Data[k].data));
				break;
			case CHAR:
				printf("CHAR : %c\n", *((char *)Tcp_data->Data[k].data));
				break;
			case STRING:
				printf("STRING : %s\n", (char *)Tcp_data->Data[k].data);
				break;
			case WAV:
				printf("WAV FILE\n");
				break;
			default:
				break;
			}
		}
	}
	printf("========================\n");
}
