#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handling(char *message);
void printFirstMenu();
void printHelp();
void insert(char *msg);
void update(char *msg);
void delete(char *msg);
void get(char *msg);

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	int str_len, recv_len, recv_cnt;

	if(argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	else
		printf("Connected...........\n");

	while(1)
	{
		char message[BUF_SIZE], cmd[BUF_SIZE], res[BUF_SIZE];
		printFirstMenu();
		fputs("Input message(Q to quit): ", stdout);
		scanf("%s", message);

		cmd[0] = '\0';
		if(!strcmp(message, "q") || !strcmp(message, "Q"))
			break;
		else if(!strcmp(message, "1"))
			insert(cmd);
		else if(!strcmp(message, "2"))
			update(cmd);
		else if(!strcmp(message, "3"))
			delete(cmd);
		else if(!strcmp(message, "4"))
			get(cmd);
		else {
			printf("잘못된 입력입니다. 다시 입력해주세요!\n");
			continue;
		}
		
	//	printf("command: %s\n", cmd);
		if(!strcmp(cmd, "Invalid Message\n"))
			printf("잘못된 입력입니다. 처음부터 다시 입력해주세요!\n");
		else {
			str_len = write(sock, cmd, strlen(cmd));
			read(sock, res, BUF_SIZE);
			printf("%s", res);
		}
	}
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void printFirstMenu()
{
	printf("1.추가  2.수정  3.삭제  4.확인 \n");
	printf("수행할 번호를 입력해주세요.\n");
}

void insert(char *msg)
{
	char input[BUF_SIZE];
	
	printf("1: Camera Table, 2: Institution Table\n");
	printf("Input message: ");
	scanf("%s", input);
	sprintf(msg, "insert&%s", input);

	if(!strcmp(input, "1")) {
		input[0] = '\0';
		printf("Input Camera ID: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);

		input[0] = '\0';
		printf("Input Camera Address: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);

		input[0] = '\0';
		printf("Input Sensor IP Address: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);

		input[0] = '\0';
		printf("Input Area Code: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);
	}
	else if(!strcmp(input, "2")) {
		input[0] = '\0';
		printf("Input Area Code: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);
		input[0] = '\0';
		printf("Input Office Number: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s",msg, input);
	}
	else {
		//에러처리
		strcpy(msg, "Invalid Message\n");
	}
}

void update(char *msg)
{
	char input[BUF_SIZE];
	
	printf("1: Camera Table, 2: Institution Table\n");
	printf("Input message: ");
	scanf("%s", input);
	sprintf(msg, "update&%s", input);

	if(!strcmp(input, "1")) {
		input[0] = '\0';
		printf("Input Camera ID: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);

		input[0] = '\0';
		printf("Input Camera Address: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);

		input[0] = '\0';
		printf("Input Sensor IP Address: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);

		input[0] = '\0';
		printf("Input Area Code: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);
	}
	else if(!strcmp(input, "2")) {
		input[0] = '\0';
		printf("Input Area Code: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);
		input[0] = '\0';
		printf("Input Office Number: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s",msg, input);
	}
	else {
		//에러처리
		strcpy(msg, "Invalid Message\n");
	}
}

void delete(char *msg)
{
	char input[BUF_SIZE];
	
	printf("1: Camera Table, 2: Institution Table\n");
	printf("Input message: ");
	scanf("%s", input);
	sprintf(msg, "delete&%s", input);

	if(!strcmp(input, "1")) {
		input[0] = '\0';
		printf("Input Camera ID: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);
	}
	else if(!strcmp(input, "2")) {
		input[0] = '\0';
		printf("Input Area Code: ");
		scanf("%s", input);
		sprintf(msg, "%s&%s", msg, input);
	}
	else {
		//에러처리
		strcpy(msg, "Invalid Message\n");
	}
}

void get(char *msg)
{
	char input[BUF_SIZE];
	
	printf("1: Camera Table, 2: Institution Table\n");
	printf("Input message: ");
	scanf("%s", input);
	if(!strcmp(input, "1") || !strcmp(input, "2"))
		sprintf(msg, "get&%s", input);
	else
		strcpy(msg, "Invalid Message\n");
}

