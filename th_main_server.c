#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include "/usr/include/mysql/mysql.h"
#include <pthread.h>
#include <cv.h>
#include <highgui.h>

IplImage* img;
int is_data_ready = 0;
char key = '0';

enum {FALSE, TRUE};
enum {Fallen_people, animal_carcass, Obstacle};
enum {Main_server, Database, Camera, LED_alarm, Institution};
#define BUF_SIZE 100
#define IMG_SIZE 1000000
#define EPOLL_SIZE 50
void *main_server(void *arg);
void *database_server(void *arg);
void *showImage(void *arg);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char recv_img[IMG_SIZE];

void setnonblockingmode(int fd);
void error_handling(char *message);
char *getClientHeader(char *buf, int *id, char *host, int *content_size);
void CameraClientProcess(char *content, int *camera_id, int *class_number, char *tmp_image);
int DatabaseProcess(int camera_id, char *camera_address, char *sensor_ip, char *institution_number);
void DatabaseManage(char *cmd, char *res);
void AlarmClientAction(char *serv_ip, int class_num);

int main(int argc, char *argv[])
{
	int width=640, height=480;
	img = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
	cvZero(img);

	pthread_t main_st, db_st, thread_show;

	if(pthread_create(&main_st, NULL, main_server, NULL))
		error_handling("Main server pthread create failed!");
	if(pthread_create(&db_st, NULL, database_server, NULL))
		error_handling("Database server pthread create failed!");
	if(pthread_create(&thread_show, NULL, showImage, NULL))
		error_handling("Show image pthread create failed!");

	pthread_join(thread_show, NULL);
	pthread_join(main_st, NULL);
	pthread_join(db_st, NULL);

	pthread_mutex_destroy(&mutex);

	return 0;
}

void *main_server(void *arg)
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t adr_sz, optlen;
	int str_len, option, i;
	char buf[BUF_SIZE];
	char port[10] = "8000";

	struct epoll_event *ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("Main Server socket() error");

	//For Binding Error
	optlen = sizeof(option);
	option = TRUE;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(port));

	if(bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("Main Server bind() error");
	if(listen(serv_sock, 5) == -1)
		error_handling("Main Server listen() error");
	else
		printf("메인 서버가 정삭적으로 연결되었습니다...\n");

	epfd = epoll_create(EPOLL_SIZE);
	ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

	event.events = EPOLLIN;
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

	while(1) {
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if(event_cnt == -1) {
			puts("epoll_wait() error");
			break;
		}
		puts("return epoll wait");

		for(i = 0; i < event_cnt; i++) {
			if(ep_events[i].data.fd == serv_sock) {
				adr_sz = sizeof(clnt_adr);
				clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
				setnonblockingmode(clnt_sock);
				event.events = EPOLLIN | EPOLLET;
				event.data.fd = clnt_sock;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				printf("connected client : %d\n ", clnt_sock);
			}
			else {
				while(1) {
					str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
					if(str_len == 0) {
						epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
						close(ep_events[i].data.fd);
						printf("closed client : %d\n", ep_events[i].data.fd);
					}
					else if(str_len < 0) {
						if(errno == EAGAIN)
							break;
					}
					else {
						int id, content_size, camera_id, class_number;
						char host[30];
						char content[BUF_SIZE], tmp_image[BUF_SIZE];

						//메시지 분리하기
						strcpy(content, getClientHeader(buf, &id, host, &content_size));
						content[content_size - 1] = '\0';
						CameraClientProcess(content, &camera_id, &class_number, tmp_image);
						printf("ID : %d, HOST : %s, CONTENT_SIZE : %d, Camera_ID: %d, Class_Number: %d\n",
								id, host, content_size, camera_id, class_number);
						char response[100];
						sprintf(response, "200OK");
						write(ep_events[i].data.fd, response, sizeof(response));
						//Img read
						int imgsize = img->imageSize;
						int bytes, j;
						memset(recv_img, 0, IMG_SIZE);
						for(j = 0; j < imgsize; j += bytes) {
						 	bytes = read(ep_events[i].data.fd, recv_img + j, imgsize);
						 	//printf("check: %d, %d\n", bytes, j);
						 	if(bytes == -1) {
						 		bytes = 0;
						 	}
						}
						strcpy(img->imageData, recv_img);
						sprintf(response, "200OK");
						write(ep_events[i].data.fd, response, sizeof(response));

						//데이터베이스 통신
						char camera_address[80], sensor_ip[30], institution_number[10];
						if(!DatabaseProcess(camera_id, camera_address, sensor_ip, institution_number))
							error_handling("Database process error");

						//Send to alarm server
						AlarmClientAction(sensor_ip, class_number);

						//Send to institution
						if(class_number == 1) {
							printf("Institution number: %s, Detected image: Fallen person, Camera address: %s\n",
								institution_number, camera_address);
							pthread_mutex_lock(&mutex);
							is_data_ready = 1;
							pthread_mutex_unlock(&mutex);
						}
						else if(class_number == 2) {
							printf("Institution number: %s, Detected image: Load kill, Camera address: %s\n",
								institution_number, camera_address);
							pthread_mutex_lock(&mutex);
							is_data_ready = 1;
							pthread_mutex_unlock(&mutex);
						}
						usleep(100000);
					}
				}
			}
		}
	}

	close(serv_sock);
	close(epfd);
}

