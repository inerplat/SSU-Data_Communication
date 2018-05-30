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
	write(fd, (void *)&Tcp_data->Header.data_useage, sizeof(Tcp_data->Header.data_useage));
	write(fd, (void *)&Tcp_data->Header.header_count, sizeof(Tcp_data->Header.header_count));
	for (i = 0; i < Tcp_data->Header.header_count; i++)
	{
		write(fd, (void *)&Tcp_data->Data_info[i].data_type, sizeof(Tcp_data->Data_info[i].data_type));
		write(fd, (void *)&Tcp_data->Data_info[i].data_count, sizeof(Tcp_data->Data_info[i].data_count));
	}
}

int tcp_write(int fd, const struct TCP_DATA *Tcp_data)
{
	int i, j, k, f;
	int file_len;
	char buffer[1024];
	write(fd, (void *)&Tcp_data->Header.data_useage, sizeof(Tcp_data->Header.data_useage));
	write(fd, (void *)&Tcp_data->Header.header_count, sizeof(Tcp_data->Header.header_count));
	for (i = 0; i < Tcp_data->Header.header_count; i++)
	{
		write(fd, (void *)&Tcp_data->Data_info[i].data_type, sizeof(Tcp_data->Data_info[i].data_type));
		write(fd, (void *)&Tcp_data->Data_info[i].data_count, sizeof(Tcp_data->Data_info[i].data_count));
	}

	for (j = 0, k = 0; j < Tcp_data->Header.header_count; j++)
	{
		for (i = 0; i < Tcp_data->Data_info[j].data_count; i++, k++)
		{
			write(fd, (void *)&Tcp_data->Data[k].data_size, sizeof(Tcp_data->Data[k].data_size));
			write(fd, Tcp_data->Data[k].data, Tcp_data->Data[k].data_size);
			if (Tcp_data->Data_info[j].data_type == WAV)
			{
				FILE *file = fopen((char *)Tcp_data->Data[k].data,"rb");
				fseek(file, 0, SEEK_END);
				file_len = ftell(file);
				fseek(file, 0, SEEK_SET);
				write(fd,(void *)&file_len,sizeof(int));
				for (f = 0; f + sizeof(buffer) < file_len; f += sizeof(buffer))
				{
					fread((void *)buffer, sizeof(char), sizeof(buffer), file);
					write(fd, (void *)buffer, sizeof(buffer));
				}
				for (; f < file_len; f++)
				{
					fread((void *)buffer, sizeof(char), 1, file);
					write(fd, (void *)buffer, sizeof(char));
				}
				fclose(file);
			}
		}
	}
	return 0;
}

int tcp_read(int clnt_sock, struct TCP_DATA *Tcp_data) 
{
	int i, j, k;
	int buffer_len, read_len, file_len;
	int data_cnt;
	char buffer[1024];
	FILE *wav_output;
	buffer_len = 0;
	while (buffer_len < sizeof(Tcp_data->Header.data_useage))
	{
		read_len = read(clnt_sock, &Tcp_data->Header.data_useage, sizeof(Tcp_data->Header.data_useage) - buffer_len);
		buffer_len += read_len;
	}
	buffer_len = 0;
	while (buffer_len < sizeof(Tcp_data->Header.header_count))
	{
		read_len = read(clnt_sock, &Tcp_data->Header.header_count, sizeof(Tcp_data->Header.header_count) - buffer_len);
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
				read_len = read(clnt_sock, &Tcp_data->Data_info[i].data_type, sizeof(Tcp_data->Data_info[i].data_type) - buffer_len);
				buffer_len += read_len;
			}
			buffer_len = 0;
			while (buffer_len < sizeof(Tcp_data->Data_info[i].data_count))
			{
				read_len = read(clnt_sock, &Tcp_data->Data_info[i].data_count, sizeof(Tcp_data->Data_info[i].data_count) - buffer_len);
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
					read_len = read(clnt_sock, &Tcp_data->Data[k].data_size, sizeof(int) - buffer_len);
					buffer_len += read_len;
				}
				buffer_len = 0;
				Tcp_data->Data[k].data = (char *)malloc(Tcp_data->Data[k].data_size);
				while (buffer_len < Tcp_data->Data[k].data_size)
				{
					read_len = read(clnt_sock, Tcp_data->Data[k].data, Tcp_data->Data[k].data_size - buffer_len);
					buffer_len += read_len;
				}
				if (Tcp_data->Data_info[j].data_type == WAV)
				{
					wav_output = fopen("receive.wav", "wb");
					buffer_len = 0;
					while (buffer_len < sizeof(int))
					{
						read_len = read(clnt_sock, &file_len, sizeof(int) - buffer_len);
						buffer_len += read_len;
					}

					buffer_len = 0;
					while (buffer_len < file_len)
					{
						read_len = read(clnt_sock, buffer, (sizeof(buffer) < file_len - buffer_len ? sizeof(buffer) : file_len - buffer_len));
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
	int i, data_cnt;
	data_cnt = 0;
	for (i = 0; i < tcp_data->Header.header_count; i++)
		data_cnt += tcp_data->Data_info[i].data_count;

	for (i = 0; i < data_cnt; i++)
	{
		free(tcp_data->Data[i].data);
		free(tcp_data->Data);
	}
	for (i = 0; i < tcp_data->Header.header_count; i++)
		free(tcp_data->Data_info);
}

void protocol_data_print(struct TCP_DATA *Tcp_data)
{
	int i, j, k;
	printf("%u %u\n", Tcp_data->Header.data_useage, Tcp_data->Header.header_count);
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
				printf("WAV : %s\n", (char *)Tcp_data->Data[k].data);
				break;
			}
		}
	}
}
