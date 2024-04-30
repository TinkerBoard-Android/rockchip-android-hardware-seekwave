/******************************************************************************
 *
 *  Copyright (C) 2020-2021 SeekWave Technology
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      scom_vendor.c
 *
 *  Description:   serials communication operation
 *
 ******************************************************************************/

#include <pthread.h>
#include <utils/Log.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <cutils/sockets.h>
#include <sys/poll.h>
#include <assert.h>
#include "scom_vendor.h"
#include "bt_hci_bdroid.h"
#include "skw_common.h"
#include "bt_vendor_skw.h"
#include "skw_btsnoop.h"
#include "skw_log.h"
#include "skw_gen_addr.h"



bt_hw_cfg_cb_st      hw_cfg_cb;
scomm_vnd_st         scomm_vnd[BT_COM_PORT_SIZE];
skw_socket_object_st skw_socket_object;
uint8_t              recvSocketBuf[4096];
static pthread_mutex_t write2host_lock;
uint16_t             chip_version = 0;
#define SKWBT_NV_FILE_PATH       "/vendor/etc/bluetooth"


static const uint8_t hci_preamble_sizes[] =
{
    0,
    HCI_COMMAND_PKT_PREAMBLE_SIZE,
    HCI_ACLDATA_PKT_PREAMBLE_SIZE,
    HCI_SCODATA_PKT_PREAMBLE_SIZE,
    HCI_EVENT_PKT_PREAMBLE_SIZE,
    HCI_ISODATA_PKT_PREAMBLE_SIZE, 
    0,
    HCI_EVENT_SKWLOG_PREAMBLE_SIZE
};


