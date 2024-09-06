#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>

void error_handling(char *message);
void* receive_msg(void* args);
int main(int argc, char* argv[]){
	int sock;
	struct sockaddr_in serv_addr;
	char message[30];
    size_t bytes;
	pthread_t pthread;

    /* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  */
	sock=socket(PF_INET, SOCK_STREAM, 0); 
	if(sock==-1) error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr("129.0.0.1");
	serv_addr.sin_port=htons(atoi("1234"));
	
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) // 클라이언트 소켓
		error_handling("connect() error");
	pthread_create(&pthread, NULL, receive_msg, (void*)&sock);
	while(strcmp(message, "exit") != 0){
        int i = 0;
		scanf("%s", message);
	    bytes = send(sock, message, sizeof(message)-1, MSG_DONTWAIT);
        if(bytes == -1) error_handling("send error");
    }
	pthread_cancel(pthread);
	close(sock);
	return 0;
}

void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n',stderr);
	exit(1);
}

void* receive_msg(void* args){
    int sock = *((int*) args);  // void*를 int로 변환
    int str_len;
    char message[30];
	
	while(1){
		str_len=read(sock, message, sizeof(message)-1);
		if(str_len==-1) error_handling("read() error");
		printf("Message from server : %s \n",message);
    }
	return NULL;
}


