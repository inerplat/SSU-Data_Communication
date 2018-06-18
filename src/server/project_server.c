#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include "protocol.h"
#include "sqlite3.h"

#define BUF_SIZE 100

void error_handling(char *message);
static int callback(void *NotUsed, int argc, char **argv, char **azColName);
static int sendRobotList(void *fd, int argc, char **argv, char **azColName);
static int retInt(void *count, int argc, char **argv, char **azColName);
static int retStr(void *str, int argc, char **argv, char **azColName);

void *robot_server(void *arg);
void *controller_server(void *arg);

sqlite3 *SQLiteDB = NULL;

sem_t sem[10];

struct thread_arg {
	int clnt_sock;
	int id;
}t_arg;

int main(int argc, char *argv[])
{
	int rc, i;
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	char cmd[21], type[21];
	int sig, tmp, id;
	FILE *motionfp;

	pthread_t t_id;

	for (i = 0; i < 10; i++)
		sem_init(&sem[i], 0, 1);

	socklen_t adr_sz;
	int str_len, state;
	char buf[BUF_SIZE];

	struct TCP_DATA tcp_data;

	char *sql;
	char *dbErrMsg = NULL;

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

	signal(SIGPIPE, SIG_IGN);
	//테이블 제거
	sqlite3_exec(SQLiteDB,
		"DROP TABLE robot",
		callback, 0, &dbErrMsg);
	sqlite3_exec(SQLiteDB,
		"DROP TABLE controller",
		callback, 0, &dbErrMsg);
	sqlite3_exec(SQLiteDB,
		"DROP TABLE motion",
		callback, 0, &dbErrMsg);

	// 테이블생성
	sqlite3_exec(SQLiteDB,
		"CREATE TABLE robot ( id integer primary key AUTOINCREMENT, name TEXT(20), connected INTEGER, fd INTEGER, type TEXT(20))",
		callback, 0, &dbErrMsg);
	sqlite3_exec(SQLiteDB,
		"CREATE TABLE controller ( id integer primary key AUTOINCREMENT, ip INTEGER )",
		callback, 0, &dbErrMsg);
	sqlite3_exec(SQLiteDB,
		"CREATE TABLE motion ( id INTEGER, cmd TEXT(20), type TEXT(20), signal INTEGER )",
		callback, 0, &dbErrMsg);

	motionfp = fopen("motion.txt", "r");
	while (!feof(motionfp))
	{
		fscanf(motionfp, "%d %s %s %d", &id, cmd, type, &sig);
		sql = sqlite3_mprintf("INSERT INTO motion ( id, cmd, type, signal ) VALUES ( %d, '%s', '%s', %d )", id, cmd, type, sig);
		rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
		sqlite3_free(sql);
	}

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

		tcp_read(clnt_sock, &tcp_data, 0);//첫 연결
		protocol_data_print(&tcp_data);
		if (tcp_data.Header.data_useage & SENDROBO)
		{
			if ((tcp_data.Header.data_useage & 0x00ffffff) == INIT)
			{
				sql = sqlite3_mprintf("INSERT INTO robot ( name, connected, fd, type ) VALUES ( '%s', 0, %d, '%s' )", tcp_data.Data[0].data, clnt_sock, tcp_data.Data[1].data);
				rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
				sqlite3_free(sql);
				t_arg.clnt_sock = clnt_sock;
				sql = sqlite3_mprintf("SELECT id FROM robot WHERE name = '%s'", tcp_data.Data[0].data);
				rc = sqlite3_exec(SQLiteDB, sql, retInt, &t_arg.id, &dbErrMsg);
				sqlite3_free(sql);
				pthread_create(&t_id, NULL, robot_server, (void *)&t_arg);
				pthread_detach(t_id);
			}
		}
		else if (tcp_data.Header.data_useage & SENDCTRL)
		{
			if ((tcp_data.Header.data_useage & 0x00ffffff) == INIT)
			{
				sql = sqlite3_mprintf("INSERT INTO controller ( ip ) VALUES ( %d )", clnt_adr.sin_addr);
				rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
				sqlite3_free(sql);
				t_arg.clnt_sock = clnt_sock;
				sql = sqlite3_mprintf("SELECT id FROM controller WHERE ip = %d", clnt_adr.sin_addr);
				rc = sqlite3_exec(SQLiteDB, sql, retInt, &t_arg.id, &dbErrMsg);
				sqlite3_free(sql);
				pthread_create(&t_id, NULL, controller_server, (void *)&t_arg);
				pthread_detach(t_id);
			}
		}
	}
	close(serv_sock);//서버 소켓 종료

	for (i = 0; i < 10; i++)
		sem_destroy(&sem[i]);
	//객체해제
	sqlite3_free(dbErrMsg);
	sqlite3_close(SQLiteDB);
	return 0;
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
	//	printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	//printf("\n");
	return 0;
};

