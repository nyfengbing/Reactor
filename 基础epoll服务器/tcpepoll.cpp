#include "net.hpp"

void setnonblocking(int fd){
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

int main(int argc, char **argv){
    //先判断main函数的输入是否正确，如果不正确并给出例子
   
    if (argc != 3) 
    { 
        printf("usage: ./tcpepoll ip port\n"); 
        printf("example: ./tcpepoll 192.168.150.128 5085\n\n"); 
        return -1; 
    }

    //创建监听套接字
    int listenfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);    //ipv4,流式套接字，更具体的协议类型
    if(listenfd < 0){
        perror("socket");
        return -1;
    }

    //设置listenfd的属性
    int opt =1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)); 
    //允许一个端口在TIME_WAIT状态下被快速重用。
    setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    //禁用 Nagle 算法，使得每次发送的数据都尽可能立即发送，而不是等待其他数据合并后一起发送。
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT,&opt,sizeof(opt));    
    //允许多个进程或线程绑定到同一个端口。
    setsockopt(listenfd,SOL_SOCKET,SO_KEEPALIVE,&opt,sizeof(opt));
    //启用TCP保活机制，定期向空闲连接发送探测包，检查连接是否仍然活着。如果对方没有响应，连接将被认为已经断开，系统会自动关闭该连接。
    /*setsockopt函数参数的意义 
     sockfd: 套接字文件描述符，指定要设置选项的套接字
     level: 协议层，通常为 SOL_SOCKET (套接字层)，或 IPPROTO_TCP (TCP 协议)
     optname: 要设置的选项名称，如 SO_REUSEADDR、SO_RCVBUF、TCP_NODELAY 等
     optval: 指向选项值的指针，根据选项类型不同，这里存储选项的值（如整数、结构体等）
     optlen: optval 所指向内存区域的大小（单位：字节）
    */

    setnonblocking(listenfd);

    struct sockaddr_in servaddr;        //服务器地址的结构体
    servaddr.sin_family = AF_INET;      //IPV4网络协议的套接字类型
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);  //将ipv4转为二进制数字
    servaddr.sin_port = htons(atoi(argv[2]));   //服务器端用于监听的端口

    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(sockaddr)) < 0){
        perror("bind");
        close(listenfd);
        return -1;
    }
    //设置最大监听序列为128
    if(listen(listenfd,128) != 0){
        perror("listen() failed");
        close(listenfd);
        return -1;
    }

    //创建红黑树句柄
    int epfd = epoll_create(1);

    //为服务端的listenfd准备读事件
    epoll_event ev;     //声明事件的数据结构
    ev.data.fd = listenfd;  //指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回
    ev.events = EPOLLIN;    //让epoll监视listenfd的读事件，采用水平触发

    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);     //把需要监视的listenfd和它的事件加入epfd中

    epoll_event evs[10];            //存放epoll_wait()返回事件的数组

    while (1)
    {
        int infds = epoll_wait(epfd,evs,10,-1); //红黑树文件描述符，返回数组，接受返回的大小，间隔时间
        //如果infds返回值为0的话证明返回失败
        if (infds == 0)
        {
            printf("epoll_wait() timeout\n");
            continue;
        }

        //如果infds返回值大于0的话就是返回的事件的发生个数
        for (int ii = 0; ii<infds; ii++){
            if(evs[ii].data.fd == listenfd){
                struct sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);
                //监听listenfd，传入clientaddr的结构体包括这个结构体的信息，指向一个缓存内存的指针（返回后将地址长度填入这段内存）
                int clientfd = accept(listenfd,(struct sockaddr*)&clientaddr,&len);
                //客户端的IO设置为非阻塞的
                setnonblocking(clientfd);
                //打印建立连接的fd的信息
                printf("accept clientfd(fd = %d, ip = %s, port = %d) ok\n",
                        clientfd,
                        inet_ntoa(clientaddr.sin_addr),
                        ntohs(clientaddr.sin_port));
                //为新的客户端连接准备读事件，并添加到epoll
                ev.data.fd = clientfd;
                ev.events  = EPOLLIN|EPOLLET;   //设置边缘触发；
                epoll_ctl(epfd,EPOLL_CTL_ADD,clientfd,&ev); //将新的clientfd加入到树上
            }else   //如果是客户端连接的fd有事件
            {  
                if(evs[ii].events&EPOLLRDHUP){
                    printf("client(eventfd = %d)disconnected\n",evs[ii].data.fd);
                    close(evs[ii].data.fd);
                }
                else if (evs[ii].events&(EPOLLIN|EPOLLPRI)){
                    char buf[1024];
                    while(1){
                        bzero(&buf,sizeof(buf));
                        ssize_t nread = read(evs[ii].data.fd,buf,sizeof(buf));
                        if(nread > 0){  //表示成功接收到数据
                            //把接收到的数据原封不动的还回去
                            printf("recv(evenfd = %d):%s\n",evs[ii].data.fd,buf);
                            send(evs[ii].data.fd,buf,strlen(buf),0);
                        }else if (nread == -1 && errno == EINTR)    //表示读取数据的时候被中断，继续读取
                        {
                            continue;
                        }else if (nread == -1 && ((errno == EAGAIN)||(errno == EWOULDBLOCK)))    //表示全部数据以及读取完毕
                        {
                            break;
                        }else if(nread == 0){
                            printf("client(eventfd = %d)disconnected\n",evs[ii].data.fd);
                            close(evs[ii].data.fd);
                            break;
                        }
                    }
                }
                else if (evs[ii].events&EPOLLOUT){

                }else{
                    printf("clients(evenfd=%d)error\n",evs[ii].data.fd);
                    close(evs[ii].data.fd);
                }
            }
        }
    }
    
    system("pause");
    return 0;
}