static tUSERIAL_CFG userial_H4_cfg =
{
    (USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
    //USERIAL_BAUD_115200,
    USERIAL_BAUD_3M,
    //USERIAL_BAUD_1_5M,
    USERIAL_HW_FLOW_CTRL_ON
};


char hex2char(int num)
{
    char ch;
    if(num >= 0 && num <= 9)
    {
        ch = num + 48;
    }
    else if(num > 9 && num <= 15)
    {
        ch = (num - 10) + 65;
    }
    else
    {
        ch = '\0';
    }
    return ch;
}

void hex2String(unsigned char hex[], unsigned char str[], int N)
{
    int i = 0, j;
    for(i = 0, j = 0; i < N; i++, j += 2)
    {
        str[j] = hex2char((hex[i] & 0xF0) >> 4);
        str[j + 1] = hex2char(hex[i] & 0x0F);
    }
    str[N << 1] = 0;
}


void scomm_vendor_init()
{
    for(uint8_t i = 0; i < BT_COM_PORT_SIZE; i++)
    {
        memset(&scomm_vnd[i], 0, sizeof(scomm_vnd_st));
        scomm_vnd[i].fd           = -1;
        scomm_vnd[i].driver_state = FALSE;
        scomm_vnd[i].mode         = O_RDWR;
    }

    pthread_mutex_init(&write2host_lock, NULL);
}

void scomm_vendor_set_port_name(uint8_t port_index, char *port_name, int mode)
{
    memset(scomm_vnd[port_index].port_name, 0, DEVICE_NODE_MAX_LEN);
    memcpy(scomm_vnd[port_index].port_name, port_name, strlen(port_name));
    scomm_vnd[port_index].mode = mode;

    ALOGD("skwbt_device_node:%s, port:%d", port_name, port_index);
}


/*******************************************************************************
**
** Function        scomm_vendor_tcio_baud
**
** Description     helper function converts USERIAL baud rates into TCIO
**                  conforming baud rates
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t scomm_vendor_tcio_baud(uint8_t cfg_baud, uint32_t *baud)
{
    if (cfg_baud == USERIAL_BAUD_115200)
    {
        *baud = B115200;
    }
    else if (cfg_baud == USERIAL_BAUD_4M)
    {
        *baud = B4000000;
    }
    else if (cfg_baud == USERIAL_BAUD_3M)
    {
        *baud = B3000000;
    }
    else if (cfg_baud == USERIAL_BAUD_2M)
    {
        *baud = B2000000;
    }
    else if (cfg_baud == USERIAL_BAUD_1M)
    {
        *baud = B1000000;
    }
    else if (cfg_baud == USERIAL_BAUD_1_5M)
    {
        *baud = B1500000;
    }
    else if (cfg_baud == USERIAL_BAUD_921600)
    {
        *baud = B921600;
    }
    else if (cfg_baud == USERIAL_BAUD_460800)
    {
        *baud = B460800;
    }
    else if (cfg_baud == USERIAL_BAUD_230400)
    {
        *baud = B230400;
    }
    else if (cfg_baud == USERIAL_BAUD_57600)
    {
        *baud = B57600;
    }
    else if (cfg_baud == USERIAL_BAUD_19200)
    {
        *baud = B19200;
    }
    else if (cfg_baud == USERIAL_BAUD_9600)
    {
        *baud = B9600;
    }
    else if (cfg_baud == USERIAL_BAUD_1200)
    {
        *baud = B1200;
    }
    else if (cfg_baud == USERIAL_BAUD_600)
    {
        *baud = B600;
    }
    else
    {
        ALOGE( "userial vendor open: unsupported baud idx %i", cfg_baud);
        *baud = B115200;
        return FALSE;
    }

    return TRUE;
}


/*******************************************************************************
**
** Function        scomm_vendor_uart_open
**
** Description     Uart Open
**
** Returns         None
**
*******************************************************************************/
int scomm_vendor_uart_open(uint8_t port_index)
{
    tUSERIAL_CFG *p_cfg = &userial_H4_cfg;
    uint32_t baud;
    uint8_t data_bits;
    uint16_t parity;
    uint8_t stop_bits;

    scomm_vnd[port_index].fd = -1;

    if (!scomm_vendor_tcio_baud(p_cfg->baud, &baud))
    {
        return -1;
    }

    if(p_cfg->fmt & USERIAL_DATABITS_8)
    {
        data_bits = CS8;
    }
    else if(p_cfg->fmt & USERIAL_DATABITS_7)
    {
        data_bits = CS7;
    }
    else if(p_cfg->fmt & USERIAL_DATABITS_6)
    {
        data_bits = CS6;
    }
    else if(p_cfg->fmt & USERIAL_DATABITS_5)
    {
        data_bits = CS5;
    }
    else
    {
        ALOGE("userial vendor open: unsupported data bits");
        return -1;
    }

    if(p_cfg->fmt & USERIAL_PARITY_NONE)
    {
        parity = 0;
    }
    else if(p_cfg->fmt & USERIAL_PARITY_EVEN)
    {
        parity = PARENB;
    }
    else if(p_cfg->fmt & USERIAL_PARITY_ODD)
    {
        parity = (PARENB | PARODD);
    }
    else
    {
        ALOGE("userial vendor open: unsupported parity bit mode");
        return -1;
    }

    if(p_cfg->fmt & USERIAL_STOPBITS_1)
    {
        stop_bits = 0;
    }
    else if(p_cfg->fmt & USERIAL_STOPBITS_2)
    {
        stop_bits = CSTOPB;
    }
    else
    {
        ALOGE("userial vendor open: unsupported stop bits");
        return -1;
    }

    ALOGI("userial vendor open: opening %s,baud:%d", scomm_vnd[port_index].port_name, p_cfg->baud);

    if ((scomm_vnd[port_index].fd = open(scomm_vnd[port_index].port_name, O_RDWR)) == -1)
    {
        ALOGE("userial vendor open: unable to open %s, %s", scomm_vnd[port_index].port_name, strerror(errno));
        return -1;
    }

    tcflush(scomm_vnd[port_index].fd, TCIOFLUSH);

    tcgetattr(scomm_vnd[port_index].fd, &scomm_vnd[port_index].termios);
    cfmakeraw(&scomm_vnd[port_index].termios);

    if(p_cfg->hw_fctrl == USERIAL_HW_FLOW_CTRL_ON)
    {
        ALOGI("userial vendor open: with HW flowctrl ON");
        scomm_vnd[port_index].termios.c_cflag |= (CRTSCTS | stop_bits | parity);
    }
    else
    {
        ALOGI("userial vendor open: with HW flowctrl OFF");
        scomm_vnd[port_index].termios.c_cflag &= ~CRTSCTS;
        scomm_vnd[port_index].termios.c_cflag |= (stop_bits | parity);

    }

    tcsetattr(scomm_vnd[port_index].fd, TCSANOW, &scomm_vnd[port_index].termios);
    tcflush(scomm_vnd[port_index].fd, TCIOFLUSH);

    tcsetattr(scomm_vnd[port_index].fd, TCSANOW, &scomm_vnd[port_index].termios);
    tcflush(scomm_vnd[port_index].fd, TCIOFLUSH);
    tcflush(scomm_vnd[port_index].fd, TCIOFLUSH);

    /* set input/output baudrate */
    cfsetospeed(&scomm_vnd[port_index].termios, baud);
    cfsetispeed(&scomm_vnd[port_index].termios, baud);
    tcsetattr(scomm_vnd[port_index].fd, TCSANOW, &scomm_vnd[port_index].termios);


    ALOGE("UART device fd = %d Open", scomm_vnd[port_index].fd);


    return scomm_vnd[port_index].fd;

}


/*******************************************************************************
**
** Function        scomm_vendor_usbsdio_open
**
** Description     check port name valid
**
** Returns         None
**
*******************************************************************************/
uint8_t scomm_vendor_check_port_valid(uint8_t port_index)
{
    if(scomm_vnd[port_index].port_name[0] == 0)
    {
        return FALSE;
    }
    return TRUE;
}



/*******************************************************************************
**
** Function        scomm_vendor_usbsdio_open
**
** Description     USB/SDIO Open
**
** Returns         None
**
*******************************************************************************/
int scomm_vendor_usbsdio_open(uint8_t port_index)
{
    if ((scomm_vnd[port_index].fd = open(scomm_vnd[port_index].port_name, O_RDWR)) == -1)
    {
        ALOGE("%s: unable to open %s: %s", __func__, scomm_vnd[port_index].port_name, strerror(errno));
        return -1;
    }
    ALOGD("USB/SDIO device[%d], %s fd = %d open", port_index, scomm_vnd[port_index].port_name, scomm_vnd[port_index].fd);

    return scomm_vnd[port_index].fd;
}


/*******************************************************************************
**
** Function        scomm_vendor_recv_rawdata
**
** Description     recv data from host and process
**
** Returns         None
**
*******************************************************************************/
uint8_t pkt_cnts = 0;
static void scomm_vendor_recv_rawdata(void *context)
{
    SKW_UNUSED(context);
    uint8_t port_index = 0;//(uint8_t)context;
    ssize_t  bytes_read = 0;
    uint8_t  pkt_type = 0, offset = 1;
    uint16_t need_read_lens = 0, total_len = 0;
    int16_t  rev_len = 0;

    //SKWBT_LOG("%s [%d] start", __func__, port_index);

    //do{

    //-----------type-------------//
    //do
    {
        rev_len = read(scomm_vnd[port_index].uart_fd[1], recvSocketBuf, 1);
        if(rev_len <= 0)
        {
            ALOGE("%s type read err, rev_len:%d", __func__, rev_len);
            return ;
        }
        bytes_read += rev_len;
    }
    //while (bytes_read < 1);

    total_len = 1;
    pkt_type = recvSocketBuf[0];
    if((pkt_type == HCI_COMMAND_PKT) || (pkt_type == HCI_ACLDATA_PKT) || (pkt_type == HCI_SCODATA_PKT) || (pkt_type == HCI_ISO_PKT))
    {
        need_read_lens = hci_preamble_sizes[pkt_type];
    }
    else
    {
        ALOGE("%s invalid data type: %d", __func__, pkt_type);
        assert(0);
    }
    offset += need_read_lens;

    //SKWBT_LOG("pkt_type:%d, bytes_read:%zd, offset:%d", pkt_type, bytes_read, offset);

    //-----------header-------------//
    bytes_read = 0;
    offset = 1;
    do
    {
        rev_len = read(scomm_vnd[port_index].uart_fd[1], recvSocketBuf + offset + bytes_read, need_read_lens);
        if(rev_len < 0)
        {
            ALOGE("%s header read err, rev_len:%d", __func__, rev_len);
            return ;
        }
        bytes_read += rev_len;
        //SKWBT_LOG("header bytes_read:%zd, need_read_lens:%d", bytes_read, need_read_lens);
    } while (bytes_read < need_read_lens);
    offset += need_read_lens;

    total_len += need_read_lens;
    //get payload length
    if((pkt_type == HCI_ACLDATA_PKT) || (pkt_type == HCI_ISO_PKT))
    {
        need_read_lens = *(uint16_t *)&recvSocketBuf[HCI_COMMON_DATA_LENGTH_INDEX];
    }
    else if((pkt_type == HCI_EVENT_PKT) || (pkt_type == HCI_EVENT_SKWLOG))
    {
        need_read_lens = recvSocketBuf[HCI_EVENT_DATA_LENGTH_INDEX];
    }
    else//cmd/sco
    {
        need_read_lens = recvSocketBuf[HCI_COMMON_DATA_LENGTH_INDEX];
    }
    //-----------payload-------------//
    bytes_read = 0;
    if(need_read_lens > 0)
    {
        do
        {
            rev_len = read(scomm_vnd[port_index].uart_fd[1], recvSocketBuf + offset + bytes_read, need_read_lens);
            if(rev_len < 0)
            {
                ALOGE("%s header read err, rev_len:%d", __func__, rev_len);
                return ;
            }
            bytes_read += rev_len;
            //SKWBT_LOG("payload bytes_read:%zd, need_read_lens:%d", bytes_read, need_read_lens);
        } while (bytes_read < need_read_lens);
        total_len += need_read_lens;
    }


    uint8_t str_buffer[2056] = {0};
    hex2String(recvSocketBuf, str_buffer, (total_len > 64) ? 64 : total_len);

    if((skwbt_transtype & SKWBT_TRANS_TYPE_UART) && (skwbtuartonly == FALSE) && (skwbtNoSleep == FALSE) && (btpw_fp > 0))//uart
    {
        char tmp_buf[6] = {0};
        int r_len;

        tmp_buf[0] = pkt_cnts ++;
        r_len = write(btpw_fp, tmp_buf, 1);
        SKWBT_LOG("r_len:%d, btpw_fp:%d, pkt_cnts:%d", r_len, btpw_fp, pkt_cnts);
    }

    uint16_t length = total_len;
    uint16_t transmitted_length = 0;
    uint8_t  send_port = BT_COM_PORT_CMDEVT;
    if(skwbt_transtype & SKWBT_TRANS_TYPE_SDIO)
    {
        switch(recvSocketBuf[0])
        {
            case HCI_ACLDATA_PKT:
                send_port = BT_COM_PORT_ACL;
                break;
            case HCI_SCODATA_PKT:
                send_port = BT_COM_PORT_AUDIO;
                break;
            case HCI_ISO_PKT:
                send_port = BT_COM_PORT_ISO;
                break;
            default:
                send_port = BT_COM_PORT_CMDEVT;
                break;
        }
    }


    SKWBT_LOG("total_len:%d, port:%d, %s", total_len, send_port, str_buffer);

    skw_btsnoop_capture(recvSocketBuf, FALSE);



    while((length > 0) && scomm_vnd[send_port].driver_state)
    {
        ssize_t ret = write(scomm_vnd[send_port].fd, recvSocketBuf + transmitted_length, length);

        switch (ret)
        {
            case -1:
                ALOGE("In %s, error writing to the scomm: %s", __func__, strerror(errno));
                return ;
            //break;
            case 0:
                ALOGE("%s, ret %zd", __func__, ret);
                break;
            default:
                transmitted_length += ret;
                length -= ret;
                //break;
        }
    }

    SKWBT_LOG("%s [%d] end", __func__, port_index);
}

static void *scomm_vendor_recv_socket_thread(void *arg)
{
    //SKW_UNUSED(arg);
    uint8_t port_index = (uint8_t)arg;
    struct epoll_event events[64];
    int j, ret;

    ALOGD("%s [%d] start", __func__, port_index);

    while(scomm_vnd[port_index].thread_running)
    {
        do
        {
            ret = epoll_wait(scomm_vnd[port_index].epoll_fd, events, 32, 500);

            //ALOGE("recv_socket_thread ret:%d, state:%d", ret, scomm_vnd[port_index].thread_running);

        } while(scomm_vnd[port_index].thread_running && (ret == -1) && (errno == EINTR));

        if (ret < 0)
        {
            ALOGE("%s error in epoll_wait:%d, %s", __func__, ret, strerror(errno));
        }
        for (j = 0; j < ret; ++j)
        {
            skw_socket_object_st *object = (skw_socket_object_st *)events[j].data.ptr;
            if (events[j].data.ptr == NULL)
            {
                continue;
            }
            else
            {
                if (events[j].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR) && object->read_ready)
                {
                    object->read_ready(object->context);
                    //object->read_ready(port_index);
                }
                if (events[j].events & EPOLLOUT && object->write_ready)
                {
                    object->write_ready(object->context);
                    //object->read_ready(port_index);
                }
            }
        }
    }

    ALOGD("%s [%d] exit", __func__, port_index);
    return NULL;
}