static int retInt(void *count, int argc, char **argv, char **azColName)
{
	*(int *)count = atoi(argv[0]);
	return 0;
};

static int retStr(void *str, int argc, char **argv, char **azColName)
{
	memcpy(str,argv[0],strlen(argv[0]));
	return 0;
};

static int sendRobotList(void *fd, int argc, char **argv, char **azColName)
{
	int i, data_size;
	char buffer[1024];
	for (i = 0; i < argc; i++)
	{
		printf("%s = %s\n", azColName[i],argv[i]);
		data_size = strlen(argv[i]) + 1;
		memcpy(buffer, argv[i], data_size - 1);
		buffer[data_size - 1] = 0;
		write(*(int *)fd, &data_size, sizeof(int));
		write(*(int *)fd, buffer, data_size);
	}
	printf("\n");
	return 0;
};

void *robot_server(void * arg)
{
	int rc, i;
	struct thread_arg t_a = *(struct thread_arg *)arg;
	int clnt_sock = t_a.clnt_sock;
	int robot_id = t_a.id;
	char *buf;
	char *sql;
	char *dbErrMsg = NULL;
	struct TCP_DATA tcp_data;

	while (1)
	{
		if (tcp_read(clnt_sock, &tcp_data, 0) < 0)//error
		{
			sql = sqlite3_mprintf("DELETE FROM robot WHERE name = '%s'", tcp_data.Data[0].data);
			rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
			sqlite3_free(sql);
			puts("client disconnected...");
			return NULL;
		}
		protocol_data_print(&tcp_data);
		switch (tcp_data.Header.data_useage & 0x00ffffff)
		{
		case QUIT:
			sql = sqlite3_mprintf("DELETE FROM robot WHERE name = '%s'", tcp_data.Data[0].data);
			rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
			sqlite3_free(sql);
			close(clnt_sock);//소켓 종료
			puts("client disconnected...");
			break;
		}
	}

	return NULL;
}

