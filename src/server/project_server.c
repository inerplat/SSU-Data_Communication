#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "protocol.h"
#include "sqlite3.h"

#define BUF_SIZE 100

void error_handling(char *message);
void read_childproc(int sig);
static int callback(void *NotUsed, int argc, char **argv, char **azColName);
static int sendRobotList(void *fd, int argc, char **argv, char **azColName);
static int dbsize(void *count, int argc, char **argv, char **azColName);

int cnt_child_proc = 0;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	
	int fds[2];

	pid_t pid;
	struct sigaction act;
	socklen_t adr_sz;
	int str_len, state;
	char buf[BUF_SIZE];

	struct TCP_DATA tcp_data;

	sqlite3 *SQLiteDB = NULL;
	char *sql;
	char *dbErrMsg = NULL;
	int rc;

	int i;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	if (sqlite3_open("SQLite.db", &SQLiteDB))
	{
		printf("Can't open database : %s\n", sqlite3_errmsg(SQLiteDB));
		sqlite3_close(SQLiteDB);
		SQLiteDB = NULL;
		return 0;
	}
	// 테이블생성
	sqlite3_exec(SQLiteDB,
		"CREATE TABLE robot ( name TEXT(20), pipe INTEGER )",
		callback, 0, &dbErrMsg);
	sqlite3_exec(SQLiteDB,
		"CREATE TABLE controller ( ip INTEGER, pipe INTEGER )",
		callback, 0, &dbErrMsg);

	act.sa_handler = read_childproc;//signal에 동작하는 함수 설정
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	state = sigaction(SIGCHLD, &act, 0);//SIGCHLD : child process가 종료되었을때 동작

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	while (1)
	{
		adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);//client소켓 생성
		if (clnt_sock == -1)
			continue;
		else
			puts("new client connected...");

		pid = fork();//client와 통신을 위한 child process 생성
		pipe(fds);//process간 통신을 위한 PIPE
		if (pid != 0)
		{
			tcp_read(clnt_sock, &tcp_data);//첫 연결
			if (tcp_data.Header.data_useage & SENDROBO)
			{
				if ((tcp_data.Header.data_useage & 0x00ffffff) == INIT)
				{
					sql = sqlite3_mprintf("INSERT INTO robot ( name, pipe ) VALUES ( '%s', %d )", tcp_data.Data[0].data, fds[0]);
					rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
					sqlite3_free(sql);
				}
			}
			else if (tcp_data.Header.data_useage & SENDCTRL)
			{
				if ((tcp_data.Header.data_useage & 0x00ffffff) == INIT)
				{
					sql = sqlite3_mprintf("INSERT INTO controller ( ip, pipe ) VALUES ( %d, %d )", clnt_adr.sin_addr , fds[0]);
					rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
					sqlite3_free(sql);
				}
			}
			tcp_free(&tcp_data);
		}
		if (pid == 0)
		{
			close(serv_sock);//child process는 client의 요청을 받기위한 소켓은 종료한다.
			while (1)
			{
				tcp_read(clnt_sock, &tcp_data);
				if (tcp_data.Header.data_useage & SENDROBO)
				{
					switch(tcp_data.Header.data_useage & 0x00ffffff)
					{
					case QUIT:
						close(clnt_sock);//소켓 종료
						puts("client disconnected...");
						return 0;//child process 종료
						break;
					case INIT:
						sql = sqlite3_mprintf("INSERT INTO robot ( name, pipe ) VALUES ( '%s', %d )", tcp_data.Data[0].data, fds[0]);
						rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
						sqlite3_free(sql);
						break;
					}
				}
				else if (tcp_data.Header.data_useage & SENDCTRL)
				{
					switch (tcp_data.Header.data_useage & 0x00ffffff)
					{
					case QUIT:
						close(clnt_sock);//소켓 종료
						puts("client disconnected...");
						return 0;//child process 종료
						break;
					case LIST:
						tcp_free(&tcp_data);
						tcp_data.Header.data_useage = SENDSERV | RECVCTRL | LIST;
						tcp_data.Header.header_count = 1;
						tcp_data.Data_info = (struct DATA_INFO*)malloc(sizeof(struct DATA_INFO)*tcp_data.Header.header_count);
						tcp_data.Data_info[0].data_type = STRING;
						sqlite3_exec(SQLiteDB, "SELECT COUNT(*) FROM robot", dbsize, &tcp_data.Data_info[0].data_count, &dbErrMsg);
						tcp_write_header(clnt_sock, &tcp_data);
						sqlite3_exec(SQLiteDB, "SELECT * FROM robot", sendRobotList, &clnt_sock, &dbErrMsg);
						break;
					}
				}
			}	
		}
		else
			close(clnt_sock);//서버는 client의 요청만 받고 통신은 하지 않기 때문에 client 통신용 소켓은 종료
	}
	close(serv_sock);//서버 소켓 종료

	//객체해제
	sqlite3_free(dbErrMsg);
	sqlite3_close(SQLiteDB);
	return 0;
}

void read_childproc(int sig)
{
	pid_t pid;
	int status;
	pid = waitpid(-1, &status, WNOHANG);
	printf("removed proc id: %d \n", pid);
	cnt_child_proc++;
}
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (i = 0; i < argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	printf("\n");
	return 0;
};

static int dbsize(void *count, int argc, char **argv, char **azColName)
{
	*(int *)count = atoi(argv[0])*2;
	return 0;
};

static int sendRobotList(void *fd, int argc, char **argv, char **azColName)
{
	int i, data_size;
	char buffer[1024];
	for (i = 0; i < argc; i++)
	{
		data_size = strlen(argv[i]) + 1;
		memcpy(buffer, argv[i], data_size - 1);
		buffer[data_size - 1] = 0;
		write(*(int *)fd, &data_size, sizeof(int));
		write(*(int *)fd, buffer, data_size);
	}
	return 0;
};