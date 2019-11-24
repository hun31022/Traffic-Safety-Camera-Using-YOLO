#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <wiringPi.h>

//enum {FALSE, TRUE};
#define BUF_SIZE 1024

#define LED1 4  //BCM_GPIO 23
#define LED2 29  //BCM_GPIO 24

void error_handling(char *message);
void setnonblockingmode(int fd);
void AlarmMsgParser(char *message, int *id, char *host, int *content_size, int *class_number);

int main(int argc, char *argv[])
{
	if(wiringPiSetup() == -1) {
		error_handling("wiringPi error");
		return 1;
	}

	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);

	int pi_serv_sock, pi_clnt_sock;
	struct sockaddr_in pi_serv_adr, pi_clnt_adr;
	socklen_t pi_clnt_adr_sz, pi_optlen;
	int str_len, pi_option;
	char port[10] = "8000";
	char msg[BUF_SIZE]; 
	
	pi_serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(pi_serv_sock == -1)
		error_handling("PI Server socket() error");

	pi_optlen = sizeof(pi_option);
	pi_option = TRUE;
	setsockopt(pi_serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&pi_option, pi_optlen);

	memset(&pi_serv_adr, 0, sizeof(pi_serv_adr));
	pi_serv_adr.sin_family = AF_INET;
	pi_serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	pi_serv_adr.sin_port = htons(atoi(port));

	if(bind(pi_serv_sock, (struct sockaddr *)&pi_serv_adr, sizeof(pi_serv_adr)) == -1)
		error_handling("PI Server bind() error");
	if(listen(pi_serv_sock, 5) == -1)
		error_handling("PI Server listen() error");
	else
		printf("start alarm server...\n");

	char res[30] = "AlarmServer-200OK";

	while(1) {
		pi_clnt_adr_sz = sizeof(pi_clnt_adr);
		pi_clnt_sock = accept(pi_serv_sock, (struct sockaddr *)&pi_clnt_adr, &pi_clnt_adr_sz);
		if(pi_clnt_sock == -1)
			error_handling("PI Server accept() error");
		else
			printf("Connected Alarm client...\n");

		while((str_len = read(pi_clnt_sock, msg, BUF_SIZE)) != 0) {
			int clnt_id, content_size, class_number;
			char clnt_host[30];
			AlarmMsgParser(msg, &clnt_id, clnt_host, &content_size, &class_number);

			if(class_number == 1) {
				printf("Class: %d\n", class_number);
				printf("detected Fallen person, turn on the red light\n");
				write(pi_clnt_sock, res, strlen(res)); 
				digitalWrite(LED1, 1);
				delay(5000);
				digitalWrite(LED1, 0);
				
			}
			else if(class_number == 2) {
				printf("Class: %d\n", class_number);
				printf("detected Road kill, turn on the white light\n");
				write(pi_clnt_sock, res, strlen(res)); 
				digitalWrite(LED2, 1);
				delay(5000);
				digitalWrite(LED2, 0);
				
			}
			else 
				printf("Message error\n");
		}
	}
	return 0;
}

void setnonblockingmode(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void AlarmMsgParser(char *message, int *id, char *host, int *content_size, int *class_number)
{
	char *token[4] = {NULL, };

	int i = 0;
	char *ptr = strtok(message, "|");
	while(ptr != NULL) {
		token[i++] = ptr;
		if(i == 4) break;
		ptr = strtok(NULL, "|");
	}
	*id = atoi(token[0]);
	strcpy(host, token[1]);
	*content_size = atoi(token[2]);
	*class_number = atoi(token[3]);
}
