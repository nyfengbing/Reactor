#include "net.hpp"

int main(int argc,char **argv){
    if (argc != 3) 
    { 
        printf("usage: ./tcpepoll ip port\n"); 
        printf("example: ./tcpepoll 192.168.150.128 5085\n\n"); 
        return -1; 
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];

    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        perror("socket");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port        = htons(atoi(argv[2]));  // 注意这里用 htons

    if (connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) != 0){
        printf("connect(%s:%s)failed\n",argv[1],argv[2]);
        close(sockfd);
        return -1;
    }
    printf("connect ok\n");
    
    for(int ii = 0;ii<200000;ii++){
        memset(buf,0,sizeof(buf));
        printf("please input:");
        if (scanf("%s", buf) != 1) {
            perror("scanf failed");
            exit(EXIT_FAILURE);
        }

        if(send(sockfd,buf,sizeof(buf),0)<=0){
            printf("write() failed\n");
            close(sockfd);
            return -1;
        }
        memset(buf,0,sizeof(buf));
        if (recv(sockfd,buf,sizeof(buf),0)<=0){
            printf("read error\n");
            close(sockfd);
            return -1;
        }
        printf("recv:%s\n",buf);
    }
    return 0;
}