/*******************************************************************************
**
** Function        scomm_vendor_send_to_host
**
** Description     send data to host
**
** Returns         None
**
*******************************************************************************/
static void scomm_vendor_send_to_host(uint8_t port_index, unsigned char *buffer, unsigned int total_length)
{
    unsigned int length = total_length;
    uint16_t transmitted_length = 0;
    ssize_t ret;


    pthread_mutex_lock(&write2host_lock);
    while ((length > 0) && scomm_vnd[port_index].thread_running)
    {
        RW_NO_INTR(ret = write(scomm_vnd[port_index].uart_fd[1], buffer + transmitted_length, length));

        SKWBT_LOG("write to host ret:%zd", ret);
        switch (ret)
        {
            case -1:
                ALOGE("In %s, error writing to socket: %s", __func__, strerror(errno));
                break;
            case 0:
                break;
            default:
                transmitted_length += ret;
                length -= ret;
                break;
        }
    }
    pthread_mutex_unlock(&write2host_lock);

    SKWBT_LOG("write to host[%d] total_length:%d, ret:%zd", port_index, total_length, ret);
}


/*******************************************************************************
**
** Function        scomm_vendor_send_hw_error
**
** Description     send HCI_HW_ERR command to hsot
**
** Returns         None
**
*******************************************************************************/