void *showImage(void *arg)
{
	cvNamedWindow("stream_server", CV_WINDOW_AUTOSIZE);

	while(key != 'q') {
		pthread_mutex_lock(&mutex);
		if(is_data_ready) {
			cvShowImage("stream_server", img);
         		is_data_ready = 0;
		}
		pthread_mutex_unlock(&mutex);

		key = cvWaitKey(10);
	}
	cvDestroyWindow("stream_server");

	if (img) cvReleaseImage(&img);
	return;
}

void *database_server(void *arg)
{
	//기본 서버로 해서 데이터베이스 관리만을 위한 서버, 단일 서버로 구현
	int db_serv_sock, db_clnt_sock;
	int str_len, db_option;
	struct sockaddr_in db_serv_adr, db_clnt_adr;
	socklen_t db_clnt_adr_sz, db_optlen;
	char port[10] = "9191";
	char cmd[100];
	char response[1000];

	db_serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(db_serv_sock == -1)
		error_handling("DB Server socket() error");

	//For Binding Error
	db_optlen = sizeof(db_option);
	db_option = TRUE;
	setsockopt(db_serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&db_option, db_optlen);

	memset(&db_serv_adr, 0, sizeof(db_serv_adr));
	db_serv_adr.sin_family = AF_INET;
	db_serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	db_serv_adr.sin_port = htons(atoi(port));

	if(bind(db_serv_sock, (struct sockaddr *)&db_serv_adr, sizeof(db_serv_adr)) == -1)
		error_handling("DB Server bind() error");
	if(listen(db_serv_sock, 5) == -1)
		error_handling("DB Server listen() error");
	else
		printf("데이터베이스 관리 서버가 정상적으로 연결되었습니다...\n");

	while(1) {
		db_clnt_adr_sz = sizeof(db_clnt_adr);
		db_clnt_sock = accept(db_serv_sock, (struct sockaddr *)&db_clnt_adr, &db_clnt_adr_sz);
		if(db_clnt_sock == -1)
			error_handling("DB Server accept() error");
		else
			printf("Connected DB management client\n");

		while((str_len = read(db_clnt_sock, cmd, 100)) != 0) {
			response[0] = '\0';
			DatabaseManage(cmd, response);
			write(db_clnt_sock, response, sizeof(response));
		}

		close(db_clnt_sock);
	}
	close(db_serv_sock);
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

char *getClientHeader(char *buf, int *id, char *host, int *content_size)
{
	char *token[4] = {NULL, };

	int i = 0;
	char *ptr = strtok(buf, "=");
	while(ptr != NULL) {
		token[i++] = ptr;
		if(i == 4) break;
		ptr = strtok(NULL, "=");
	}
	*id = atoi(token[0]);
	strcpy(host, token[1]);
	*content_size = atoi(token[2]);

	return token[3];
}

void CameraClientProcess(char *content, int *camera_id, int *class_number, char *tmp_image)
{
	char *token[3] = {NULL, };

	int i = 0;
	char *ptr = strtok(content, "-");
	while(ptr != NULL) {
		token[i++] = ptr;
		if(i == 3) break;
		ptr = strtok(NULL, "-");
	}
	*camera_id = atoi(token[0]);
	*class_number = atoi(token[1]);
	strcpy(tmp_image, token[2]);
}

int DatabaseProcess(int camera_id, char *camera_address, char *sensor_ip, char *institution_number)
{
	MYSQL conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char user[12] = "root";
	char pwd[20] = "root";

	mysql_init(&conn);
	if(!(mysql_real_connect(&conn, NULL, user, pwd, NULL, 3306, (char *)NULL, 0))) {
		printf("MYSQL error code : %s\n", mysql_error(&conn));
		mysql_close(&conn);
		error_handling("MYSQL connection error");
	}
	if(mysql_query(&conn, "USE crosswalk_yolo")) {
		printf("MYSQL error code : %s\n", mysql_error(&conn));
		mysql_close(&conn);
		error_handling("MYSQL query error");
	}
	if(mysql_query(&conn, "SELECT * FROM camera")) {
		printf("MYSQL error code : %s\n", mysql_error(&conn));
		mysql_close(&conn);
		error_handling("MYSQL query error");
	}

	int numOfField;
	char area_code[10];
	res = mysql_store_result(&conn);
	numOfField = mysql_num_fields(res);
	if(numOfField <= camera_id) {
		printf("inacceptable CAMERA ID\n");
		return 0;
	}

	while((row = mysql_fetch_row(res))) {
		if(camera_id == atoi(row[0])) {
			strcpy(camera_address, row[1]);
			strcpy(sensor_ip, row[2]);
			strcpy(area_code, row[3]);
			break;
		}
	}
	printf("%s\n", camera_address);
	if(mysql_query(&conn, "SELECT * FROM institution")) {
		printf("MYSQL error code : %s\n", mysql_error(&conn));
		mysql_close(&conn);
		error_handling("MYSQL query error");
	}
	res = mysql_store_result(&conn);
	while((row = mysql_fetch_row(res))) {
		if(!strcmp(area_code, row[0])) {
			strcpy(institution_number, row[1]);
			break;
		}
	}
	mysql_free_result(res);
	mysql_close(&conn);
	return 1;
}

void AlarmClientAction(char *serv_ip, int class_num)
{
	printf("Alram Server IP: %s\n", serv_ip);
	int alarm_sock;
	struct sockaddr_in alarm_serv_addr;

	alarm_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(alarm_sock == -1)
		error_handling("Alarm socket() error");

	memset(&alarm_serv_addr, 0, sizeof(alarm_serv_addr));
	alarm_serv_addr.sin_family = AF_INET;
	alarm_serv_addr.sin_addr.s_addr = inet_addr(serv_ip);
	alarm_serv_addr.sin_port = htons(8000);
	if(connect(alarm_sock, (struct sockaddr *)&alarm_serv_addr, sizeof(alarm_serv_addr)) == -1)
		error_handling("Alarm connect() error");
	char alarm_msg[BUF_SIZE];
	char check_msg[30];
	sprintf(alarm_msg, "1|127.0.0.1|%d|%d", sizeof(class_num), class_num);
	while(1) {
		write(alarm_sock, alarm_msg, strlen(alarm_msg));
		read(alarm_sock, check_msg, 30);
		if(strstr(check_msg, "AlarmServer-200OK")) {
			printf("%s\n", check_msg);
			break;
		}
	}
	close(alarm_sock);
}

void DatabaseManage(char *message, char *response)
{
	char *token[6];
	int table_number, camera_id;
	char cmd[10], camera_address[80], sensor_ip[30], area_code[5], office_number[10];

	int i = 0;
	char *ptr = strtok(message, "&");
	token[i++] = ptr;
	strcpy(cmd, token[0]);
	while(ptr != NULL) {
		ptr = strtok(NULL, "&");
		token[i++] = ptr;
	}
	table_number = atoi(token[1]);
	if(!strcmp(cmd, "insert") || !strcmp(cmd, "update")) {
		if(table_number == 1) {  //camera table
			camera_id = atoi(token[2]);
			strcpy(camera_address, token[3]);
			strcpy(sensor_ip, token[4]);
			strcpy(area_code, token[5]);
		}
		else if(table_number == 2) {  //institution table
			strcpy(area_code, token[2]);
			strcpy(office_number, token[3]);
		}
	}
	else if(!strcmp(cmd, "delete")) {
		if(table_number == 1) {
			camera_id = atoi(token[2]);
		}
		else if(table_number == 2) {
			strcpy(area_code, token[2]);
		}
	}

	MYSQL conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char query[255];
	char user[12] = "root";
	char pwd[20] = "root";

	mysql_init(&conn);
	if(!(mysql_real_connect(&conn, NULL, user, pwd, NULL, 3306, (char *)NULL, 0))) {
		printf("MYSQL error code : %s\n", mysql_error(&conn));
		mysql_close(&conn);
		error_handling("MYSQL connection error");
	}
	
	if(mysql_query(&conn, "USE crosswalk_yolo")) {
		printf("MYSQL error code : %s\n", mysql_error(&conn));
		mysql_close(&conn);
		error_handling("MYSQL query error");
	}
	if(!strcmp(cmd, "insert")) {
		if(table_number == 1) {  //camera table
			sprintf(query, "INSERT INTO camera values(%d, '%s', '%s', '%s');",
					camera_id, camera_address, sensor_ip, area_code);
			if(mysql_query(&conn, query)) {
				printf("MYSQL error code : %s\n", mysql_error(&conn));
				mysql_close(&conn);
				error_handling("MYSQL query error");
			}
			sprintf(response, "Success insert(Camera table, %d, %s, %s, %s)\n200OK\n", 
					camera_id, camera_address, sensor_ip, area_code);
		}
		else if(table_number == 2) {  //institution table
			sprintf(query, "INSERT INTO institution values('%s', '%s');",
					area_code, office_number);
			if(mysql_query(&conn, query)) {
				printf("MYSQL error code : %s\n", mysql_error(&conn));
				mysql_close(&conn);
				error_handling("MYSQL query error");
			}
			sprintf(response, "Success insert(Institution table, %s, %s)\n200OK\n", 
					area_code, office_number);
		}
	}
	else if(!strcmp(cmd, "update")) {
		if(table_number == 1) {
			if(!strcmp(camera_address, "pass") || !strcmp(camera_address, "PASS")) {
				if(!strcmp(sensor_ip, "pass") || !strcmp(sensor_ip, "PASS")) {
					sprintf(query, "UPDATE camera SET area_code='%s' WHERE id=%d;", area_code, camera_id);
					if(mysql_query(&conn, query)) {
						printf("MYSQL error code : %s\n", mysql_error(&conn));
						mysql_close(&conn);
						error_handling("MYSQL query error");
					}
					sprintf(response, "Success update(Camera table)\n200OK\n");
				}
				else if(!strcmp(area_code, "pass") || !strcmp(area_code, "PASS")) {
					sprintf(query, "UPDATE camera SET sensor_IP_address='%s' WHERE id=%d;",
							sensor_ip, camera_id);
					if(mysql_query(&conn, query)) {
						printf("MYSQL error code : %s\n", mysql_error(&conn));
						mysql_close(&conn);
						error_handling("MYSQL query error");
					}
					sprintf(response, "Success update(Camera table)\n200OK\n");
				}
				else {
					sprintf(query, "UPDATE camera SET sensor_IP_address='%s', area_code='%s' WHERE id=%d;",
							sensor_ip, area_code, camera_id);
					if(mysql_query(&conn, query)) {
						printf("MYSQL error code : %s\n", mysql_error(&conn));
						mysql_close(&conn);
						error_handling("MYSQL query error");
					}
					sprintf(response, "Success update(Camera table)\n200OK\n");
				}
			}
			else if(!strcmp(sensor_ip, "pass") || !strcmp(sensor_ip, "PASS")) {
				if(!strcmp(area_code, "pass") || !strcmp(area_code, "PASS")) {
					sprintf(query, "UPDATE camera SET camera_address='%s' WHERE id=%d;",
							camera_address, camera_id);
					if(mysql_query(&conn, query)) {
						printf("MYSQL error code : %s\n", mysql_error(&conn));
						mysql_close(&conn);
						error_handling("MYSQL query error");
					}
					sprintf(response, "Success update(Camera table)\n200OK\n");
				}
				else {
					sprintf(query, "UPDATE camera SET camera_address='%s', area_code='%s' WHERE id=%d;",
							camera_address, area_code, camera_id);
					if(mysql_query(&conn, query)) {
						printf("MYSQL error code : %s\n", mysql_error(&conn));
						mysql_close(&conn);
						error_handling("MYSQL query error");
					}
					sprintf(response, "Success update(Camera table)\n200OK\n");
				}
			}
			else if(!strcmp(area_code, "pass") || !strcmp(area_code, "PASS")) {
				sprintf(query, "UPDATE camera SET camera_address='%s', sensor_IP_address='%s' WHERE id=%d;",
						camera_address, sensor_ip, camera_id);
				if(mysql_query(&conn, query)) {
					printf("MYSQL error code : %s\n", mysql_error(&conn));
					mysql_close(&conn);
					error_handling("MYSQL query error");
				}
				sprintf(response, "Success update(Camera table)\n200OK\n");
			}
			else {
				sprintf(query, "UPDATE camera SET camera_address='%s',sensor_IP_address='%s' area_code='%s'WHERE id=%d;", camera_address, sensor_ip, area_code, camera_id);
				if(mysql_query(&conn, query)) {
					printf("MYSQL error code : %s\n", mysql_error(&conn));
					mysql_close(&conn);
					error_handling("MYSQL query error");
				}
				sprintf(response, "Success update(Camera table)\n200OK\n");
			}
		}
		else if(table_number == 2) {
			sprintf(query, "UPDATE institution SET office_number='%s' WHERE area_code='%s';",
					office_number, area_code);
			if(mysql_query(&conn, query)) {
				printf("MYSQL error code : %s\n", mysql_error(&conn));
				mysql_close(&conn);
				error_handling("MYSQL query error");
			}
			sprintf(response, "Success update(Institution table)\n200OK\n");
		}
	}
	else if(!strcmp(cmd, "delete")) {
		if(table_number == 1) {
			sprintf(query, "DELETE FROM camera WHERE id=%d;", camera_id);
			if(mysql_query(&conn, query)) {
				printf("MYSQL error code : %s\n", mysql_error(&conn));
				mysql_close(&conn);
				error_handling("MYSQL query error");
			}
			sprintf(response, "Success delete(Camera table, %d)\n200OK\n", camera_id);
		}
		else if(table_number == 2) {
			sprintf(query, "DELETE FROM institution WHERE area_code='%s';", area_code);
			if(mysql_query(&conn, query)) {
				printf("MYSQL error code : %s\n", mysql_error(&conn));
				mysql_close(&conn);
				error_handling("MYSQL query error");
			}
			sprintf(response, "Success delete(Institution table, %s)\n200OK\n", area_code);
		}
	}
	else if(!strcmp(cmd, "get")) {
		if(table_number == 1) {
			sprintf(query, "SELECT *from camera ORDER BY id ASC;");
			if(mysql_query(&conn, query)) {
				printf("MYSQL error code : %s\n", mysql_error(&conn));
				mysql_close(&conn);
				error_handling("MYSQL query error");
			}

			int numOfField;
			res = mysql_store_result(&conn);
			numOfField = mysql_num_fields(res);
			if(numOfField == 0) {
				sprintf(response, "Empty Camera Table\n");
				return;
			}
			sprintf(response, "%-5s%-40s%-30s%-20s\n", "id", "Camera Address", "Sensor IP Address",
					"Area Code");
			while((row = mysql_fetch_row(res))) {
				sprintf(response, "%s%-5s%-40s%-30s%-20s\n", response, row[0], row[1], row[2], row[3]);
			}
			sprintf(response, "%s200OK\n", response);
		}
		else if(table_number == 2) {
			sprintf(query, "SELECT *from institution;");
			if(mysql_query(&conn, query)) {
				printf("MYSQL error code : %s\n", mysql_error(&conn));
				mysql_close(&conn);
				error_handling("MYSQL query error");
			}
			int numOfField;
			res = mysql_store_result(&conn);
			numOfField = mysql_num_fields(res);
			if(numOfField == 0) {
				sprintf(response, "Empty Institution Table\n");
				return;
			}
			sprintf(response, "%-20s%-30s\n", "Area Code", "Office Number");
			while((row = mysql_fetch_row(res))) {
				sprintf(response, "%s%-20s%-30s\n", response, row[0], row[1]);
			}
			sprintf(response, "%s200OK\n", response);
		}
		mysql_free_result(res);
	}
	mysql_close(&conn);
}