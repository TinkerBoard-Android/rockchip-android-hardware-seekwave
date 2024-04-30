/******************************************************************************
 *
 *  Copyright (C) 2020-2021 SeekWave Technology
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <utils/Log.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MACDEF_FOR_INCAR 1

#define BD_ADDR_LEN 6

unsigned char bdaddr_lap[3] = {0x12, 0x24, 0x56};
char bdaddr_valid = 1;
extern char skw_btsnoop_path[];

extern void hex2String(unsigned char hex[], unsigned char str[],int N);

void skw_addr_gen_init()
{
    char basepath[PATH_MAX] = {0};
    char filepath[PATH_MAX] = {0};
    int size = strlen(skw_btsnoop_path);
    for(int i = size - 1; i > 0; i--)//get log dir
    {
        char ch = skw_btsnoop_path[i];
        if((ch == '/') || (ch == '\\'))
        {
            memcpy(basepath, skw_btsnoop_path, i);
            //ALOGD("cp:%d", i);
            break;
        }
    }
    snprintf(filepath, PATH_MAX, "%s/skwbdaddr.txt", basepath);

    ALOGD("%s, %s", __func__, filepath);

    FILE *fp = fopen(filepath, "r"); //read only
    if(fp == NULL)//file not exist
    {
        fp = fopen(filepath, "w"); //file not exist, create it
        srand(time(NULL));
        if(fp)
        {
            bdaddr_lap[0] = (unsigned char)(rand() & 0xFF);
            bdaddr_lap[1] = (unsigned char)(rand() & 0xFF);
            bdaddr_lap[2] = (unsigned char)(rand() & 0xFF);
            fwrite(bdaddr_lap, 3, 1, fp);
            fflush(fp);
            fclose(fp);
        }
        else
        {
            bdaddr_valid = 0;
        }
        //else use default addr
    }
    else
    {
        fread(bdaddr_lap, 3, 1, fp);
        fclose(fp);
    }

    ALOGD("%s, addr valid:%d, %02X %02X %02X", __func__, bdaddr_valid, bdaddr_lap[0], bdaddr_lap[1], bdaddr_lap[2]);
}


#if MACDEF_FOR_INCAR

#define VENDOR_REQ_TAG      0x56524551
#define VENDOR_READ_IO      _IOW('v', 0x01, unsigned int)
#define VENDOR_WRITE_IO     _IOW('v', 0x02, unsigned int)

#define VENDOR_SN_ID        1
#define VENDOR_WIFI_MAC_ID  2
#define VENDOR_LAN_MAC_ID   3
#define VENDOR_BLUETOOTH_ID 4

struct rk_vendor_req
{
    uint32_t tag;
    uint16_t id;
    uint16_t len;
    uint8_t data[1];
};

int m_get_mac_address(uint8_t *local_addr)
{
    int ret ;
    uint8_t p_buf[64];
    struct rk_vendor_req *req;

    req = (struct rk_vendor_req *)p_buf;
    int sys_fd = open("/dev/vendor_storage", O_RDWR, 0);
    if(sys_fd < 0)
    {
        ALOGE("vendor_storage open fail\n");
        return -1;
    }

    req->tag = VENDOR_REQ_TAG;
    req->id = VENDOR_BLUETOOTH_ID;

    req->len = 6;
    ret = ioctl(sys_fd, VENDOR_READ_IO, req);
    if (!ret)
    {
        //uint8_t str_buffer[32] = {0};
        int i = 0, j = BD_ADDR_LEN - 1;
        for(; i < BD_ADDR_LEN; i++, j--)
        {
        	local_addr[i] = req->data[j];
        }
        //memcpy(local_addr, req->data, BD_ADDR_LEN);
        //hex2String(local_addr, str_buffer, BD_ADDR_LEN);
        //ALOGE("bt addr get ok:%s", str_buffer);
    }

    close(sys_fd);
    return 0;
}


#endif

char skw_addr_check_valid(unsigned char *bd_addr)
{
	uint32_t addr_check = 0;
	for(int i = 0; i < BD_ADDR_LEN; i++)
	{
		addr_check += bd_addr[i];
	}
	return (addr_check > 0);
}

/*
Get bd addr from AP
if address exist, return 1, else 0
*/
char skw_addr_from_ap(unsigned char *bd_addr)
{
	memset(bd_addr, 0, BD_ADDR_LEN);
#if MACDEF_FOR_INCAR
	if(m_get_mac_address(bd_addr) == 0)
	{
		return skw_addr_check_valid(bd_addr);
	}
#endif
	return 0;
}

void skw_addr_get(unsigned char *buffer)
{
    if(bdaddr_valid > 0)
    {
        buffer[0] = bdaddr_lap[0];
        buffer[1] = bdaddr_lap[1];
        buffer[2] = bdaddr_lap[2];
    }
}

