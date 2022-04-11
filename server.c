#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

char name[10][20]={0};
char recv_buf[1024];
int arr_sockfd[10];
int i,j,k;
FILE *fp;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER; // add mutex lock

//check the number of arr_sockfd
int check_num(){
    int l;
    int number = 0;
    for(l = 0 ; l < 10 ; l++){
        if(arr_sockfd[l] != 0){
            number ++;
        }
    }
    return number;
}

//set user's name
void set_name(){
    for(j = 0; j < strlen(recv_buf) ; j++){
        if(recv_buf[j] == ']'){  
            strncpy(name[i] , recv_buf , j);
            name[i][j] = '\0';
            for(k = 0 ; k < j ; k++){
                name[i][k] = name[i][k+1];
            }
            break; 
        }
    }
}

//show online user's name
void show_user(){
    char user_list[1024] = "Online Member:\n";
    for(j = 0; j < 10 ; j++){
        if(arr_sockfd[j] != 0){
            strcat(user_list , name[j]);
            strcat(user_list , "   \n");
        }
    }
}


void* broadcast(void* sockfd){
    int accept_sd = *((int *)sockfd);
    int bytes_recv;
    while(1){
        bzero(recv_buf , 1024);
        bytes_recv = recv(accept_sd , recv_buf , sizeof(recv_buf) , 0);//receive message from client
        if(bytes_recv <= 0){//if the client disconnect from the server (receive failed)
            for(i = 0 ; i < 10 ; i++){
	        if(accept_sd == arr_sockfd[i]){//set offline clients' arr_sock to 0
		    arr_sockfd[i] = 0;
                }
	    }    
	    break ;
        }
        
        /*
        if receive msg from a client successfully 
        broadcast the msg to the other clients
        */
        pthread_mutex_lock( &mutex1 ); //lock
        for(i = 0 ; i < 10 ; i++){
            //find client that send msg ,and record it's name
            if(arr_sockfd[i] == accept_sd){
                set_name();
                if(strcmp(recv_buf ,"userlist") == 0){
                    char user_list[1024] = "Online Member:\n";
                    for(j = 0; j < 10 ; j++){
                        if(arr_sockfd[j] != 0){
                            strcat(user_list , name[j]);
                            strcat(user_list , "  \n");
                        }
                    }
                    send(arr_sockfd[i] ,user_list , strlen(user_list) , 0 );
                }
            }
            //find all the user that's connecting,except who send msg
            if(arr_sockfd[i] != 0 &&arr_sockfd[i] != accept_sd && strcmp(recv_buf,"userlist") != 0){
                send(arr_sockfd[i] , recv_buf , bytes_recv , 0);
	        }
        }
        pthread_mutex_unlock( &mutex1 ); //unlock
        if(strcmp(recv_buf ,"userlist") == 0)
            continue;
        printf("%s\n",recv_buf);
        //the sending msg synchronous record to chatlog
        fprintf(fp,"%s\n",recv_buf);
        fflush(fp);
        bzero(recv_buf , 1024);
    }
    close(accept_sd);

}
int main(int argc, char const *argv[]){
    
    fp = fopen("chatlog.txt","w");
    int client_number = 0;
    //building TCP socket
    int socket_listen = socket(AF_INET , SOCK_STREAM , 0);
    //int acc_sd = socket(AF_INET , SOCK_STREAM , 0);
    struct sockaddr_in serv_info,client_info;

    //make sure that socket has been built successfully
    if(socket_listen < 0){
        printf("socket failed\n");
        return -1;
    }

    bzero(&serv_info , sizeof(serv_info));
    serv_info.sin_family = AF_INET;
    serv_info.sin_port = htons(22222);
    serv_info.sin_addr.s_addr = INADDR_ANY ;//don't care about local ip , let the kernel determine
    //use the address above to bind and then listen

    //bind local internet 
    int check_bind = bind(socket_listen , (struct sockaddr *)&serv_info , sizeof(serv_info));
    if(check_bind == -1){
        printf("bind failed\n");
        return -1;
    }
    //if bind successfully , we can continue on listening
    
    
    int check_listen = listen (socket_listen , 10);
    if(check_listen == -1){
        printf("listen failed\n");
        return -1;
    }
    //if listen successfully , we can continue on accept()
    printf("Chat Room \n");
    printf("Waiting for user to connect......\n");
    

    while(1) {
	    int ci_length = sizeof(client_info);
        int acc_sd = accept(socket_listen , (struct sockaddr*)&client_info , &ci_length);
        if(acc_sd == -1) {
    	    printf("Accept failed\n");
    	    return -1;
    	} else {
            client_number = check_num();
            if(client_number >= 10) { //reaching maximum 
                printf("Chat Room Full\n");
		        close(acc_sd); //make the client who wants to join DISAPPEAR
	        } 
        }
        

    	for(i = 0 ; i < 10 ; i++) {
            if(arr_sockfd[i] == 0 ) {
                arr_sockfd[i] = acc_sd;
                break; 
    	    }
        }

	    pthread_t recv_handler;
        pthread_create(&recv_handler , NULL , *broadcast , &acc_sd); //use another thread to deal with send/recieve msg sychronize
    }

    fclose(fp);
    close(socket_listen);
    return 0;  
}
