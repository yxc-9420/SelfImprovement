#include<winsock2.h>
#include<iostream>
#include <ws2tcpip.h>
#include "pthread.h"


using namespace std;

#pragma comment(lib, "ws2_32.lib")

//信息结构体
struct SockInfo
{
    struct sockaddr_in addr;
    int fd;
};

struct SockInfo infos[512];

void* working(void*);

int main()
{
    //初始化套接字库
    WSADATA wsData;
    WSAStartup(MAKEWORD(2,2), &wsData);
    //1、创建监听的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == fd)
    {
        perror("socket");
        return -1;
    }

    //2、绑定ip和端口
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.S_un.S_addr = INADDR_ANY; // 0 = 0.0.0.0

    int ret = bind(fd, (const sockaddr*)&saddr, sizeof(saddr));
    if(-1 == ret)
    {
        perror("bind");
        return -1;
    }

    //3、设置监听
    ret = listen(fd, 128);
    if(-1 == ret)
    {
        perror("listen");
        return -1;
    }

    //初始化结构体数组
    int max = sizeof(infos)/ sizeof(infos[0]);
    for(int i = 0; i< max; ++i)
    {
        memset(&infos[i], 0, sizeof(infos[i]));
        infos[i].fd = -1;
    }

    //4、阻塞并等待连接

    //struct sockaddr_in caddr;
    int addrlen = sizeof(struct sockaddr_in);
    while (1)
    {
        struct SockInfo* pInfo;
        for(int i = 0; i< max; ++i)
        {
            if(-1 == infos[i].fd)
            {
                pInfo = &infos[i];
                break;
            }
        }
        int cfd = accept(fd, (sockaddr*)&pInfo->addr, &addrlen);
        pInfo->fd = cfd;
        if(-1 == ret)
        {
            perror("accept");
            break;
        }
        //创建子线程
        pthread_t tid;
        //要传递ip和端口，以及通信描述符
        pthread_create(&tid,NULL,working,(void*)pInfo);
        pthread_detach(tid);
    }
    closesocket(fd);
    //释放套接字库
    WSACleanup();

    return 0;
}
    
    
void* working(void* arg)
{
    struct SockInfo* pInfo = (struct SockInfo*)arg;
    char ip[32] = {0};
    printf("客户端的IP：%s,端口：%d\n", 
            inet_ntoa(pInfo->addr.sin_addr),
            ntohs(pInfo->addr.sin_port));
    
    //5、通信
    while(1)
    {
        char buff[1024] = {0};
        int len = recv(pInfo->fd, buff, sizeof(buff), 0);
        if(len > 0)
        {
            printf("client say：%s\n", buff);
            send(pInfo->fd, buff,len, 0);
        }
        else if (0 == len)
        {
            printf("客户端已经断开了连接...\n");
            break;
        }
        else
        {
            perror("recr");
            break;
        }
    }
    closesocket(pInfo->fd);
    pInfo->fd = -1; //回收

    return NULL;
}