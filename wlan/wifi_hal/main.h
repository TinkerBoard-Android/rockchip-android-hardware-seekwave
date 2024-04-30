/**********************************************************************************
 *
 * Copyright (C) 2017 The Android Open Source Project
 * Copyright (C) 2020 SeekWave Technology Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **********************************************************************************/
#ifndef __MAIN_H__
#define __MAIN_H__

#define LOG_TAG  "SKW_WiFiHAL"
#include <log/log.h>

#include "wifi_hal.h"

#define OUI_GOOGLE               0x001A11
#define SKW_NR_IFACE             8

#define SKW_INVALID              -1
#define SKW_BIT(nr)              (1 << (nr))

#define skw_nla_for_each_nested(pos, nla, rem)                   \
	for (pos = (nlattr *)nla_data(nla), rem = nla_len(nla);  \
	     nla_ok(pos, rem);                                   \
	     pos = (nlattr *)nla_next(pos, &(rem)))

enum SKW_CHANNEL_WIDTH {
	SKW_CHAN_WIDTH_20,
	SKW_CHAN_WIDTH_40,
	SKW_CHAN_WIDTH_80,
	SKW_CHAN_WIDTH_80P80,
	SKW_CHAN_WIDTH_160,

	SKW_CHAN_WIDTH_MAX,
};

enum SKW_IFACE_MODE {
	SKW_MODE_NONE,
	SKW_MODE_STA,
	SKW_MODE_AP,
	SKW_MODE_GC,
	SKW_MODE_GO,
	SKW_MODE_P2P_DEV,
	SKW_MODE_IBSS,

	SKW_MODE_LAST,
};

enum SKW_BAND {
	SKW_BAND_2GHZ,
	SKW_BAND_5GHZ,
	SKW_BAND_6GHZ,
	SKW_BAND_60GHZ,

	SKW_BAND_INVALD,
};

struct skw_usable_chan_info {
	u32 band_mask;
	u32 filter_mask;
	u32 iface_mode_mask;
	u32 flags;
	u32 resvd;
};

struct skw_usable_chan {
	u16 center_freq;
	u16 band_width;
	u16 iface_mode_mask;
	u16 flags;
	u32 resvd;
};

typedef struct {
	int  iface_idx;                                // id to use when talking to driver
	int  wdev_idx;                                 // id to use when talking to driver
	wifi_handle hal_handle;                        // handle to wifi data
	char name[IFNAMSIZ+1];                         // interface name + trailing null
} interface_info;

typedef struct {
	struct nl_sock *nl_hal;                        // command socket object
	struct nl_sock *nl_event;                      // event socket object
	int family_nl80211;                            // family id for 80211 driver

	bool exit;                                     // Indication to exit since cleanup has started
	int exit_socks[2];                             // sockets used to implement wifi_cleanup
	wifi_cleaned_up_handler cleaned_up_handler;

	bool in_event_loop;                             // Indicates that event loop is active

	pthread_mutex_t cb_lock;                        // mutex for the event_cb access

	int num_cmd;                                    // number of commands
	int alloc_cmd;                                  // number of commands allocated

	int nr_interfaces;
	interface_info interfaces[SKW_NR_IFACE];
	wifi_interface_handle interface_handle[SKW_NR_IFACE];

	int max_num_interfaces;                         // max number of interfaces

	// add other details
} hal_info;

static inline hal_info *getHalInfo(wifi_interface_handle handle)
{
    return (hal_info *)(((interface_info *)handle)->hal_handle);
}
#endif
