/**
 * @file wifi_event.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-06-29
 *
 * @copyright Copyright (c) 2023
 *
*/
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "bl_fw_api.h"
#include "wifi_mgmr_ext.h"
#include "wifi_mgmr.h"
#include "bflb_irq.h"
#include "bflb_uart.h"
#include "bflb_l1c.h"
#include "bflb_mtimer.h"

#include "bl616_glb.h"
#include "rfparam_adapter.h"

#include "board.h"
#include "log.h"
//tcp
#include "tcp_client.h"

#define DBG_TAG "WIFI EVENT"

#define WIFI_STACK_SIZE     (1024*4)
#define TASK_PRIORITY_FW    (16)

static wifi_conf_t conf =
{
    .country_code = "CN",
};
static TaskHandle_t wifi_fw_task;
static TaskHandle_t wifi_con_task;
static uint32_t sta_ConnectStatus = 0;
static char* SSID = NULL;
static char* PASS = NULL;

/**
 * @brief WiFi 任务
 *
 * @return int
*/
int wifi_start_firmware_task(void)
{
    LOG_I("Starting wifi ...");

    /* enable wifi clock */

    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_IP_WIFI_PHY | GLB_AHB_CLOCK_IP_WIFI_MAC_PHY | GLB_AHB_CLOCK_IP_WIFI_PLATFORM);
    GLB_AHB_MCU_Software_Reset(GLB_AHB_MCU_SW_WIFI);

    /* set ble controller EM Size */

    GLB_Set_EM_Sel(GLB_WRAM160KB_EM0KB);

    if (0 != rfparam_init(0, NULL, 0)) {
        LOG_I("PHY RF init failed!");
        return 0;
    }

    LOG_I("PHY RF init success!");

    /* Enable wifi irq */

    extern void interrupt0_handler(void);
    bflb_irq_attach(WIFI_IRQn, (irq_callback)interrupt0_handler, NULL);
    bflb_irq_enable(WIFI_IRQn);

    xTaskCreate(wifi_main, (char*)"fw", WIFI_STACK_SIZE, NULL, TASK_PRIORITY_FW, &wifi_fw_task);

    return 0;
}
/**
 * @brief wifi event handler
 *      WiFi 事件回调
 *
 * @param code
*/
void wifi_event_handler(uint32_t code)
{

    sta_ConnectStatus = code;
    BaseType_t xHigherPriorityTaskWoken;
    switch (code) {
        case CODE_WIFI_ON_INIT_DONE:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_INIT_DONE", __func__);
            wifi_mgmr_init(&conf);
        }
        break;
        case CODE_WIFI_ON_MGMR_DONE:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_MGMR_DONE", __func__);

        }
        break;
        case CODE_WIFI_ON_SCAN_DONE:
        {

            wifi_mgmr_sta_scanlist();
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_SCAN_DONE SSID numbles:%d", __func__, wifi_mgmr_sta_scanlist_nums_get());

        }
        break;
        case CODE_WIFI_ON_CONNECTED:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_CONNECTED", __func__);
            void mm_sec_keydump();
            mm_sec_keydump();
        }
        break;
        case CODE_WIFI_ON_GOT_IP:
        {

            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_GOT_IP", __func__);
        }
        break;
        case CODE_WIFI_ON_DISCONNECT:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_DISCONNECT", __func__);

        }
        break;
        case CODE_WIFI_ON_AP_STARTED:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_AP_STARTED", __func__);
        }
        break;
        case CODE_WIFI_ON_AP_STOPPED:
        {
            LOG_I("[APP] [EVT] %s, CODE_WIFI_ON_AP_STOPPED", __func__);
        }
        break;
        case CODE_WIFI_ON_AP_STA_ADD:
        {
            LOG_I("[APP] [EVT] [AP] [ADD] %lld", xTaskGetTickCount());
        }
        break;
        case CODE_WIFI_ON_AP_STA_DEL:
        {
            LOG_I("[APP] [EVT] [AP] [DEL] %lld", xTaskGetTickCount());
        }
        break;
        default:
        {
            LOG_I("[APP] [EVT] Unknown code %u ", code);
        }
    }
}
/**
 * @brief 读取回调
 *
 * @param buff
 * @param len
*/
static void tcp_read_cb(char* buff, signed int len)
{
    LOG_I("TCP client read %d:%s", len, buff);
    if (memcmp(buff, "close", len)==0) {
        LOG_F("tcp client close.....");
        tcpClientClose();
    }
}

static void wifi_connect_task(void* arg)
{
    int ret = 255;
    // struct fhost_vif_ip_addr_cfg ip_cfg = { 0 };
    uint32_t ipv4_addr = 0;

    while (1)
    {
        if (NULL==SSID || 0==strlen(SSID)) {
            goto  __suTsk;
        }
        if (wifi_mgmr_sta_state_get() == 1) {
            wifi_sta_disconnect();
        }
        if (wifi_sta_connect(SSID, PASS, NULL, NULL, 0, 0, 0, 1)) {
            goto  __suTsk;
        }

        //等待连接成功
        sta_ConnectStatus = 0;
        for (int i = 0;i<10*30;i++) {

            vTaskDelay(100/portTICK_PERIOD_MS);
            switch (sta_ConnectStatus) {
                case CODE_WIFI_ON_MGMR_DONE:
                    goto  __suTsk;
                case CODE_WIFI_ON_SCAN_DONE:

                    goto  __suTsk;
                case CODE_WIFI_ON_DISCONNECT:	//连接失败（超过了重连次数还没有连接成功的状态）

                    goto  __suTsk;
                case CODE_WIFI_ON_CONNECTED:	//连接成功(表示wifi sta状态的时候表示同时获取IP(DHCP)成功，或者使用静态IP)
                    break;
                case CODE_WIFI_ON_GOT_IP:
                    // tcp_client_init();
                    tcpClientInit();
                    tcpClientConentStart("122.114.122.174", 35810);
                    tcpClientSend("Hello Server!");
                    tcpClientRead(tcp_read_cb);
                    goto  __suTsk;
                default:
                    //等待连接成功
                    break;
            }
        }
    __suTsk:
        vTaskSuspend(wifi_con_task);
    }
}

uint8_t wifi_connect(char* ssid, char* passwd)
{
    SSID = ssid;
    PASS = passwd;
    if (wifi_con_task==NULL)
        xTaskCreate(wifi_connect_task, "wifi_con_tak", 1024, 2, NULL, &wifi_con_task);
    else
        vTaskResume(wifi_con_task);
}
