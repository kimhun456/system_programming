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

/*
thread and message variable
*/
#define MAX_THREADS 10000
#define MAX_MESSAGE 2000


// ip , port
char *ip = "52.69.176.156";
int port = 1337;
int sock;
struct sockaddr_in server;

pthread_t p_thread[MAX_THREADS];
int p_thread_id[MAX_THREADS];
int count = 0;

// thread 가 실행하는 function
void * t_function(void *data)
{
    printf("Thread Start\n");
    char str[MAX_MESSAGE] ;
    int count = 0;
    strcpy(str,(char *)data);
    char *ptr;
    char device[MAX_MESSAGE];
    char type[MAX_MESSAGE];

    // maximum 10
    char params[10][MAX_MESSAGE];
    int i;

    printf("input string is  : %s\n" , str) ;

    ptr = strtok(str, "/");
    strcpy(device,ptr);
    ptr = strtok(NULL, "/");
    strcpy(type,ptr);

    while(ptr != NULL ){

        ptr = strtok(NULL, "/");

        if(ptr!=NULL){
            strcpy(params[count],ptr);
        }
        count++;
    }
    count --;

    printf("device : %s\n",device);
    printf("type : %s\n",type);



    


    for(i=0;i<count;i++){
        printf("params %d : %s\n",i+1,params[i]);
    }

    //sleep(1);

    printf("Thread end\n");

    // initialized

    for(int i=0;i<MAX_MESSAGE;i++){
        str[i] = '\0';
    }

    return (void*)data;
}

int main(int argc , char *argv[])
{

    char server_reply[MAX_MESSAGE];
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

        p_thread_id[count] = pthread_create(&p_thread[count], NULL, t_function, (void *)server_reply);

        if (p_thread_id[count] < 0)
        {
            perror("thread create error : ");
            exit(0);
        }

        // 식별번호 p_thread[count] 를 가지는 쓰레드를 detach
        // 시켜준다.

        pthread_detach(p_thread[count]);

        // 초기화
        count++;


        if(count > MAX_THREADS){
            count=count%MAX_THREADS;
        }
        // for(i=0;i<2000;i++){
        //   server_reply[i]='\0';
        // }
    }

    close(sock);
    return 0;
}
