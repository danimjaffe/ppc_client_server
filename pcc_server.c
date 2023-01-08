#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

//globals for signal handler access
int* pcc_total;
int flag = 0; // 1 if getting SIGINT signal during TCP connection handling
int connfd = -1; // -1 if no current connected host
/*SIGINT handler as described*/
void sighandler() {
    if (connfd == -1){
        for(int i = 0; i < 95; i++){
            printf("char '%c' : %u times\n", (char)(i+32), pcc_total[i]);
        }
        exit(0);
    }
    flag=1;    
}
/*Checks client connection in case
returns 0 if bytes > 0 meaning no tcp connection error
return -1 if an error that doesn't require exit
otherwise exists
*/
int check_client_connection(int nbytes){
    if (nbytes<=0){
        if (nbytes==0){
            fprintf(stderr,"Error in TCP connection: %s", strerror(errno));
            connfd=-1;
            return -1;
        }
        else{
            if (errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE){
                fprintf(stderr,"Error in TCP connection: %s", strerror(errno));
                connfd=-1;
                return -1;
            }
            else{
                fprintf(stderr,"Error in TCP connection: %s", strerror(errno));
                exit(EXIT_FAILURE);
            }
            
        }
    }
    return 0;
}
/* argv[1]: server’s port 
*/
int main(int argc, char *argv[]){
    struct sockaddr_in serv_addr;
    int listenfd = -1;
    int connfd = -1;
    char* N_buff,*C_buff;
    char* recv_buff;
    uint32_t N,C;
    int notwread; //how much we have left to read
    int totalread; //how much we've read so far
    int nread; //how much we've read in last read() call
    int notwritten; //how much we have left to write
    int totalsent; //how much we've written so far
    int nsent; //how much we've written in last write() call
    if(argc!=2){
        fprintf(stderr,"Error in number of args: %s", strerror(EINVAL));
        exit(EXIT_FAILURE);
    }
    // create socket similar to extra work 10
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0))==-1){
        fprintf(stderr,"Error in socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Use the SO_REUSEADDR socket option to enable reusing the port quickly after the server terminates
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1){
        fprintf(stderr,"Error in setsockopt: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // set serv_addr struct values similar to extra work 10
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(strtol(argv[1],NULL,10));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // as required in the assignment
    // bind a name to a socket similar to extra work 10
    if(bind(listenfd,(struct sockaddr*) &serv_addr,sizeof(struct sockaddr_in ))==-1){
        fprintf(stderr,"Error in bind: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // listen for connections on a socket similar to extra work 10
    if(listen(listenfd,10)==-1){
        fprintf(stderr,"Error in listen: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Handle sigint
    signal(SIGINT, sighandler);
    //initialize pcc_total
    pcc_total = (int *)calloc(95, sizeof(int));
    while (1){
        // accept a connection on a socket similar to extra work 10
        connfd = accept( listenfd,NULL,NULL);
        if( connfd < 0 ){
            fprintf(stderr,"Error in accept: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }  
        // Read N similar to extra work 10
        N_buff = (char*)&N;
        notwread = sizeof(uint32_t);
        totalread=0;
        while( notwread > 0 )
        {
            nread = read(connfd,N_buff + totalread,notwread);
            if (nread<=0){
                break;
            }
            totalread  += nread;
            notwread -= nread;
        }
        // check Client connections which may terminate unexpectedly due to TCP errors
        if (check_client_connection(nread)<0){
            continue;
        }
        N = ntohl(N);
        // Read N bytes transfered similat to extra work 10
        recv_buff = malloc(N);
        if(recv_buff==NULL){
        fprintf(stderr,"Error in malloc: %s", strerror(errno));
        exit(EXIT_FAILURE);
        }
        notwread = N;
        totalread = 0;
        while( notwread > 0 )
        {
            nread = read(connfd,recv_buff + totalread,notwread);
            if (nread<=0){
                break;
            }
            totalread  += nread;
            notwread -= nread;
        }
        // check Client connections which may terminate unexpectedly due to TCP errors
        if (check_client_connection(nread)<0){
            continue;
        }
        // update C and pcc_total
        C=0;
        for(int i=0;i<N;i++){
            int temp = (int)recv_buff[i];
            if (temp>=32 && temp<=126){
                pcc_total[temp-32]++;
                C++;
            }  
        }
        free(recv_buff);
        // Send C - number of bytes in file similar to extra work 10
        C = htonl(C);
        C_buff = (char*)&C;
        notwritten = sizeof(C);
        totalsent = 0;
        while( notwritten > 0 )
        {
            nsent = write(connfd,C_buff + totalsent,notwritten);
            if (nsent<=0){
                break;
            }
            totalsent  += nsent;
            notwritten -= nsent;
        }
        // check Client connections which may terminate unexpectedly due to TCP errors
        if (check_client_connection(nsent)<0){
            continue;
        }
        // check if sigint flag is on
        if (flag==1){
            for(int i = 0; i < 95; i++){
                printf("char '%c' : %u times\n", (char)(i+32), pcc_total[i]);
            }
            exit(0);
        }
        close(connfd);
        flag=-1;
    }
}