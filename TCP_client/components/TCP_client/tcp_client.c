/**
 * @file tcp_client.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-10-11
 *
 * @copyright Copyright (c) 2023
 *
*/
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "log.h"
//tcp 
#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "tcp_client.h"
#define DBG_TAG "TCP CLIENT"

static int socket_fd = -1;

static xTaskHandle tcp_read;
static tcp_read_handler_t* read_handle_cb;
/**
 * @brief TCP初始化函数
 *
 * @return int
*/
int tcpClientInit(void)
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);//创建socket
    if (socket_fd<0) return socket_fd;//创建失败返回
    else return 0; //创建成功返回 0
}
/**
 * @brief TCP连接函数
 *
 * @param addr
 * @param port
 * @return int
*/
int tcpClientConentStart(char* addr, uint16_t port)
{
    //判断socket 是否创建成功
    if (socket_fd<0) {
        LOG_E("socket is no creat");
        return -1;
    }
    struct sockaddr_in remote_addr = {
        .sin_addr.s_addr = inet_addr(addr), //配置IP地址
        .sin_port = htons(port),//配置端口号
        .sin_family = AF_INET,//配置连接类型为TCP
    };

    memset(&(remote_addr.sin_zero), 0, sizeof(remote_addr.sin_zero));

    if (connect(socket_fd, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr)) != 0) {//发起连接
        LOG_E("TCP client connect server falied!"); //连接失败显示
        closesocket(socket_fd);//关闭socket
        return -1;//返回错误
    }
    LOG_I("TCP client connect server success");//连接成功显示
    return 0; //返回OK
}
/**
 * @brief TCP 发送函数
 *
 * @param data
 * @return int
*/
int tcpClientSend(char* data)
{
    size_t ret;
    ret = write(socket_fd, data, strlen(data));
    if (ret<0) {
        LOG_E("TCP client send falied!");
        return -1;
    }
    return ret;
}


/**
 * @brief
 *
 * @param arg
*/
static void read_task(void* arg)
{
    char* buff = pvPortMalloc(1024);
    uint64_t ret = 0;
    while (1) {
        memset(buff, 0, 1024);
        ret = recv(socket_fd, buff, 1024, 0);
        if (ret<=0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        *read_handle_cb(buff, ret);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    vPortFree(buff);
}
/**
 * @brief TCP读取函数
 *
 * @param buff
 * @return int
*/
int tcpClientRead(tcp_read_handler_t read_handler)
{
    if (tcp_read==NULL)
    {
        xTaskCreate(read_task, "tcp read", 1024*2, NULL, 3, &tcp_read);
        read_handle_cb = read_handler;
    }
    else {
        LOG_F("tcp_read runing");
    }

    return 0;
}
/**
 * @brief TCP断开连接
 *
 * @return int
*/
int tcpClientClose(void)
{
    if (tcp_read != NULL)vTaskDelete(tcp_read);
    shutdown(SHUT_RDWR, socket_fd);
    closesocket(socket_fd);
}
