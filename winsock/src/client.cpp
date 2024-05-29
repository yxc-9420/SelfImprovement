#include<winsock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
int main()
{
    //初始化套接字库
    WSADATA wsData;
    WSAStartup(MAKEWORD(2,2), &wsData);
    //1、创建通信的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == fd)
    {
        perror("socket");
        return -1;
    }

    //连接服务器的ip
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//172.16.1.240
    saddr.sin_port = htons(9999);

    int ret = connect(fd, (const sockaddr*)&saddr, sizeof(saddr));
    if(-1 == ret)   
    {
        perror("connect");
        return -1;
    }

    int num = 0;
    //3、通信
    while(1)
    {
        //发送数据
        char buff[1024] = {0};
        sprintf(buff,"你好，hello，world，%d...\n", num++);
        send(fd, buff,strlen(buff) + 1, 0);
        //接收数据
        memset(buff, 0, sizeof(buff));
        int len = recv(fd, buff, sizeof(buff), 0);
        if(len > 0)
        {
            printf("server say：%s\n", buff);
        }
        else if (0 == len)
        {
            printf("服务器端已经断开了连接...\n");
        }
        else
        {
            perror("recr");
            break;
        }
        Sleep(1000);
    }
    closesocket(fd);
    return 0; 
}