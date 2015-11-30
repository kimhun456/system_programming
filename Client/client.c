/*

 ECHO client example using sockets


*/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_THREADS 1000
#define MAX_MESSAGE 2000

char *ip = "52.69.176.156";
int port = 1337;


// thread 가 실행하는 function
void * t_function(void *data)
{
    printf("Thread Start\n");

  printf("%s\n",(char*)data);
  sleep(1);
  printf("Thread end\n");

  return (void*)data;
}

int main(int argc , char *argv[])
{
    int sock;
    int i;

    struct sockaddr_in server;
    char message[MAX_MESSAGE];
    char server_reply[MAX_MESSAGE];
    pthread_t p_thread[MAX_THREADS];
    int thr_id;
    int status;
    int a = 100;
    int count = 0;
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    printf("Socket created\n");


    // PORT AND IP setting
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons( port );



    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    printf("Connected\n");

    //keep communicating with server
    while(count < MAX_THREADS)
    {

        //Receive a data from the server
        if( recv(sock , server_reply , MAX_MESSAGE , 0) < 0)
        {
            puts("recv failed");
            break;
        }

        // printf("Received data : ");
        //
        // printf("%s\n",server_reply);

        printf("Before Thread Created\n");

        thr_id = pthread_create(&p_thread[count], NULL, t_function, (void *)server_reply);

        if (thr_id < 0)
        {
            perror("thread create error : ");
            exit(0);
        }

        // 식별번호 p_thread[count] 를 가지는 쓰레드를 detach
        // 시켜준다.

        pthread_detach(p_thread[count]);

        // 초기화
        count++;
        // for(i=0;i<2000;i++){
        //   server_reply[i]='\0';
        // }
    }

    close(sock);
    return 0;
}


/*
printf("Enter message : ");

fgets(message , 2000, stdin);

message[strlen(message)-1]='\0';


//Send some data
if( send(sock , message , strlen(message) , 0) < 0)
{
    puts("Send failed");
    return 1;
}
for(i=0;i<2000;i++){
  message[i]='\0';
}

*/
