#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

bool hasExit = false;

void* pthread_recvmsg(void *sock_connect){
    char recv_msg[1024];
    int sockfd2 = *((int *)sock_connect);
    while(1){
        if(recv(sockfd2 , recv_msg , sizeof(recv_msg) , 0 ) == -1){
            if(!hasExit) printf("receive failed\n");
            break;
        }
        printf("%s\n",recv_msg);
        memset(recv_msg , 0 , sizeof(recv_msg));
    }
}

int main(int argc, char const *argv[]){
    char send_msg[1024];
    char onli_msg[50];
    char ofli_msg[50];
    char name[20];
    int sockfd = socket(AF_INET , SOCK_STREAM , 0);
    struct sockaddr_in server_info;
    
    //make sure that socket is created successfully
    if(sockfd < 0){
        printf("socket failed\n");
        return -1;
    }
    
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(22222);
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");//convert ip address to binary data

    //connecting to the server
    if(connect(sockfd , (struct sockaddr *)&server_info , sizeof(server_info)) == -1){
        printf("connection failed");
        return -1;
    }
    
    
    char left[]  ="[\0";
    char right[] ="]\0";
    char online[] = " is online now!!";
    char offline[] = " is offline now!!";
    
    memset(onli_msg , 0 , sizeof(onli_msg));
    memset(ofli_msg , 0 , sizeof(ofli_msg));

    printf("Please enter your name : ");
    fgets(name , sizeof(name) , stdin );
    name[strlen(name)-1] = '\0';
    printf("Connect Success!\n");
    //send message to inform the server that a client has logged in
    strcat(onli_msg , left);
    strcat(onli_msg , name);
    strcat(onli_msg , right);
    strcat(onli_msg , online);
    printf("%s\n",onli_msg);
    send(sockfd , onli_msg , strlen(onli_msg) , 0);
        
    
    //taking care of receiving messages from server
    pthread_t pre_recv;
    pthread_create(&pre_recv , NULL , pthread_recvmsg , &sockfd);
    

    char msg_buf[512];    
    int check_send;
    //taking care of sending messages from the client to the server
    while(1){
        memset(send_msg , 0 , sizeof(send_msg));//initializing the message sent to server
	    memset(msg_buf , 0 ,sizeof(msg_buf));//initializing the message that client enter 
        fgets(msg_buf , sizeof(msg_buf) , stdin );
        
        msg_buf[strlen(msg_buf)-1] = '\0';
        if(strcmp(msg_buf , "exit") == 0) {//check if client want to leave
            hasExit = true;
            strcat(ofli_msg , left);
            strcat(ofli_msg , name);
            strcat(ofli_msg , right);
            strcat(ofli_msg , offline);
            send(sockfd , ofli_msg , strlen(ofli_msg) , 0);
            break;
	    } else if(strcmp(msg_buf , "userlist") == 0) {
            send(sockfd , msg_buf , strlen(msg_buf) , 0);
            continue;
        }

        /*
    	combine client's message and user name, 
    	then send them to the server 
    	*/
        strcat(send_msg , name);
        strcat(send_msg , " : ");
        strcat(send_msg , msg_buf);
        check_send = send(sockfd , send_msg ,strlen(send_msg) , 0 );
        if(check_send <= 0){
            printf("sending failed\n");
            break;
        }
    }
    close(sockfd);
    return 0;  
}
