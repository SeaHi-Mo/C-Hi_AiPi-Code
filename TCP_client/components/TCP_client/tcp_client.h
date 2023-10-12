/**
 * @file tcp_client.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-10-11
 *
 * @copyright Copyright (c) 2023
 *
*/
#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

typedef void* (tcp_read_handler_t)(char* buff, signed int len);

int tcpClientInit(void);
int tcpClientConentStart(char* addr, uint16_t port);
int tcpClientSend(char* data);
int tcpClientRead(tcp_read_handler_t read_handler);
int tcpClientClose(void);
#endif