#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include <semaphore.h>
#include <sys/wait.h>

#define MAX_CONENCT 3

sem_t connection;
int cnt = 0;

void error_handling(char *message);
void* get_message(void* args);
void find();
int main(int argc, char *argv[]){
	int serv_sock;
	int clnt_sock;
    int str_len;
	int max_test = 0;
	int next_thread = 0;
	pthread_t pthread_list[MAX_CONENCT];
    if(sem_init(&connection, 0, 1) < 0){                     // 두번째: 프로세스 공유 여부 세 번째 인자: 공유 자원 동시 접근수
        return 0;
    };
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;
	serv_sock=socket(PF_INET, SOCK_STREAM, 0); // 소켓 생성(이후 bind와 accept를 호출하기에 서버소켓이 된다.)
	if(serv_sock==-1){
		error_handling("socket() error");
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi("1234"));

	if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1){ // IP주소, PORT번호 할당
		error_handling("bind() error");
		return 0;
	}

	if(listen(serv_sock, 20)==-1){ // 소켓 연결요청 받아들일 수 있는 상태가 됨 
		error_handling("listen() error");
		return 0;
	}

	printf("iam listening!\n", clnt_sock);
	while(max_test < 6) {
	    clnt_addr_size = sizeof(clnt_addr);
	    clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size); // 연결요청이 있을 때 까지 함수는 반환되지 않음
        if(clnt_sock==-1)
	    	error_handling("accept() error");
        if(cnt >= MAX_CONENCT){
            printf("MAX CONNECTION %d\n", clnt_sock);
            close(clnt_sock);
            continue;
		}else{
			sem_wait(&connection);
			printf("make thread\n");
        	cnt++;
			next_thread = find();
        	pthread_create(&pthread_list[cnt], NULL, get_message, (void*)&clnt_sock);
        	sem_post(&connection);
			max_test++;
		}
    }
	close(serv_sock);
    sem_close(&connection);
	return 0;
}

void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
}
void* get_message(void* args){
    int clnt_sock = *((int*) args);  // void*를 int로 변환
    int str_len;
    int temp;
    printf("socket id: %d thread id: %lu used connection %d\n", clnt_sock, pthread_self(), cnt);
    char message[30];

	while(strcmp(message, "exit") != 0){
	    str_len=read(clnt_sock, message, sizeof(message)-1);
	    if(str_len==-1) {error_handling("read() error"); return NULL;}
		str_len = send(clnt_sock, message, sizeof(message)-1, MSG_DONTWAIT);
        if(str_len == -1) error_handling("send error");
        printf("socket id %d:", clnt_sock);
        printf("%s\n", message);
    }
    sem_wait(&connection);
	close(clnt_sock);
    cnt --;
    sem_post(&connection);
    printf("socket id %d closed remain conenction%d\n", clnt_sock, cnt);
	return NULL;
}
void find();