void scomm_vendor_send_hw_error()
{
    unsigned char p_buf[10];

    ALOGE("%s, CP Error", __func__);

    p_buf[0] = HCI_EVENT_PKT;//event
    p_buf[1] = HCI_HARDWARE_ERROR_EVENT;//hardware error
    p_buf[2] = 0x01;//len
    p_buf[3] = HWERR_CODE_CP_ERROR;//userial error code
    scomm_vendor_send_to_host(0, p_buf, 4);
}

/*******************************************************************************
**
** Function        scomm_vendor_find_valid_type
**
** Description     find the index of valid packet type
**
** Returns         None
**
*******************************************************************************/
int scomm_vendor_find_valid_type(uint8_t *buffer, uint16_t len)
{
    for(int i = 0; i < len; i++)
    {
        switch(buffer[i])
        {
            case HCI_EVENT_PKT:
            case HCI_ACLDATA_PKT:
            case HCI_SCODATA_PKT:
            //case HCI_COMMAND_PKT:
            case HCI_EVENT_SKWLOG:
                return i;
            default:
                break;
        }
#if 0
        if((HCI_EVENT_PKT == buffer[i]) || (HCI_ACLDATA_PKT == buffer[i]) || (HCI_SCODATA_PKT == buffer[i]) || (HCI_COMMAND_PKT == buffer[i])
                || (HCI_EVENT_SKWLOG == buffer[i]))
        {
            return i;
        }
#endif
    }
    return len;
}


