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
#ifndef __WIFI_COMMAND_H__
#define __WIFI_COMMAND_H__

#include "main.h"
#include "nl80211_copy.h"

#define SKW_VCMD_GET_CHANNELS                   0x1009
#define SKW_VCMD_SET_COUNTRY                    0x100E
#define SKW_VCMD_GET_VERSION                    0x1403
#define SKW_VCMD_GET_RING_BUFFERS_STATUS        0x1404
#define SKW_VCMD_GET_LOGGER_FEATURES            0x1406

#define SKW_VCMD_GET_APF_CAPABILITIES           0x1800
#define SKW_VCMD_GET_USABLE_CHANS               0x2000

enum SKW_SUBCMD_GET_VERSION {
	SKW_DRV_VERSION = 1,
	SKW_FW_VERSION,
};

class WifiCommand {
private:
	struct nl_msg *msg;
	struct nl_sock *sock;
	int id;

public:
	WifiCommand(struct nl_sock *sk, int family_id, int flags, int nl80211_cmd);
	wifi_error send();

	virtual ~WifiCommand();
	virtual wifi_error build(wifi_interface_handle handle, void *param) = 0;
	// virtual wifi_error parser(struct nl_msg *msg) = 0;
	virtual wifi_error parser(struct nlattr *attr[]) = 0;
	// virtual wifi_error parser(struct nlattr *tb[CTRL_ATTR_MAX])

	struct nl_msg *nlmsg()
	{
		return msg;
	}

	int put_s8(int attribute, int8_t value)
	{
		return nla_put(nlmsg(), attribute, sizeof(int8_t), &value);
	}

	int put_u8(int attribute, uint8_t value)
	{
		return nla_put(nlmsg(), attribute, sizeof(uint8_t), &value);
	}

	int put_s16(int attribute, int16_t value)
	{
		return nla_put(nlmsg(), attribute, sizeof(int16_t), &value);
	}

	int put_u16(int attribute, uint16_t value)
	{
		return nla_put(nlmsg(), attribute, sizeof(uint16_t), &value);
	}

	int put_s32(int attribute, int32_t value)
	{
		return nla_put(nlmsg(), attribute, sizeof(int32_t), &value);
	}

	int put_u32(int attribute, uint32_t value)
	{
		return nla_put(nlmsg(), attribute, sizeof(uint32_t), &value);
	}

	int put_s64(int attribute, int64_t value)
	{
		return nla_put(nlmsg(), attribute, sizeof(int64_t), &value);
	}

	int put_u64(int attribute, uint64_t value)
	{
		return nla_put(nlmsg(), attribute, sizeof(uint64_t), &value);
	}

	int put_data(int attribute, int size, void *data)
	{
		return nla_put(nlmsg(), attribute, size, data);
	}

	int put_string(int attribute, const char *value)
	{
		return nla_put(nlmsg(), attribute, strlen(value) + 1, value);
	}

	int put_addr(int attribute, mac_addr value)
	{
		return nla_put(nlmsg(), attribute, sizeof(mac_addr), value);
	}

	struct nlattr *attr_start()
	{
		return nla_nest_start(nlmsg(), NL80211_ATTR_VENDOR_DATA);
	}

	void attr_end(struct nlattr *attribute)
	{
		nla_nest_end(nlmsg(), attribute);
	}
};

#endif
