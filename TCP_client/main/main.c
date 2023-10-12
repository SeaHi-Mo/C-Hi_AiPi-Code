/**
 * @file main.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-08-22
 *
 * @copyright Copyright (c) 2023
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include "board.h"
#include "log.h"
#include "wifi_event.h"
#include "bflb_mtimer.h"

#define DBG_TAG "MAIN"


int main(void)
{
    board_init();
    tcpip_init(NULL, NULL);
    wifi_start_firmware_task();
    wifi_connect("AIOT@FAE", "fae12345678");

    vTaskStartScheduler();
}