/*******************************************************************************
**
** Function        scomm_vendor_recv_scomm_thread
**
** Description     recv data from UART/USB/SDIO and process
**
** Returns         None
**
*******************************************************************************/
static void *scomm_vendor_recv_scomm_thread(void *arg)
{
    //SKW_UNUSED(arg);
    uint8_t port_index = (uint8_t)arg;
    struct pollfd pfd[2];
    scomm_vnd_st *scomm = &scomm_vnd[port_index];
    pfd[0].events = POLLIN | POLLHUP | POLLERR | POLLRDHUP;
    pfd[0].fd = scomm->signal_fd[1];
    pfd[1].events = POLLIN | POLLHUP | POLLERR | POLLRDHUP;
    pfd[1].fd = scomm->fd;

    uint8_t   read_buffer[2056] = {0};
    uint8_t   str_buffer[2056] = {0};
    ssize_t   bytes_read;
    uint16_t  last_len = 0, rev_len = 0;
    int       ret;

    scomm->recv_comm_thread_running = TRUE;
    ALOGD("%s [%d] start", __func__, port_index);

    while(scomm->thread_running)
    {
        do
        {
            ret = poll(pfd, 2, 500);
        } while(ret == -1 && errno == EINTR && scomm->thread_running);
        //exit signal is always at first index
        if(pfd[0].revents && !scomm->thread_running)
        {
            ALOGE("receive exit signal and stop thread ");
            break;
        }
        if(pfd[1].revents & POLLIN)
        {
            scomm->is_busying = FALSE;
            bytes_read = read(scomm->fd, read_buffer + last_len, 2056 - last_len);
            scomm->is_busying = TRUE;

            if(bytes_read == 0)
            {
                if(scomm->thread_running)
                {
                    continue;
                }
                break;
            }
            if(bytes_read < 0)
            {
                ALOGE("%s, read fail, thread[%d] state:%d, error code:%zd, %s", __func__, port_index, (scomm->thread_running), bytes_read, strerror(errno));
                if((scomm->thread_running) && (0 == port_index))
                {
                    scomm_vendor_send_hw_error();
                }
                break;
            }
            hex2String(read_buffer + last_len, str_buffer, (bytes_read > 64) ? 64 : bytes_read);
            SKWBT_LOG("scomm[%d] read:%zd, last_len:%d, %s", port_index, bytes_read, last_len, str_buffer);

            //data parse for get a commplete packet and capture the snoop log
            rev_len = bytes_read + last_len;
            do
            {
                uint8_t pkt_type = read_buffer[0];
                if((pkt_type == HCI_EVENT_PKT) || (pkt_type == HCI_ACLDATA_PKT) || (pkt_type == HCI_SCODATA_PKT) || (pkt_type == HCI_EVENT_SKWLOG) || (pkt_type == HCI_ISO_PKT))
                {
                    uint16_t hdr_lens = hci_preamble_sizes[pkt_type] + 1;
                    if(rev_len >= hdr_lens)
                    {
                        uint16_t pkt_len = 0;
                        //get payload length
                        if((pkt_type == HCI_ACLDATA_PKT) || (pkt_type == HCI_ISO_PKT))
                        {
                            pkt_len = *(uint16_t *)&read_buffer[HCI_COMMON_DATA_LENGTH_INDEX];
                        }
                        else if(pkt_type == HCI_EVENT_PKT)
                        {
                            pkt_len = read_buffer[HCI_EVENT_DATA_LENGTH_INDEX];
                        }
                        else if(pkt_type == HCI_EVENT_SKWLOG)
                        {
                            pkt_len = *(uint16_t *)&read_buffer[HCI_SKWLOG_DATA_LENGTH_INDEX];
                        }
                        else//cmd/sco
                        {
                            pkt_len = read_buffer[HCI_COMMON_DATA_LENGTH_INDEX];
                        }

                        SKWBT_LOG("rev_len:%d, pkt_type:%d, hdr_lens:%d, pkt_len:%d", rev_len, pkt_type, hdr_lens, pkt_len);

                        pkt_len += hdr_lens;
                        if(rev_len >= pkt_len)
                        {
                            if(pkt_type == HCI_EVENT_SKWLOG)
                            {
                                skwlog_write(read_buffer, pkt_len);
                            }
                            else//
                            {

                                skw_btsnoop_capture(read_buffer, TRUE);
                                scomm_vendor_send_to_host(0, read_buffer, pkt_len);
                            }
                            last_len = rev_len - pkt_len;
                            memcpy(read_buffer, read_buffer + pkt_len, last_len);
                            if(last_len >= 4)
                            {
                                rev_len = last_len;
                                SKWBT_LOG(" more packet, rev_len:%d ", rev_len);
                                continue;
                            }

                        }
                        else
                        {
                            last_len = rev_len;
                            SKWBT_LOG("need more, rev_len:%d, pkt_len:%d, last_len:%d", rev_len, pkt_len, last_len);
                        }
                    }
                    else
                    {
                        last_len = rev_len;
                    }
                }
                else//invalid data, discard
                {
                    int vLen = scomm_vendor_find_valid_type(read_buffer, rev_len);
                    ALOGE("invalid type:%02X, vLen:%d", pkt_type, vLen);

                    last_len = rev_len - vLen;
                    if(vLen < rev_len)
                    {
                        memcpy(read_buffer, read_buffer + vLen, last_len);
                    }
                }

                break;
            } while (1);
            continue;
        }

        if (pfd[1].revents & (POLLERR | POLLHUP))
        {
            ALOGE("%s poll error, fd : %d", __func__, scomm->fd);
            scomm->driver_state = FALSE;
            close(scomm->fd);
            break;
        }
    }


    scomm->is_busying = FALSE;
    scomm->thread_uart_id = -1;

    ALOGD("%s [%d] exit", __func__, port_index);
    return NULL;
}


/*******************************************************************************
**
** Function        scomm_vendor_socket_open
**
** Description     USB/SDIO Open
**
** Returns         None
**
*******************************************************************************/
int scomm_vendor_socket_open(uint8_t port_index)
{
    int ret = 0;
    struct epoll_event event;
    if((ret = socketpair(AF_UNIX, SOCK_STREAM, 0, scomm_vnd[port_index].uart_fd)) < 0)
    {
        ALOGE("%s, errno : %s", __func__, strerror(errno));
        return ret;
    }

    if((ret = socketpair(AF_UNIX, SOCK_STREAM, 0, scomm_vnd[port_index].signal_fd)) < 0)
    {
        ALOGE("%s, errno : %s", __func__, strerror(errno));
        return ret;
    }

    scomm_vnd[port_index].epoll_fd = epoll_create(64);
    if (scomm_vnd[port_index].epoll_fd == -1)
    {
        ALOGE("%s unable to create epoll instance: %s", __func__, strerror(errno));
        return -1;
    }

    scomm_vnd[port_index].thread_running = TRUE;
    scomm_vnd[port_index].recv_comm_thread_running = FALSE;

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

    if(port_index == 0)
    {
        skw_socket_object.fd = scomm_vnd[port_index].uart_fd[1];
        skw_socket_object.read_ready = scomm_vendor_recv_rawdata;
        skw_socket_object.write_ready = NULL;

        memset(&event, 0, sizeof(event));
        event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
        event.data.ptr = (void *)&skw_socket_object;
        if (epoll_ctl(scomm_vnd[port_index].epoll_fd, EPOLL_CTL_ADD, scomm_vnd[port_index].uart_fd[1], &event) == -1)
        {
            ALOGE("%s unable to register fd %d to epoll set: %s", __func__, scomm_vnd[port_index].uart_fd[1], strerror(errno));
            close(scomm_vnd[port_index].epoll_fd);
            scomm_vnd[port_index].epoll_fd = -1;
            scomm_vnd[port_index].thread_running = FALSE;
            return -1;
        }

        event.data.ptr = NULL;
        if (epoll_ctl(scomm_vnd[port_index].epoll_fd, EPOLL_CTL_ADD, scomm_vnd[port_index].signal_fd[1], &event) == -1)
        {
            ALOGE("%s unable to register signal fd %d to epoll set: %s", __func__, scomm_vnd[port_index].signal_fd[1], strerror(errno));
            close(scomm_vnd[port_index].epoll_fd);
            scomm_vnd[port_index].epoll_fd = -1;
            scomm_vnd[port_index].thread_running = FALSE;
            return -1;
        }

        if (pthread_create(&scomm_vnd[port_index].thread_socket_id, &thread_attr, scomm_vendor_recv_socket_thread, (void *)(long)port_index) != 0 )
        {
            ALOGE("pthread_create : %s", strerror(errno));
            close(scomm_vnd[port_index].epoll_fd);
            scomm_vnd[port_index].epoll_fd = -1;
            scomm_vnd[port_index].thread_socket_id = -1;
            scomm_vnd[port_index].thread_running = FALSE;
            return -1;
        }
    }

    if (pthread_create(&scomm_vnd[port_index].thread_uart_id, &thread_attr, scomm_vendor_recv_scomm_thread, (void *)(long)port_index) != 0 )
    {
        ALOGE("pthread_create : %s", strerror(errno));
        close(scomm_vnd[port_index].epoll_fd);
        scomm_vnd[port_index].thread_running = FALSE;
        pthread_join(scomm_vnd[port_index].thread_socket_id, NULL);
        scomm_vnd[port_index].thread_socket_id = -1;
        return -1;
    }
    while(!scomm_vnd[port_index].recv_comm_thread_running)
    {
        usleep(20);
    }

    scomm_vnd[port_index].driver_state = TRUE;

    ret = scomm_vnd[port_index].uart_fd[0];

    ALOGD("%s uart_fd:%d", __func__, ret);
    return ret;
}


