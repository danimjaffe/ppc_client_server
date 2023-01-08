#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#define MIN(a,b) (((a)<(b))?(a):(b))// code for minimum
#define MAX_CHUNK 1048576// one megabyte

/* argv[1]: server’s IP address (assume a valid IP address)
   argv[2]: server’s port (assume a 16-bit unsigned integer).
   argv[3]: path of the file to send.
*/
int main(int argc, char *argv[]){
    struct sockaddr_in serv_addr;
    int fd;
    int sockfd = -1;
    uint32_t N,C;
    char* N_buff,*C_buff;
    char* data_buff;
    int notwritten; //how much we have left to write
    int totalsent; //how much we've written so far
    int nsent; //how much we've written in last write() call
    int notwread; //how much we have left to read
    int totalread; //how much we've read so far
    int nread; //how much we've read in last read() call

    if(argc!=4){
        fprintf(stderr,"Error: %s", strerror(EINVAL));
        exit(EXIT_FAILURE);
    }
    // try to open specified file in argv[3]
    fd = open(argv[3],O_RDONLY);
    if (fd==-1){
        fprintf(stderr,"Error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // create socket similar to extra work 10
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1){
        fprintf(stderr,"Error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // set serv_addr struct values similar to extra work 10
    memset(&serv_addr, 0, sizeof(serv_addr));
    if (inet_pton(AF_INET, argv[1], &(serv_addr.sin_addr))!=1){
        fprintf(stderr,"Error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(strtol(argv[2],NULL,10));
    // connect socket to target similar to extra work 10
    if( connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) == -1){
        fprintf(stderr,"Error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Send N - number of bytes in file similar to extra work 10
    N = htonl(lseek(fd, 0, SEEK_END));
    N_buff = (char*)&N;
    notwritten = sizeof(N);
    totalsent=0;
    while( notwritten > 0 )
    {
      nsent = write(sockfd,N_buff + totalsent,notwritten);
      if (nsent==-1){
        fprintf(stderr,"Error: %s", strerror(errno));
        exit(EXIT_FAILURE);
      }
      totalsent  += nsent;
      notwritten -= nsent;
    }
    // Send N bytes from file similar to extra work 10
    N = ntohl(N);
    uint32_t total_bytes = N;
    lseek(fd, 0, SEEK_SET); // set fd to begginning of file
    while (total_bytes>0)
    {
        uint32_t bytes_to_transfer = MIN(total_bytes,MAX_CHUNK);
        data_buff = malloc(bytes_to_transfer);
        if(data_buff==NULL){
            fprintf(stderr,"Error in malloc: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (read(fd,data_buff,bytes_to_transfer)!=bytes_to_transfer){
            fprintf(stderr,"Error in read: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        notwritten = bytes_to_transfer;
        totalsent=0;
        while( notwritten > 0 ){
            nsent = write(sockfd,data_buff + totalsent,notwritten);
            if (nsent==-1){
                fprintf(stderr,"Error: %s", strerror(errno));
                exit(EXIT_FAILURE);
            }
            totalsent  += nsent;
            notwritten -= nsent;
        }
        total_bytes-=totalsent;
        free(data_buff);
    }
    // read C from server
    C_buff = (char *)&C;
    notwread = sizeof(uint32_t);
    totalread=0;
    while( notwread > 0 )
    {
        nread = read(sockfd,C_buff + totalread,notwread);
        if (nread==-1){
            fprintf(stderr,"Error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        totalread  += nread;
        notwread -= nread;
    }
    C = ntohl(C);
    printf("# of printable characters: %u\n",C);
    exit(EXIT_SUCCESS);
}