void *controller_server(void *arg)
{
	int rc, i, check, tmp, robot_sock;
	struct thread_arg t_a = *(struct thread_arg *)arg;
	int robot_id, controller_id = t_a.id;
	int clnt_sock = t_a.clnt_sock;
	char robot_type[21];
	char *sql;
	char *dbErrMsg = NULL;
	char *pycmd = "python3 main.py test.wav";
	char *wavfile;
	char pyret[2048];
	int pyretid;
	FILE *pyfp;
	struct TCP_DATA tcp_data;

	wavfile = sqlite3_mprintf("voice%d.wav", controller_id);
	pycmd = sqlite3_mprintf("python3 main.py voice%d.wav", controller_id);
	while (1)
	{
		
		if (tcp_read(clnt_sock, &tcp_data, wavfile) < 0)//error
		{
			sql = sqlite3_mprintf("UPDATE robot SET connected = 0 WHERE connected = %d", controller_id);
			rc = sqlite3_exec(SQLiteDB, sql, callback, robot_type, &dbErrMsg);
			sqlite3_free(sql);
			puts("client disconnected...");
			return NULL;
		}

		protocol_data_print(&tcp_data);

		switch (tcp_data.Header.data_useage & 0x00ffffff)
		{
		case QUIT:
			sql = sqlite3_mprintf("UPDATE robot SET connected = 0 WHERE connected = %d", controller_id);
			rc = sqlite3_exec(SQLiteDB, sql, callback, robot_type, &dbErrMsg);
			sqlite3_free(sql);
			close(clnt_sock);//소켓 종료
			puts("client disconnected...");
			break;
		case LIST:
			tcp_free(&tcp_data);
			tcp_data.Header.data_useage = SENDSERV | RECVCTRL | LIST;
			tcp_data.Header.header_count = 1;
			tcp_data.Data_info = (struct DATA_INFO*)malloc(sizeof(struct DATA_INFO)*tcp_data.Header.header_count);
			tcp_data.Data_info[0].data_type = STRING;
			sql = sqlite3_mprintf("SELECT COUNT(*) FROM robot WHERE connected = 0 OR connected = %d", controller_id);
			sqlite3_exec(SQLiteDB, sql, retInt, &tcp_data.Data_info[0].data_count, &dbErrMsg);
			sqlite3_free(sql);
			tcp_data.Data_info[0].data_count *= 3;
			tcp_write_header(clnt_sock, &tcp_data);
			sql = sqlite3_mprintf("SELECT id, name, connected FROM robot WHERE connected = 0 OR connected = %d", controller_id);
			sqlite3_exec(SQLiteDB, sql, sendRobotList, &clnt_sock, &dbErrMsg);
			sqlite3_free(sql);
			break;
		case SEND:
			robot_id = *(int*)tcp_data.Data[0].data;
			sql = sqlite3_mprintf("SELECT connected FROM robot WHERE id = %d", robot_id);
			rc = sqlite3_exec(SQLiteDB, sql, retInt, &tmp, &dbErrMsg);
			sqlite3_free(sql);
			if (tmp == controller_id)
			{
				sql = sqlite3_mprintf("SELECT fd FROM robot WHERE id = %d", robot_id);
				rc = sqlite3_exec(SQLiteDB, sql, retInt, &robot_sock, &dbErrMsg);
				sqlite3_free(sql);
				sql = sqlite3_mprintf("SELECT type FROM robot WHERE id = %d", robot_id);
				rc = sqlite3_exec(SQLiteDB, sql, retStr, robot_type, &dbErrMsg);
				sqlite3_free(sql);
				sql = sqlite3_mprintf("SELECT signal FROM motion WHERE cmd = '%s' AND type = '%s'", (char *)tcp_data.Data[1].data, robot_type);
				rc = sqlite3_exec(SQLiteDB, sql, retInt, &tmp, &dbErrMsg);
				sqlite3_free(sql);
				tcp_free(&tcp_data);
				tcp_data.Header.data_useage = SENDCTRL | RECVROBO | SEND;
				tcp_data.Header.header_count = 1;
				tcp_data.Data_info = (struct DATA_INFO*)malloc(sizeof(struct DATA_INFO)*tcp_data.Header.header_count);
				tcp_data.Data_info[0].data_type = 0;
				tcp_data.Data_info[0].data_count = 1;
				tcp_data.Data = (struct PROTOCOL_DATA*)malloc(sizeof(struct PROTOCOL_DATA));
				tcp_data.Data[0].data_size = sizeof(int);
				tcp_data.Data[0].data = (int *)malloc(tcp_data.Data[0].data_size);
				*(int *)tcp_data.Data[0].data = tmp;
				sem_wait(&sem[robot_id]);
				tcp_write(robot_sock, &tcp_data);
				sem_post(&sem[robot_id]);
			}
			break;
		case LINK:
			sql = sqlite3_mprintf("UPDATE robot SET connected = %d WHERE id = %d ", controller_id, *(int *)tcp_data.Data[0].data);
			rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
			sqlite3_free(sql);
			break;
		case CMDV:
			pyfp = popen(pycmd, "r");
			pyretid = 0;
			check = 0;
			while (fgets(pyret, 2048, pyfp) != NULL)
			{
				for (i = 0; i < strlen(pyret); i++)
				{
					if (pyret[i] == '#')
						check = 1;
					if (check && pyret[i] >= '0' && pyret[i] <= '9')
						pyretid = pyretid * 10 + pyret[i] - '0';
				}
			}
			pclose(pyfp);
			robot_id = *(int*)tcp_data.Data[0].data;
			sql = sqlite3_mprintf("SELECT connected FROM robot WHERE id = %d", robot_id);
			rc = sqlite3_exec(SQLiteDB, sql, retInt, &tmp, &dbErrMsg);
			sqlite3_free(sql);
			if (tmp == controller_id)
			{
				tmp = 0;
				sql = sqlite3_mprintf("SELECT fd FROM robot WHERE id = %d", robot_id);
				rc = sqlite3_exec(SQLiteDB, sql, retInt, &robot_sock, &dbErrMsg);
				sqlite3_free(sql);
				sql = sqlite3_mprintf("SELECT type FROM robot WHERE id = %d", robot_id);
				rc = sqlite3_exec(SQLiteDB, sql, retStr, robot_type, &dbErrMsg);
				sqlite3_free(sql);
				sql = sqlite3_mprintf("SELECT COUNT(*) FROM motion WHERE id = %d AND type = '%s'", pyretid, robot_type);
				rc = sqlite3_exec(SQLiteDB, sql, retInt, &tmp, &dbErrMsg);
				sqlite3_free(sql);
				tcp_free(&tcp_data);
				if (tmp)
				{
					sql = sqlite3_mprintf("SELECT signal FROM motion WHERE id = %d AND type = '%s'", pyretid, robot_type);
					rc = sqlite3_exec(SQLiteDB, sql, retInt, &tmp, &dbErrMsg);
					sqlite3_free(sql);
					tcp_data.Header.data_useage = SENDCTRL | RECVROBO | SEND;
					tcp_data.Header.header_count = 1;
					tcp_data.Data_info = (struct DATA_INFO*)malloc(sizeof(struct DATA_INFO)*tcp_data.Header.header_count);
					tcp_data.Data_info[0].data_type = 0;
					tcp_data.Data_info[0].data_count = 1;
					tcp_data.Data = (struct PROTOCOL_DATA*)malloc(sizeof(struct PROTOCOL_DATA));
					tcp_data.Data[0].data_size = sizeof(int);
					tcp_data.Data[0].data = (int *)malloc(tcp_data.Data[0].data_size);
					*(int *)tcp_data.Data[0].data = tmp;
					sem_wait(&sem[robot_id]);
					tcp_write(robot_sock, &tcp_data);
					sem_post(&sem[robot_id]);
				}
			}
			break;
		case CMDN:
			robot_id = *(int*)tcp_data.Data[0].data;
			tmp = 0;
			sql = sqlite3_mprintf("SELECT connected FROM robot WHERE id = %d", robot_id);
			rc = sqlite3_exec(SQLiteDB, sql, retInt, &tmp, &dbErrMsg);
			sqlite3_free(sql);
			if (tmp == controller_id)
			{
				tmp = 0;
				sql = sqlite3_mprintf("SELECT fd FROM robot WHERE id = %d", robot_id);
				rc = sqlite3_exec(SQLiteDB, sql, retInt, &robot_sock, &dbErrMsg);
				sqlite3_free(sql);
				sql = sqlite3_mprintf("SELECT type FROM robot WHERE id = %d", robot_id);
				rc = sqlite3_exec(SQLiteDB, sql, retStr, robot_type, &dbErrMsg);
				sqlite3_free(sql);
				sql = sqlite3_mprintf("SELECT signal FROM motion WHERE id = %d AND type = '%s'", *(int *)tcp_data.Data[1].data, robot_type);
				rc = sqlite3_exec(SQLiteDB, sql, retInt, &tmp, &dbErrMsg);
				sqlite3_free(sql);
				tcp_free(&tcp_data);
				if (tmp)
				{
					tcp_data.Header.data_useage = SENDCTRL | RECVROBO | SEND;
					tcp_data.Header.header_count = 1;
					tcp_data.Data_info = (struct DATA_INFO*)malloc(sizeof(struct DATA_INFO)*tcp_data.Header.header_count);
					tcp_data.Data_info[0].data_type = 0;
					tcp_data.Data_info[0].data_count = 1;
					tcp_data.Data = (struct PROTOCOL_DATA*)malloc(sizeof(struct PROTOCOL_DATA));
					tcp_data.Data[0].data_size = sizeof(int);
					tcp_data.Data[0].data = (int *)malloc(tcp_data.Data[0].data_size);
					*(int *)tcp_data.Data[0].data = tmp;
					sem_wait(&sem[robot_id]);
					tcp_write(robot_sock, &tcp_data);
					sem_post(&sem[robot_id]);
				}
			}
			break;
		case FIN:
			sql = sqlite3_mprintf("UPDATE robot SET connected = 0 WHERE connected = %d AND id = %d ", controller_id, *(int *)tcp_data.Data[0].data);
			rc = sqlite3_exec(SQLiteDB, sql, callback, 0, &dbErrMsg);
			sqlite3_free(sql);
			break;
		}
	}
	return NULL;
}