/*******************************************************************************
**
** Function        scomm_vendor_socket_close
**
** Description     socket close
**
** Returns         None
**
*******************************************************************************/
static void scomm_vendor_socket_close(uint8_t port_index)
{
    int result;

    ALOGE( "%s [%d], thread_socket_id:0x%X", __func__, port_index, (int)scomm_vnd[port_index].thread_socket_id);

    if ((scomm_vnd[port_index].uart_fd[0] > 0) && (result = close(scomm_vnd[port_index].uart_fd[0])) < 0)
    {
        ALOGE( "%s (fd:%d) FAILED result:%d", __func__, scomm_vnd[port_index].uart_fd[0], result);
    }

    if (epoll_ctl(scomm_vnd[port_index].epoll_fd, EPOLL_CTL_DEL, scomm_vnd[port_index].uart_fd[1], NULL) == -1)
    {
        ALOGE("%s unable to unregister fd %d from epoll set: %s", __func__, scomm_vnd[port_index].uart_fd[1], strerror(errno));
    }

    if (epoll_ctl(scomm_vnd[port_index].epoll_fd, EPOLL_CTL_DEL, scomm_vnd[port_index].signal_fd[1], NULL) == -1)
    {
        ALOGE("%s unable to unregister signal fd %d from epoll set: %s", __func__, scomm_vnd[port_index].signal_fd[1], strerror(errno));
    }

    if ((scomm_vnd[port_index].uart_fd[1] > 0) && (result = close(scomm_vnd[port_index].uart_fd[1])) < 0)
    {
        ALOGE( "%s (fd:%d) FAILED result:%d", __func__, scomm_vnd[port_index].uart_fd[1], result);
    }

    if(scomm_vnd[port_index].thread_socket_id != -1)
    {
        if ((result = pthread_join(scomm_vnd[port_index].thread_socket_id, NULL)) < 0)
        {
            ALOGE( "data thread pthread_join()  scomm_vnd[port_index].thread_socket_id failed result:%d", result);
        }
        else
        {
            scomm_vnd[port_index].thread_socket_id = -1;
            ALOGE( "data thread pthread_join() scomm_vnd[port_index].thread_socket_id pthread_join_success result:%d", result);
        }
    }
    if(scomm_vnd[port_index].epoll_fd > 0)
    {
        close(scomm_vnd[port_index].epoll_fd);
    }

    if ((scomm_vnd[port_index].signal_fd[0] > 0) && (result = close(scomm_vnd[port_index].signal_fd[0])) < 0)
    {
        ALOGE( "%s (signal fd[0]:%d) FAILED result:%d", __func__, scomm_vnd[port_index].signal_fd[0], result);
    }
    if ((scomm_vnd[port_index].signal_fd[1] > 0) && (result = close(scomm_vnd[port_index].signal_fd[1])) < 0)
    {
        ALOGE( "%s (signal fd[1]:%d) FAILED result:%d", __func__, scomm_vnd[port_index].signal_fd[1], result);
    }

    scomm_vnd[port_index].epoll_fd = -1;
    scomm_vnd[port_index].uart_fd[0] = -1;
    scomm_vnd[port_index].uart_fd[1] = -1;
    scomm_vnd[port_index].signal_fd[0] = -1;
    scomm_vnd[port_index].signal_fd[1] = -1;
    ALOGE( "%s [%d] end", __func__, port_index);
}


void scomm_vendor_write_bt_state()
{
    //if(skwbt_transtype & SKWBT_TRANS_TYPE_USB)
    if(chip_version == SKW_CHIPID_6160)
    {
        char buffer[10] = {0x01, 0x80, 0xFE, 0x01, 0x00};
        scomm_vnd_st *scomm = &scomm_vnd[0];
        write(scomm->fd, buffer, 5);
        usleep(15000);
    }
}

/*******************************************************************************
**
** Function        scomm_vendor_port_close
**
** Description     Conduct vendor-specific close works
**
** Returns         None
**
*******************************************************************************/
void scomm_vendor_port_close(uint8_t port_index)
{
    //send close signal
    unsigned char close_signal = 1;
    ssize_t ret;
    int res;
    scomm_vnd_st *scomm = &scomm_vnd[port_index];
    ALOGD( "%s [%d] start, fd:%d, busy:%d", __func__, port_index, scomm->fd, scomm->is_busying);

    if(scomm->fd == -1)
    {
        return;
    }

    scomm->thread_running = FALSE;
    scomm->driver_state = FALSE;

    while(scomm->is_busying)
    {
        usleep(20);
    }

    res = ioctl(scomm->fd, 0);
    //ALOGE("res:%d, %s", res, strerror(errno));

    ALOGE("%s signal_fd:%d", __func__, scomm->signal_fd[1]);
    RW_NO_INTR(ret = write(scomm->signal_fd[1], &close_signal, 1));

    usleep(300);//wait
    for(int i = 0; (skwbt_transtype & SKWBT_TRANS_TYPE_SDIO) && (i < 2) && (scomm->thread_uart_id != -1); i++)
    {
        res = ioctl(scomm->fd, 0);//try again
        usleep(200);
	ALOGD("%s,%d times:%d, res:%d, %s", __func__, port_index, i, res, strerror(errno));
    }

    //scomm close
    if ((scomm->fd > 0) && (res = close(scomm->fd)) < 0)
    {
        ALOGE( "%s (fd:%d) FAILED result:%d", __func__, scomm->fd, res);
    }

    ALOGD("Run Here");

    if(scomm->thread_uart_id != -1)
    {
        pthread_join(scomm->thread_uart_id, NULL);
    }

    scomm_vendor_socket_close(port_index);

    //close(scomm_vnd[port_index].fd);
    scomm->fd = -1;


    ALOGD( "%s [%d] finish", __func__, port_index);

}

/*******************************************************************************
**
** Function        scomm_vendor_close
**
** Description     Conduct vendor-specific close works
**
** Returns         None
**
*******************************************************************************/
void scomm_vendor_close()
{
    int idx = 0;
    for(idx = 0; idx < BT_COM_PORT_SIZE; idx++)
    {
        scomm_vendor_port_close(idx);
    }
    if(btpw_fp > 0)
    {
        close(btpw_fp);
    }
}

/*******************************************************************************
**
** Function         scomm_vendor_init_err
**
** Description      init err
**
** Returns          None
**
*******************************************************************************/
void scomm_vendor_init_err(HC_BT_HDR   *p_buf)
{
    hw_cfg_cb.state = HW_CFG_INIT;
    bt_vendor_cbacks->dealloc(p_buf);
    fclose(hw_cfg_cb.nv_fp);
    hw_cfg_cb.nv_fp = NULL;

}

/*******************************************************************************
**
** Function         scomm_vendor_config_callback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void scomm_vendor_config_callback(void *p_mem)
{
    uint8_t     status = 0;
    uint16_t    opcode = 0;
    HC_BT_HDR   *p_buf = NULL;
    HC_BT_HDR   *p_evt_buf = NULL;


    if(p_mem != NULL)
    {
        p_evt_buf = (HC_BT_HDR *) p_mem;
        status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET);
        uint8_t *p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE_OFFSET;
        STREAM_TO_UINT16(opcode, p);
    }


    ALOGD("%s status:%d ,opcode:%04X", __func__, status, opcode);
    if((status == 0) && bt_vendor_cbacks)
    {
        p_buf = (HC_BT_HDR *)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_MAX_LEN);
    }

    if(p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->len = 0;
        p_buf->layer_specific = 0;

        ALOGD("hw_cfg_cb.state = %d", hw_cfg_cb.state);

        switch (hw_cfg_cb.state)
        {
            case HW_CFG_START:
            {
                uint8_t *ptr = (uint8_t *) (p_buf + 1);
                UINT16_TO_STREAM(ptr, HCI_READ_LOCAL_VERSION_INFO);
                UINT8_TO_STREAM(ptr, 0);

                p_buf->len = 3;//packet len
                bt_vendor_cbacks->xmit_cb(HCI_READ_LOCAL_VERSION_INFO, p_buf, scomm_vendor_config_callback);
                hw_cfg_cb.state = HW_CFG_READ_HCI_VERSION;
                break;
            }
            case HW_CFG_READ_HCI_VERSION:
            {
                char file_name[128] = {0};
                uint8_t skip_header = 0;
                uint8_t *p = (uint8_t *)(p_evt_buf + 1) + 7;
                STREAM_TO_UINT16(chip_version, p);

                ALOGD("chip_version:0x%04X", chip_version);

                switch(chip_version)
                {
                    case SKW_CHIPID_6316://0x6316
                    {
                        skip_header = 1;
                        sprintf(file_name, "%s/sv6316.nvbin", SKWBT_NV_FILE_PATH);
                        break;
                    }
                    default:
                    {
                        sprintf(file_name, "%s/sv6160.nvbin", SKWBT_NV_FILE_PATH);
                        chip_version = SKW_CHIPID_6160;
                        break;
                    }
                }

                hw_cfg_cb.nv_fp = fopen(file_name, "rb");
                if(!hw_cfg_cb.nv_fp)
                {
                    ALOGE("%s unable to open nv file:%s: %s", __func__, SKWBT_NV_FILE_PATH, strerror(errno));
                    bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
                    hw_cfg_cb.state = HW_CFG_INIT;
                    return;
                }
                hw_cfg_cb.file_offset = 0;
                hw_cfg_cb.state = HW_CFG_NV_SEND;
                if(skip_header)//skip header
                {
                    char buffer[6];
                    fread(buffer, 1, 4, hw_cfg_cb.nv_fp);
                }
            }
            case HW_CFG_NV_SEND:
            {
                uint8_t len = 0, res = 0;
                uint8_t *ptr = (uint8_t *) (p_buf + 1);//skip header
                UINT16_TO_STREAM(ptr, HCI_CMD_SKW_BT_NVDS);

                if(chip_version == SKW_CHIPID_6316)//0x6316
                {
                    uint8_t tmp_buffer[10] = {0};
                    uint8_t *param_buf = ptr + 3;
                    uint8_t nv_tag = 0;
                    int  nv_param_len = 0;
                    int  total_len = 0;
                    char file_end = 0;
                    int  file_ptr;
                    while(1)
                    {
                        file_ptr = ftell(hw_cfg_cb.nv_fp);
                        len = fread(tmp_buffer, 1, 3, hw_cfg_cb.nv_fp);
                        if((len < 3) || feof(hw_cfg_cb.nv_fp))
                        {
                            file_end = 1;
                            break;
                        }
                        memcpy(param_buf + total_len, tmp_buffer, 3);
                        nv_param_len = tmp_buffer[2];
                        nv_tag = tmp_buffer[0];
                        ALOGD("tag:%d, nv_param_len:%d, file_ptr:%d", tmp_buffer[0], nv_param_len, file_ptr);

                        if((nv_param_len + total_len + 3) > 252)//252 + 3
                        {
                            fseek(hw_cfg_cb.nv_fp, file_ptr, SEEK_SET);
                            break;
                        }
                        total_len += 3;
                        if(nv_param_len > 0)
                        {
                            len = fread(param_buf + total_len, 1, nv_param_len, hw_cfg_cb.nv_fp);
                            if(len < nv_param_len)
                            {
                                ALOGE("%s, len:%d, nv_param_len:%d", __func__, len, nv_param_len);
                                scomm_vendor_init_err(p_buf);
                                break;
                            }
                            if(nv_tag == NV_TAG_BD_ADDR)
                            {
                                skw_addr_get(param_buf + total_len + 3);
                            }
                            total_len += len;
                        }
                    }
                    if(total_len > 0)
                    {
                        ptr[0] = (total_len + 2);//payload len
                        ptr[1] = hw_cfg_cb.file_offset;
                        ptr[2] = total_len;//para len
                        p_buf->len = total_len + 2 + 3;//packet len
                        res = bt_vendor_cbacks->xmit_cb(HCI_CMD_SKW_BT_NVDS, p_buf, scomm_vendor_config_callback);
                        hw_cfg_cb.file_offset ++;
                        if(res == FALSE)//send error
                        {
                            scomm_vendor_init_err(p_buf);
                            break;
                        }
                    }
                    if(file_end)
                    {
                        hw_cfg_cb.state = HW_CFG_WRITE_BD_ADDR;
                        fclose(hw_cfg_cb.nv_fp);
                        hw_cfg_cb.nv_fp = NULL;
                    }
                }
                else
                {
                    len = fread(ptr + 3, 1, NV_FILE_RD_BLOCK_SIZE, hw_cfg_cb.nv_fp);
                    ptr[0] = (len + 2);//payload len
                    ptr[1] = hw_cfg_cb.file_offset;
                    ptr[2] = len;//para len
                    p_buf->len = len + 2 + 3;//packet len
                    if(0 == hw_cfg_cb.file_offset)
                    {
                        //*(ptr + 3 + 3) = 'A';
                        //byte7
                        skw_addr_get(ptr + 3 + 7);
                    }
                    else if(1 == hw_cfg_cb.file_offset)
                    {
                        *(ptr + 3 + 62) |= 0x80;
                    }

                    res = bt_vendor_cbacks->xmit_cb(HCI_CMD_SKW_BT_NVDS, p_buf, scomm_vendor_config_callback);

                    ALOGD("len:%d, file_offset:%d, plen:%d,%d res:%d", len, hw_cfg_cb.file_offset, p_buf->len, ptr[0], res);

                    hw_cfg_cb.file_offset ++;

                    if((len < NV_FILE_RD_BLOCK_SIZE) || (len == 0) || feof(hw_cfg_cb.nv_fp))//end of file
                    {
                        hw_cfg_cb.state = HW_CFG_WRITE_BD_ADDR;
                        fclose(hw_cfg_cb.nv_fp);
                        hw_cfg_cb.nv_fp = NULL;
                    }
                    if(res == FALSE)//send error
                    {
                        scomm_vendor_init_err(p_buf);
                    }

                }
                break;
            }
            case HW_CFG_WRITE_BD_ADDR:
            {
                uint8_t *ptr = (uint8_t *) (p_buf + 1);
                UINT16_TO_STREAM(ptr, HCI_CMD_WRITE_BD_ADDR);
                UINT8_TO_STREAM(ptr, 6);

                p_buf->len = 3 + 6;//packet len
                if(skw_addr_from_ap(ptr))
                {
                    bt_vendor_cbacks->xmit_cb(HCI_CMD_WRITE_BD_ADDR, p_buf, scomm_vendor_config_callback);
                    hw_cfg_cb.state = HW_CFG_NV_SEND_CMPL;
                    break;
                }
            }
            case HW_CFG_NV_SEND_CMPL:
            {
                bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
                bt_vendor_cbacks->dealloc(p_buf);//free buffer
                hw_cfg_cb.state = HW_CFG_INIT;
                break;
            }
        }
    }


    /* Free the RX event buffer */
    if ((bt_vendor_cbacks) && (p_evt_buf != NULL))
    {
        bt_vendor_cbacks->dealloc(p_evt_buf);
    }


}

/*******************************************************************************
**
** Function        scomm_vendor_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void scomm_vendor_config_start()
{
    memset(&hw_cfg_cb, 0, sizeof(bt_hw_cfg_cb_st));

    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    hw_cfg_cb.state = HW_CFG_INIT;

    if (bt_vendor_cbacks)
    {
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE);
        if (p_buf)
        {
            p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE;

            p = (uint8_t *) (p_buf + 1);
            UINT16_TO_STREAM(p, HCI_RESET);
            *p = 0; /* parameter length */

            hw_cfg_cb.state = HW_CFG_START;

            bt_vendor_cbacks->xmit_cb(HCI_RESET, p_buf, scomm_vendor_config_callback);
        }
        else
        {
            ALOGE("%s buffer alloc fail", __func__);
        }
    }
    else
    {
        ALOGE("%s call back func is null", __func__);
    }
}



