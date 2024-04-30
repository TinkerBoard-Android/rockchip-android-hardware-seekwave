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
#include <errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <sys/types.h>
#include <unistd.h>

#include "main.h"
#include "wifi_command.h"

#define SKW_BUFF_SIZE                    256
#define WIFI_HAL_SOCK_DEFAULT_PORT       644
#define WIFI_HAL_SOCK_EVENT_PORT         645
#define SOCK_BUFF_SIZE                   0x40000

struct nl_sock *getSock(wifi_interface_handle handle)
{
	interface_info *info = (interface_info *)handle;

	return ((hal_info *)info->hal_handle)->nl_hal;
}

int getFamily(wifi_interface_handle handle)
{
	interface_info *info = (interface_info *)handle;

	return ((hal_info *)info->hal_handle)->family_nl80211;
}

void skw_wifi_get_error_info(wifi_error error, const char **chr)
{
	ALOGD("%s", __func__);
}

wifi_error skw_wifi_get_supported_feature_set(wifi_interface_handle handle, feature_set *feature)
{

	ALOGD("%s, feature: 0x%x", __func__, *feature);

	*feature = 0;

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_concurrency_matrix(wifi_interface_handle handle, int count,
					feature_set *feature, int *args)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_scanning_mac_oui(wifi_interface_handle handle, unsigned char *oui)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_supported_channels(wifi_handle handle, int *num, wifi_channel *channels)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_is_epr_supported(wifi_handle handle)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

class GetInterfacesCommand : public WifiCommand {
private:
	int index;
	interface_info ifaces[SKW_NR_IFACE];

public:
	GetInterfacesCommand(struct nl_sock *sk, int family, int flags, int nl80211_cmd)
		: WifiCommand(sk, family, flags, nl80211_cmd)
	{
		index = 0;
		memset(ifaces, 0x0, sizeof(interface_info));
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		return WIFI_SUCCESS;
	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		if (attr[NL80211_ATTR_IFINDEX])
			ifaces[index].iface_idx = nla_get_u32(attr[NL80211_ATTR_IFINDEX]);
		else
			ifaces[index].wdev_idx = nla_get_u32(attr[NL80211_ATTR_WDEV]);

		if (attr[NL80211_ATTR_IFNAME])
			strcpy(ifaces[index].name, nla_get_string(attr[NL80211_ATTR_IFNAME]));

		index++;

		return WIFI_SUCCESS;
	}

	int getIfaceNum()
	{
		return index;
	}

	interface_info *iface(int idx)
	{
		return &ifaces[idx];
	}
};

wifi_error skw_wifi_get_ifaces(wifi_handle handle, int *num, wifi_interface_handle **iface_handle)
{
	int i;
	hal_info *hal = (hal_info *)handle;
	GetInterfacesCommand cmd(hal->nl_hal, hal->family_nl80211, NLM_F_DUMP, NL80211_CMD_GET_INTERFACE);

	memset(hal->interfaces, 0x0, sizeof(hal->interfaces));

	cmd.build(NULL, NULL);
	cmd.send();

	if (cmd.getIfaceNum() == 0)
		return WIFI_ERROR_UNKNOWN;

	hal->nr_interfaces = cmd.getIfaceNum();

	for (i = 0; i < hal->nr_interfaces; i++) {
		strcpy(hal->interfaces[i].name, cmd.iface(i)->name);
		hal->interfaces[i].iface_idx = cmd.iface(i)->iface_idx;
		hal->interfaces[i].wdev_idx = cmd.iface(i)->wdev_idx;
		hal->interfaces[i].hal_handle = handle;

		hal->interface_handle[i] = (wifi_interface_handle)(&hal->interfaces[i]);
	}

	*iface_handle = &hal->interface_handle[0];
	*num = hal->nr_interfaces;

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_iface_name(wifi_interface_handle handle, char *name, size_t size)
{
	interface_info *iface = (interface_info *)handle;

	ALOGD("%s: name: %s", __func__, iface->name);

	strncpy(name, iface->name, size);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_iface_event_handler(wifi_request_id,wifi_interface_handle ,
		wifi_event_handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_reset_iface_event_handler(wifi_request_id, wifi_interface_handle)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_start_gscan(wifi_request_id id, wifi_interface_handle iface,
		wifi_scan_cmd_params params, wifi_scan_result_handler handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_stop_gscan(wifi_request_id, wifi_interface_handle)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_cached_gscan_results(wifi_interface_handle, byte, int,
		wifi_cached_scan_results *results, int *)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_bssid_hotlist(wifi_request_id, wifi_interface_handle,
		wifi_bssid_hotlist_params, wifi_hotlist_ap_found_handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_reset_bssid_hotlist(wifi_request_id, wifi_interface_handle)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_significant_change_handler(wifi_request_id, wifi_interface_handle,
		wifi_significant_change_params, wifi_significant_change_handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_reset_significant_change_handler(wifi_request_id id, wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_gscan_capabilities(wifi_interface_handle iface, wifi_gscan_capabilities *capa)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_link_stats(wifi_interface_handle iface, wifi_link_layer_params params)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_link_stats(wifi_request_id id, wifi_interface_handle handle,
		wifi_stats_result_handler result_handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_clear_link_stats(wifi_interface_handle,u32, u32 *, u8, u8 *)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

class GetValidChannelsCommand : public WifiCommand {
private:
	wifi_channel *mChannels;
	int max_channels, nr_channels;

public:
	GetValidChannelsCommand(struct nl_sock *sk, int family, int flags, int nl80211_cmd,
			wifi_channel *channels, int max_chans) : WifiCommand(sk, family, flags, nl80211_cmd)
	{
		nr_channels = 0;
		mChannels = channels;
		max_channels = max_chans;
		memset(channels, 0x0, max_chans * sizeof(*channels));
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		struct nlattr *data;
		interface_info *iface = (interface_info *)handle;

#define SKW_ATTR_BAND                 20

		if (iface->wdev_idx)
			put_u32(NL80211_ATTR_WDEV, iface->wdev_idx);
		else
			put_u32(NL80211_ATTR_IFINDEX, iface->iface_idx);

		put_u32(NL80211_ATTR_VENDOR_ID, OUI_GOOGLE);
		put_u32(NL80211_ATTR_VENDOR_SUBCMD, SKW_VCMD_GET_CHANNELS);

		data = attr_start();

		put_u32(SKW_ATTR_BAND, *(int *)param);

		attr_end(data);

		return WIFI_SUCCESS;
	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		int left, type;
		struct nlattr *nla;
		struct nlattr *data = attr[NL80211_ATTR_VENDOR_DATA];

#define SKW_ATTR_NR_CAHNNELS           36
#define SKW_ATTR_VALID_CHANNELS        37

		if (!data)
			return WIFI_ERROR_NOT_AVAILABLE;

		nla_for_each_attr(nla, (struct nlattr *)nla_data(data), nla_len(data), left)
		{
			type = nla_type(nla);
			switch (type) {
			case SKW_ATTR_NR_CAHNNELS:
				nr_channels = nla_get_u32(nla);
				break;

			case SKW_ATTR_VALID_CHANNELS:
				memcpy(mChannels, nla_data(nla), nla_len(nla));
				break;

			default:
				break;
			}
		}

		return WIFI_SUCCESS;
	}

	int numChannels()
	{
		return nr_channels;
	}
};

wifi_error skw_wifi_get_valid_channels(wifi_interface_handle iface, int band,
			int max_channels, wifi_channel *channels, int *num_channels)
{
	int i;
	wifi_error err = WIFI_ERROR_UNKNOWN;

	GetValidChannelsCommand cmd(getSock(iface), getFamily(iface), 0, NL80211_CMD_VENDOR, channels, max_channels);

	cmd.build(iface, &band);
	err = cmd.send();

	*num_channels = cmd.numChannels();

	ALOGD("%s, BAND: %d, nr channels: %d", __func__, band, *num_channels);

	return err;
}

wifi_error skw_wifi_rtt_range_request(wifi_request_id, wifi_interface_handle, unsigned,
		wifi_rtt_config[], wifi_rtt_event_handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_rtt_range_cancel(wifi_request_id id,  wifi_interface_handle iface,
		unsigned range, mac_addr addr[])
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_rtt_capabilities(wifi_interface_handle handle, wifi_rtt_capabilities *capa)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_rtt_get_responder_info(wifi_interface_handle iface,
		wifi_rtt_responder *responder_info)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_enable_responder(wifi_request_id id, wifi_interface_handle iface,
		wifi_channel_info channel_hint, unsigned max_duration_seconds,
		wifi_rtt_responder *responder_info)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_disable_responder(wifi_request_id id, wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_nodfs_flag(wifi_interface_handle, u32)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_start_logging(wifi_interface_handle, u32, u32, u32, u32, char *)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_epno_list(wifi_request_id, wifi_interface_handle,
				const wifi_epno_params *, wifi_epno_handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_reset_epno_list(wifi_request_id, wifi_interface_handle)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

class SetCountryCodeCommand : public WifiCommand {
public:
	SetCountryCodeCommand(struct nl_sock *sk, int family, int flags, int nl80211_cmd)
		: WifiCommand(sk, family, flags, nl80211_cmd)
	{
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		struct nlattr *data;
		interface_info *iface = (interface_info *)handle;

		put_u32(NL80211_ATTR_VENDOR_ID, OUI_GOOGLE);
		put_u32(NL80211_ATTR_VENDOR_SUBCMD, SKW_VCMD_SET_COUNTRY);

		if (iface->wdev_idx)
			put_u32(NL80211_ATTR_WDEV, iface->wdev_idx);
		else
			put_u32(NL80211_ATTR_IFINDEX, iface->iface_idx);

		data = attr_start();

		put_string(5, (const char *)param);

		attr_end(data);

		return WIFI_SUCCESS;
	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		return WIFI_SUCCESS;
	}
};

static wifi_error skw_wifi_set_country_code(wifi_interface_handle handle, const char *country)
{
	SetCountryCodeCommand cmd(getSock(handle), getFamily(handle), 0, NL80211_CMD_VENDOR);

	ALOGD("%s, country: %s", __func__, country);

	cmd.build(handle, (void *)country);

	return cmd.send();
}

wifi_error skw_wifi_get_firmware_memory_dump( wifi_interface_handle iface,
		wifi_firmware_memory_dump_handler handler)
{
	ALOGD("%s", __func__);

	return WIFI_ERROR_NOT_SUPPORTED;
}

wifi_error skw_wifi_set_log_handler(wifi_request_id id, wifi_interface_handle iface,
		wifi_ring_buffer_data_handler handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_reset_log_handler(wifi_request_id id, wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_alert_handler(wifi_request_id id, wifi_interface_handle iface,
		wifi_alert_handler handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_reset_alert_handler(wifi_request_id id, wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

class GetVersionCommand : public WifiCommand {
private:
	char buff[SKW_BUFF_SIZE];

public:
	GetVersionCommand(struct nl_sock *sk, int family, int flags, int cmd)
		: WifiCommand(sk, family, flags, cmd)
	{
		memset(buff, 0x0, sizeof(buff));
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		struct nlattr *data;
		interface_info *info = (interface_info *)handle;

		put_u32(NL80211_ATTR_VENDOR_ID, OUI_GOOGLE);
		put_u32(NL80211_ATTR_VENDOR_SUBCMD, SKW_VCMD_GET_VERSION);

		if (info->wdev_idx)
			put_u32(NL80211_ATTR_WDEV, info->wdev_idx);
		else
			put_u32(NL80211_ATTR_IFINDEX, info->iface_idx);

		data = attr_start();

		put_u32(*(int *)param, 0);

		attr_end(data);

		return WIFI_SUCCESS;
	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		if (attr[NL80211_ATTR_VENDOR_DATA])
			strncpy(buff, (char *)nla_data(attr[NL80211_ATTR_VENDOR_DATA]), sizeof(buff));

		return WIFI_SUCCESS;
	}

	char *getVersion()
	{
		return buff;
	}
};


wifi_error skw_wifi_get_firmware_version(wifi_interface_handle iface,
					char *buffer, int buffer_size)
{
	int subcmd = SKW_FW_VERSION;
	wifi_error err = WIFI_ERROR_UNKNOWN;
	GetVersionCommand cmd(getSock(iface), getFamily(iface), 0, NL80211_CMD_VENDOR);

	cmd.build(iface, &subcmd);
	err = cmd.send();

	strncpy(buffer, cmd.getVersion(), buffer_size);

	ALOGD("%s: %s", __func__, buffer);

	return err;
}

class GetRingBuffStatus : public WifiCommand
{
private:
	u32 *num;
	wifi_ring_buffer_status *mStatus;

public:
	GetRingBuffStatus(struct nl_sock *sk, int family, int flags,
		int nl80211_cmd, u32 *num_rings, wifi_ring_buffer_status *status)
		: WifiCommand(sk, family, flags, nl80211_cmd)
	{
		*num_rings = 0;
		num = num_rings;
		mStatus = status;
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		interface_info *iface = (interface_info *)handle;

		put_u32(NL80211_ATTR_VENDOR_ID, OUI_GOOGLE);
		put_u32(NL80211_ATTR_VENDOR_SUBCMD, SKW_VCMD_GET_RING_BUFFERS_STATUS);

		if (iface->wdev_idx)
			put_u32(NL80211_ATTR_WDEV, iface->wdev_idx);
		else
			put_u32(NL80211_ATTR_IFINDEX, iface->iface_idx);

		return WIFI_SUCCESS;

	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		int type, left;
		struct nlattr *nla, *data;
		int i = 0, nr_buff = 0;

#define SKW_ATTR_RING_BUFFERS_STATUS    13
#define SKW_ATTR_NUM_RING_BUFFERS       14

		data = attr[NL80211_ATTR_VENDOR_DATA];
		if (!data)
			return WIFI_ERROR_NOT_AVAILABLE;

		nla_for_each_attr(nla, (struct nlattr *)nla_data(data), nla_len(data), left)
		{
			type = nla_type(nla);
			switch (type) {
			case SKW_ATTR_NUM_RING_BUFFERS:
				nr_buff = nla_get_u32(nla);
				break;

			case SKW_ATTR_RING_BUFFERS_STATUS:
				if (nla_len(nla) == sizeof(wifi_ring_buffer_status)) {
					memcpy(&mStatus[i++], nla_data(nla), nla_len(nla));
					*num = i;
				} else {
					ALOGE("wifi_ring_buffer_status not match");
				}

				break;

			default:
				break;
			}
		}

		if (nr_buff != *(int *)num)
			ALOGE("num of ring buffs not match, %d - %d", nr_buff, *num);

		return WIFI_SUCCESS;
	}
};

// There are no flags for these 3 in the legacy feature set. Adding them to
// the set because all the current devices support it.
//  *hidl_caps |= HidlChipCaps::DEBUG_RING_BUFFER_VENDOR_DATA;
//  *hidl_caps |= HidlChipCaps::DEBUG_HOST_WAKE_REASON_STATS;
//  *hidl_caps |= HidlChipCaps::DEBUG_ERROR_ALERTS;
wifi_error skw_wifi_get_ring_buffers_status(wifi_interface_handle iface,
		u32 *num_rings, wifi_ring_buffer_status *status)
{
	GetRingBuffStatus cmd(getSock(iface), getFamily(iface), 0,
			NL80211_CMD_VENDOR, num_rings, status);
	cmd.build(iface, NULL);
	cmd.send();

	ALOGD("%s, num rings: %d", __func__, *num_rings);

	return WIFI_SUCCESS;
}

class GetLoggerFeature : public WifiCommand
{
private:
	unsigned int mFeatures;

public:
	GetLoggerFeature(struct nl_sock *sk, int family, int flags, int nl80211_cmd)
		: WifiCommand(sk, family, flags, nl80211_cmd)
	{
		mFeatures = 0;
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		interface_info *iface = (interface_info *)handle;

		put_u32(NL80211_ATTR_VENDOR_ID, OUI_GOOGLE);
		put_u32(NL80211_ATTR_VENDOR_SUBCMD, SKW_VCMD_GET_LOGGER_FEATURES);

		if (iface->wdev_idx)
			put_u32(NL80211_ATTR_WDEV, iface->wdev_idx);
		else
			put_u32(NL80211_ATTR_IFINDEX, iface->iface_idx);

		return WIFI_SUCCESS;
	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		struct nlattr *data = attr[NL80211_ATTR_VENDOR_DATA];

		if (!data)
			return WIFI_ERROR_NOT_AVAILABLE;

		mFeatures = nla_get_u32(data);

		return WIFI_SUCCESS;
	}

	int features()
	{
		return mFeatures;
	}
};

wifi_error skw_wifi_get_logger_supported_feature_set(wifi_interface_handle iface,
							unsigned int *features)
{
	GetLoggerFeature cmd(getSock(iface), getFamily(iface), 0, NL80211_CMD_VENDOR);
	cmd.build(iface, NULL);
	cmd.send();

	*features = cmd.features();

	ALOGD("%s, features: 0x%x", __func__, *features);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_ring_data(wifi_interface_handle iface, char *ring_name)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_enable_tdls(wifi_interface_handle, mac_addr, wifi_tdls_params *,
		wifi_tdls_handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_disable_tdls(wifi_interface_handle, mac_addr)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_tdls_status(wifi_interface_handle, mac_addr, wifi_tdls_status *)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_tdls_capabilities(wifi_interface_handle iface,
		wifi_tdls_capabilities *capabilities)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_driver_version(wifi_interface_handle handle, char *buffer,
					int buffer_size)
{
	int subcmd = SKW_DRV_VERSION;
	wifi_error err = WIFI_ERROR_UNKNOWN;
	GetVersionCommand cmd(getSock(handle), getFamily(handle), 0, NL80211_CMD_VENDOR);

	if (cmd.build(handle, &subcmd) == WIFI_SUCCESS)
		err = cmd.send();

	if (err == WIFI_SUCCESS)
		strncpy(buffer, cmd.getVersion(), buffer_size);

	ALOGD("%s: %s", __func__, buffer);

	return err;
}

wifi_error skw_wifi_set_passpoint_list(wifi_request_id id, wifi_interface_handle iface,
		int num, wifi_passpoint_network *networks, wifi_passpoint_event_handler handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_reset_passpoint_list(wifi_request_id id, wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_lci(wifi_request_id id, wifi_interface_handle iface,
		wifi_lci_information *lci)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_lcr(wifi_request_id id, wifi_interface_handle iface,
		wifi_lcr_information *lcr)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

#if 0
wifi_error skw_wifi_start_sending_offloaded_packet(wifi_request_id id,
		wifi_interface_handle iface, u16 ether_type, u8 *ip_packet,
		u16 ip_packet_len, u8 *src_mac_addr, u8 *dst_mac_addr,
		u32 period_msec)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}
#endif

wifi_error skw_wifi_stop_sending_offloaded_packet(wifi_request_id id,
		wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_start_rssi_monitoring(wifi_request_id id, wifi_interface_handle
		iface, s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh)
{
	ALOGD("%s", __func__);
	return WIFI_SUCCESS;
}

wifi_error skw_wifi_stop_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_wake_reason_stats(wifi_interface_handle iface,
		WLAN_DRIVER_WAKE_REASON_CNT *wifi_wake_reason_cnt)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_configure_nd_offload(wifi_interface_handle iface, u8 enable)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_driver_memory_dump(wifi_interface_handle iface,
		wifi_driver_memory_dump_callbacks callbacks)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_start_pkt_fate_monitoring(wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_tx_pkt_fates(wifi_interface_handle handle,
		wifi_tx_report *tx_report_bufs, size_t n_requested_fates,
		size_t *n_provided_fates)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_rx_pkt_fates(wifi_interface_handle handle,
		wifi_rx_report *rx_report_bufs, size_t n_requested_fates,
		size_t *n_provided_fates)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/* NAN functions */
wifi_error skw_wifi_nan_enable_request(transaction_id id,
		wifi_interface_handle iface, NanEnableRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_disable_request(transaction_id id,
		wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_publish_request(transaction_id id,
		wifi_interface_handle iface, NanPublishRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_publish_cancel_request(transaction_id id,
		wifi_interface_handle iface, NanPublishCancelRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_subscribe_request(transaction_id id,
		wifi_interface_handle iface, NanSubscribeRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_subscribe_cancel_request(transaction_id id,
		wifi_interface_handle iface, NanSubscribeCancelRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_transmit_followup_request(transaction_id id,
		wifi_interface_handle iface, NanTransmitFollowupRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_stats_request(transaction_id id,
		wifi_interface_handle iface, NanStatsRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_config_request(transaction_id id,
		wifi_interface_handle iface, NanConfigRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_tca_request(transaction_id id,
		wifi_interface_handle iface, NanTCARequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_beacon_sdf_payload_request(transaction_id id,
		wifi_interface_handle iface, NanBeaconSdfPayloadRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_register_handler(wifi_interface_handle iface,
		NanCallbackHandler handlers)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_get_version(wifi_handle handle,
		NanVersion* version)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_get_capabilities(transaction_id id,
		wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_data_interface_create(transaction_id id,
		wifi_interface_handle iface,
		char *iface_name)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_data_interface_delete(transaction_id id,
		wifi_interface_handle iface, char *iface_name)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_data_request_initiator(
		transaction_id id, wifi_interface_handle iface,
		NanDataPathInitiatorRequest *msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_data_indication_response(
		transaction_id id, wifi_interface_handle iface,
		NanDataPathIndicationResponse *msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_nan_data_end(transaction_id id,
		wifi_interface_handle iface,
		NanDataPathEndRequest *msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_select_tx_power_scenario(wifi_interface_handle iface,
					wifi_power_scenario scenario)
{
	ALOGD("%s", __func__);

	return WIFI_ERROR_NOT_SUPPORTED;
}

wifi_error skw_wifi_reset_tx_power_scenario(wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_ERROR_NOT_SUPPORTED;
}

class GetPacketFilterCapa : public WifiCommand
{
private:
	u32 mVersion;
	u32 mLength;

public:
	GetPacketFilterCapa(struct nl_sock *sk, int family, int flags, int nl80211_cmd)
		: WifiCommand(sk, family, flags, nl80211_cmd)
	{
		mVersion = 0;
		mLength = 0;
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		interface_info *iface = (interface_info *)handle;

		put_u32(NL80211_ATTR_VENDOR_ID, OUI_GOOGLE);
		put_u32(NL80211_ATTR_VENDOR_SUBCMD, SKW_VCMD_GET_APF_CAPABILITIES);

		if (iface->wdev_idx)
			put_u32(NL80211_ATTR_WDEV, iface->wdev_idx);
		else
			put_u32(NL80211_ATTR_IFINDEX, iface->iface_idx);

		return WIFI_SUCCESS;
	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		int type, left;
		struct nlattr *nla, *data;

#define SKW_ATTR_APF_VERSION     0
#define SKW_ATTR_APF_MAX_LEN     1

		data = attr[NL80211_ATTR_VENDOR_DATA];
		if (!data)
			return WIFI_ERROR_NOT_AVAILABLE;

		nla_for_each_attr(nla, (struct nlattr *)nla_data(data), nla_len(data), left)
		{
			type = nla_type(nla);
			switch (type) {
			case SKW_ATTR_APF_VERSION:
				mVersion = nla_get_u32(nla);
				break;

			case SKW_ATTR_APF_MAX_LEN:
				mLength = nla_get_u32(nla);
				break;

			default:
				break;
			}
		}

		return WIFI_SUCCESS;
	}

	u32 version()
	{
		return mVersion;
	}

	u32 max_len()
	{
		return mLength;
	}
};

/**
 * Returns the chipset's hardware filtering capabilities:
 * @param version pointer to version of the packet filter interpreter
 *                supported, filled in upon return. 0 indicates no support.
 * @param max_len pointer to maximum size of the filter bytecode, filled in
 *                upon return.
 */
wifi_error skw_wifi_get_packet_filter_capabilities(wifi_interface_handle iface,
		u32 *version, u32 *max_len)
{
	GetPacketFilterCapa cmd(getSock(iface), getFamily(iface), 0, NL80211_CMD_VENDOR);
	cmd.build(iface, NULL);
	cmd.send();

	*version = cmd.version();
	*max_len = cmd.max_len();

	ALOGD("%s, version: %d, max_len: %d\n", __func__, *version, *max_len);

	return WIFI_SUCCESS;
}
/**
      * Programs the packet filter.
 * @param program pointer to the program byte-code.
 * @param len length of the program byte-code.
     */
wifi_error skw_wifi_set_packet_filter(wifi_interface_handle handle,
		const u8 *program, u32 len)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_read_packet_filter(wifi_interface_handle handle,
		u32 src_offset, u8 *host_dst,
		u32 length)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_get_roaming_capabilities(wifi_interface_handle handle,
		wifi_roaming_capabilities *caps)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_enable_firmware_roaming(wifi_interface_handle handle,
		fw_roaming_state_t state)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_configure_roaming(wifi_interface_handle handle,
		wifi_roaming_config *roaming_config)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_radio_mode_change_handler(wifi_request_id id, wifi_interface_handle
		iface, wifi_radio_mode_change_handler eh)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

#if __ANDROID_API__ > __ANDROID_API_Q__
wifi_error skw_wifi_set_latency_mode(wifi_interface_handle iface,
		wifi_latency_mode mode)
{
	ALOGD("%s", __func__);

	return WIFI_ERROR_NOT_SUPPORTED;
}

wifi_error skw_wifi_set_thermal_mitigation_mode(wifi_handle handle,
		wifi_thermal_mode mode, u32 completion_window)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_map_dscp_access_category(wifi_handle handle,
		u32 start, u32 end, u32 access_category)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_reset_dscp_mapping(wifi_handle handle)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_virtual_interface_create(wifi_handle handle, const char* ifname,
		wifi_interface_type iface_type)
{
	ALOGD("%s: ifname: %s, type: 0x%x", __func__, ifname, iface_type);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_virtual_interface_delete(wifi_handle handle, const char* ifname)
{
	ALOGD("%s: ifname: %s", __func__, ifname);

	return WIFI_SUCCESS;
}

wifi_error skw_wifi_set_subsystem_restart_handler(wifi_handle handle,
		wifi_subsystem_restart_handler handler)
{
	ALOGD("%s", __func__);

	return WIFI_ERROR_NOT_SUPPORTED;
}

/**
 * Allow vendor HAL to choose interface name when creating
 * an interface. This can be implemented by chips with their
 * own interface naming policy.
 * If not implemented, the default naming will be used.
 */
wifi_error skw_wifi_get_supported_iface_name(wifi_handle handle, u32 iface_type,
		char *name, size_t len)
{
	return WIFI_ERROR_NOT_AVAILABLE;
}

/**
 * Perform early initialization steps that are needed when WIFI
 * is disabled.
 * If the function returns failure, it means the vendor HAL is unusable
 * (for example, if chip hardware is not installed) and no further
 * functions should be called.
 */
wifi_error skw_wifi_early_initialize(void)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**
 * Get supported feature set which are chip-global, that is
 * not dependent on any created interface.
 */
wifi_error skw_wifi_get_chip_feature_set(wifi_handle handle, feature_set *set)
{
	ALOGD("%s: 0x%x", __func__, *set);

#if 0
	*set |= WIFI_FEATURE_INFRA;
	*set |= WIFI_FEATURE_INFRA_5G;
	*set |= WIFI_FEATURE_P2P;
	*set |= WIFI_FEATURE_SOFT_AP;
#endif

	return WIFI_SUCCESS;
}

/**
 * Invoked to indicate that the provided iface is the primary STA iface when there are more
 * than 1 STA iface concurrently active.
 */
wifi_error skw_wifi_multi_sta_set_primary_connection(wifi_handle handle,
		wifi_interface_handle iface)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**
 * When there are 2 simultaneous STA connections, this use case hint
 * indicates what STA + STA use-case is being enabled by the framework.
 */
wifi_error skw_wifi_multi_sta_set_use_case(wifi_handle handle,
				wifi_multi_sta_use_case use_case)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**
 * Invoked to indicate that the following list of wifi_coex_unsafe_channel should be avoided
 * with the specified restrictions.
 * @param unsafeChannels list of current |wifi_coex_unsafe_channel| to avoid.
 * @param restrictions bitmask of |wifi_coex_restriction| indicating wifi interfaces to
 *         restrict from the current unsafe channels.
 */
wifi_error skw_wifi_set_coex_unsafe_channels(wifi_handle handle, u32 num_channels,
			wifi_coex_unsafe_channel *unsafeChannels, u32 restrictions)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**
 * Invoked to set voip optimization mode for the provided STA iface
 */
wifi_error skw_wifi_set_voip_mode(wifi_interface_handle iface, wifi_voip_mode mode)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**@brief twt_register_handler
 *        Request to register TWT callback before sending any TWT request
 * @param wifi_interface_handle:
 * @param TwtCallbackHandler: callback function pointers
 * @return Synchronous wifi_error
 */
wifi_error skw_wifi_twt_register_handler(wifi_interface_handle iface,
					TwtCallbackHandler handler)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**@brief twt_get_capability
 *        Request TWT capability
 * @param wifi_interface_handle:
 * @return Synchronous wifi_error and TwtCapabilitySet
 */
wifi_error skw_wifi_twt_get_capability(wifi_interface_handle iface,
					TwtCapabilitySet* twt_cap_set)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**@brief twt_setup_request
 *        Request to send TWT setup frame
 * @param wifi_interface_handle:
 * @param TwtSetupRequest: detailed parameters of setup request
 * @return Synchronous wifi_error
 * @return Asynchronous EventTwtSetupResponse CB return TwtSetupResponse
 */
wifi_error skw_wifi_twt_setup_request(wifi_interface_handle iface,
					TwtSetupRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**@brief twt_teardown_request
 *        Request to send TWT teardown frame
 * @param wifi_interface_handle:
 * @param TwtTeardownRequest: detailed parameters of teardown request
 * @return Synchronous wifi_error
 * @return Asynchronous EventTwtTeardownCompletion CB return TwtTeardownCompletion
 * TwtTeardownCompletion may also be received due to other events
 * like CSA, BTCX, TWT scheduler, MultiConnection, peer-initiated teardown, etc.
 */
wifi_error skw_wifi_twt_teardown_request(wifi_interface_handle iface,
					TwtTeardownRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**@brief twt_info_frame_request
 *        Request to send TWT info frame
 * @param wifi_interface_handle:
 * @param TwtInfoFrameRequest: detailed parameters in info frame
 * @return Synchronous wifi_error
 * @return Asynchronous EventTwtInfoFrameReceived CB return TwtInfoFrameReceived
 * Driver may also receive Peer-initiated TwtInfoFrame
 */
wifi_error skw_wifi_twt_info_frame_request(wifi_interface_handle iface,
					TwtInfoFrameRequest* msg)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**@brief twt_get_stats
 *        Request to get TWT stats
 * @param wifi_interface_handle:
 * @param config_id: configuration ID of TWT request
 * @return Synchronous wifi_error and TwtStats
 */
wifi_error skw_wifi_twt_get_stats(wifi_interface_handle iface, u8 config_id,
				TwtStats* stats)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**@brief twt_clear_stats
 *        Request to clear TWT stats
 * @param wifi_interface_handle:
 * @param config_id: configuration ID of TWT request
 * @return Synchronous wifi_error
 */
wifi_error skw_wifi_twt_clear_stats(wifi_interface_handle iface, u8 config_id)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

/**
 * Invoked to set DTIM configuration when the host is in the suspend mode
 * @param wifi_interface_handle:
 * @param multiplier: when STA in the power saving mode, the wake up interval will be set to
 *              1) multiplier * DTIM period if multiplier > 0.
 *              2) the device default value if multiplier <=0
 * Some implementations may apply an additional cap to wake up interval in the case of 1).
 */
wifi_error skw_wifi_set_dtim_config(wifi_interface_handle handle, u32 multiplier)
{
	ALOGD("%s", __func__);

	return WIFI_SUCCESS;
}

static wifi_channel_width to_hal_band_width(int skw_bw)
{
	wifi_channel_width cw;

	switch (skw_bw) {
	case SKW_CHAN_WIDTH_20:
		cw = WIFI_CHAN_WIDTH_20;
		break;

	case SKW_CHAN_WIDTH_40:
		cw = WIFI_CHAN_WIDTH_40;
		break;

	case SKW_CHAN_WIDTH_80:
		cw = WIFI_CHAN_WIDTH_80;
		break;

	case SKW_CHAN_WIDTH_160:
		cw = WIFI_CHAN_WIDTH_160;
		break;

	case SKW_CHAN_WIDTH_80P80:
		cw = WIFI_CHAN_WIDTH_80P80;
		break;

	default:
		cw = WIFI_CHAN_WIDTH_INVALID;
		break;
	}

	return cw;
}

static int to_skw_band(int hal_bw)
{
	int skw_bw;

	switch (hal_bw) {
	case WLAN_MAC_2_4_BAND:
		skw_bw = SKW_BAND_2GHZ;
		break;

	case WLAN_MAC_5_0_BAND:
		skw_bw = SKW_BAND_5GHZ;
		break;

	case WLAN_MAC_6_0_BAND:
		skw_bw = SKW_BAND_6GHZ;
		break;

	case WLAN_MAC_60_0_BAND:
		skw_bw = SKW_BAND_60GHZ;
		break;

	default:
		skw_bw = SKW_INVALID;
		break;
	}

	return skw_bw;
}

static u32 to_skw_band_mask(u32 hal_band_mask)
{
	int band;
	u32 tmp, skw_mask = 0;

	while (hal_band_mask) {
		tmp = hal_band_mask - 1;
		band = to_skw_band(((tmp ^ hal_band_mask) + 1) >> 1);
		if (band != SKW_INVALID)
			skw_mask |= SKW_BIT(band);

		hal_band_mask &= tmp;
	}

	return skw_mask;
}

static int to_hal_iface_mode_mask(int skw_mode_mask)
{
	int hal_mask;

	switch (skw_mode_mask) {
	case SKW_BIT(SKW_MODE_STA):
		hal_mask = SKW_BIT(WIFI_INTERFACE_TYPE_STA);
		break;

	case SKW_BIT(SKW_MODE_AP):
		hal_mask = SKW_BIT(WIFI_INTERFACE_TYPE_AP);
		break;

	case SKW_BIT(SKW_MODE_GC):
	case SKW_BIT(SKW_MODE_GO):
		hal_mask = SKW_BIT(WIFI_INTERFACE_TYPE_P2P);
		break;

	default:
		hal_mask = SKW_INVALID;
		break;
	}

	return hal_mask;
}

static u32 to_hal_iface_mask(u32 skw_iface_mask)
{
	u32 mask, tmp;
	u32 hal_mask = 0;

	while (skw_iface_mask) {
		tmp = skw_iface_mask - 1;

		mask = to_hal_iface_mode_mask(((tmp ^ skw_iface_mask) + 1) >> 1);
		if (mask != SKW_INVALID)
			hal_mask |= mask;

		skw_iface_mask &= tmp;
	}

	return hal_mask;
}

static int to_skw_iface_mode_mask(int hal_mode_mask)
{
	int hal_mask;

	switch (hal_mode_mask) {
	case SKW_BIT(WIFI_INTERFACE_TYPE_STA):
		hal_mask = SKW_BIT(SKW_MODE_STA);
		break;

	case SKW_BIT(WIFI_INTERFACE_TYPE_AP):
		hal_mask = SKW_BIT(SKW_MODE_AP);
		break;

	case SKW_BIT(WIFI_INTERFACE_TYPE_P2P):
		hal_mask = SKW_BIT(SKW_MODE_GC);
		break;
	default:
		hal_mask = SKW_INVALID;
		break;
	}

	return hal_mask;
}

static u32 to_skw_iface_mask(u32 hal_iface_mask)
{
	u32 mask, tmp;
	u32 skw_mask = 0;

	while (hal_iface_mask) {
		tmp = hal_iface_mask - 1;

		mask = to_skw_iface_mode_mask(((tmp ^ hal_iface_mask) + 1) >> 1);
		if (mask != SKW_INVALID)
			skw_mask |= mask;

		hal_iface_mask &= tmp;
	}

	return skw_mask;
}

class GetUsableChannels : public WifiCommand
{
private:
	u32 mChanDataLen;
	void *mChanData;

public:
	GetUsableChannels(struct nl_sock *sk, int family, int flags, int nl80211_cmd)
		: WifiCommand(sk, family, flags, nl80211_cmd)
	{
		mChanData = NULL;
		mChanDataLen = 0;
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		struct nlattr *data;
		interface_info *iface = (interface_info *)handle;
		struct skw_usable_chan_info *info = (struct skw_usable_chan_info *)param;

		if (iface->wdev_idx)
			put_u32(NL80211_ATTR_WDEV, iface->wdev_idx);
		else
			put_u32(NL80211_ATTR_IFINDEX, iface->iface_idx);

		put_u32(NL80211_ATTR_VENDOR_ID, OUI_GOOGLE);
		put_u32(NL80211_ATTR_VENDOR_SUBCMD, SKW_VCMD_GET_USABLE_CHANS);

		put_data(NL80211_ATTR_VENDOR_DATA, sizeof(*info), info);

		return WIFI_SUCCESS;
	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		struct nlattr *data;

		data = attr[NL80211_ATTR_VENDOR_DATA];
		if (!data)
			return WIFI_ERROR_NOT_AVAILABLE;

		mChanDataLen = nla_len(data);
		mChanData = malloc(mChanDataLen);
		if (!mChanData)
			mChanDataLen = 0;

		memcpy(mChanData, nla_data(data), mChanDataLen);

		return WIFI_SUCCESS;
	}

	u32 channel_data_len()
	{
		return mChanDataLen;
	}

	struct skw_usable_chan *channel_data()
	{
		return (struct skw_usable_chan *)mChanData;
	}

	~GetUsableChannels()
	{
		free(mChanData);
	}
};

/**@brief wifi_get_usable_channels
 *        Request list of usable channels for the requested bands and modes. Usable
 *        implies channel is allowed as per regulatory for the current country code
 *        and not restricted due to other hard limitations (e.g. DFS, Coex) In
 *        certain modes (e.g. STA+SAP) there could be other hard restrictions
 *        since MCC operation many not be supported by SAP. This API also allows
 *        driver to return list of usable channels for each mode uniquely to
 *        distinguish cases where only a limited set of modes are allowed on
 *        a given channel e.g. srd channels may be supported for P2P but not
 *        for SAP or P2P-Client may be allowed on an indoor channel but P2P-GO
 *        may not be allowed. This API is not interface specific and will be
 *        used to query capabilities of driver in terms of what modes (STA, SAP,
 *        P2P_CLI, P2P_GO, NAN, TDLS) can be supported on each of the channels.
 * @param handle global wifi_handle
 * @param band_mask BIT MASK of WLAN_MAC* as represented by |wlan_mac_band|
 * @param iface_mode_mask BIT MASK of BIT(WIFI_INTERFACE_*) represented by
 *        |wifi_interface_mode|. Bitmask respresents all the modes that the
 *        caller is interested in (e.g. STA, SAP, WFD-CLI, WFD-GO, TDLS, NAN).
 *        Note: Bitmask does not represent concurrency matrix. If the caller
 *        is interested in CLI, GO modes, the iface_mode_mask would be set
 *        to WIFI_INTERFACE_P2P_CLIENT|WIFI_INTERFACE_P2P_GO.
 * @param filter_mask BIT MASK of WIFI_USABLE_CHANNEL_FILTER_* represented by
 *        |wifi_usable_channel_filter|. Indicates if the channel list should
 *        be filtered based on additional criteria. If filter_mask is not
 *        specified, driver should return list of usable channels purely
 *        based on regulatory constraints.
 * @param max_size maximum number of |wifi_usable_channel|
 * @param size actual number of |wifi_usable_channel| entries returned by driver
 * @param channels list of usable channels represented by |wifi_usable_channel|
 */
wifi_error skw_wifi_get_usable_channels(wifi_handle handle, u32 band_mask, u32 iface_mode_mask,
					u32 filter_mask, u32 max_size, u32* size,
					wifi_usable_channel* channels)
{
	u32 i, nr_chan;
	struct skw_usable_chan *chan;
	hal_info *hal = (hal_info *)handle;
	struct skw_usable_chan_info info;

	GetUsableChannels cmd(hal->nl_hal, hal->family_nl80211, 0, NL80211_CMD_VENDOR);

	info.band_mask = to_skw_band_mask(band_mask);
	info.iface_mode_mask = to_skw_iface_mask(iface_mode_mask);

	cmd.build((wifi_interface_handle)&hal->interfaces[0], &info);
	cmd.send();

	nr_chan = cmd.channel_data_len() / sizeof(*chan);
	*size = nr_chan > max_size ? max_size : nr_chan;

	chan = cmd.channel_data();

	for (i = 0; i < *size; i++)
	{
		channels[i].freq = chan[i].center_freq;
		channels[i].width = to_hal_band_width(chan[i].band_width);
		channels[i].iface_mode_mask = to_hal_iface_mask(chan[i].iface_mode_mask);
	}

	ALOGD("%s, band_mask: 0x%x, max_size: %d, size: %d\n",
	      __func__, band_mask, max_size, *size);

	return WIFI_SUCCESS;
}

/**
 * Trigger wifi subsystem restart to reload firmware
 */
wifi_error skw_wifi_trigger_subsystem_restart(wifi_handle handle)
{
	ALOGD("%s", __func__);

	return WIFI_ERROR_NOT_SUPPORTED;
}
#endif

static nl_sock *skw_create_socket(int port)
{
	struct nl_sock * sock = NULL;
	uint32_t pid = getpid() & 0x3FFFFF;

	sock = nl_socket_alloc();
	if (sock == NULL) {
		ALOGE("%s: socked alloc failed", __func__);

		return NULL;
	}

	nl_socket_set_local_port(sock, pid + (port << 22));
	if (nl_connect(sock, NETLINK_GENERIC)) {
		ALOGE("%s: connect failed", __func__);

		nl_socket_free(sock);

		return NULL;
	}

	if (nl_socket_set_buffer_size(sock, SOCK_BUFF_SIZE, 0) < 0)
		ALOGE("cmd_sock: set RX buffer failed, %d", strerror(errno));

	return sock;
}

class GetMulticastId : public WifiCommand {
private:
	char *group;
	int mId;

public:
	GetMulticastId(struct nl_sock *sk, int family, int flags, int nl80211_cmd)
		: WifiCommand(sk, family, flags, nl80211_cmd)
	{
		group = NULL;
		mId = -1;
	}

	virtual wifi_error build(wifi_interface_handle handle, void *param)
	{
		group = (char *)param;
		put_string(CTRL_ATTR_FAMILY_NAME, "nl80211");

		return WIFI_SUCCESS;
	}

	virtual wifi_error parser(struct nlattr *attr[NL80211_ATTR_MAX])
	{
		int i;
		struct nlattr *mcgrp;

		if (!attr[CTRL_ATTR_MCAST_GROUPS])
			return WIFI_ERROR_UNKNOWN;

		skw_nla_for_each_nested(mcgrp, attr[CTRL_ATTR_MCAST_GROUPS], i) {
			struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];

			nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, (struct nlattr *)nla_data(mcgrp),
				  nla_len(mcgrp), NULL);

			if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] || !tb2[CTRL_ATTR_MCAST_GRP_ID] ||
			    strncmp((const char *)nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]),
				    group, nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME])) != 0)
				continue;

			mId = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);

			break;
		};

		return WIFI_SUCCESS;
	}

	int id()
	{
		return mId;
	}
};

static int skw_get_multicast_id(struct nl_sock *sk, const char *group)
{
	GetMulticastId cmd(sk, genl_ctrl_resolve(sk, "nlctrl"), 0, CTRL_CMD_GETFAMILY);
	if (cmd.build(NULL, (void *)group) == WIFI_SUCCESS)
		cmd.send();

	return cmd.id();
}

static int skw_add_membership(struct nl_sock *sk, const char *group)
{
	int id = skw_get_multicast_id(sk, group);
	if (id < 0) {
		ALOGE("Could not find group %s", group);
		return id;
	}

	int ret = nl_socket_add_membership(sk, id);
	if (ret < 0)
		ALOGE("Could not add membership to group %s", group);

	return ret;
}

static int evtHandler(struct nl_msg *msg, void *arg)
{
	ALOGD("%s", __func__);

	return NL_SKIP;
}

static wifi_error skw_wifi_event_init(hal_info *hal)
{
	int err;

	hal->nl_event = skw_create_socket(WIFI_HAL_SOCK_EVENT_PORT);
	if (hal->nl_event == NULL) {
		ALOGE("%s: create event socket failed", __func__);

		return WIFI_ERROR_UNKNOWN;
	}

	//nl_socket_modify_cb(hal->nl_event, NL_CB_SEQ_CHECK, no_seq_check, &err);
	nl_socket_modify_cb(hal->nl_event, NL_CB_VALID, NL_CB_CUSTOM, evtHandler, &err);

	skw_add_membership(hal->nl_event, "scan");
	skw_add_membership(hal->nl_event, "mlme");
	skw_add_membership(hal->nl_event, "vendor");
	skw_add_membership(hal->nl_event, "regulatory");

	return WIFI_SUCCESS;
}

static void skw_wifi_event_deinit(hal_info *hal)
{
	if (hal->nl_event)
		nl_socket_free(hal->nl_event);
}

static wifi_error skw_wifi_hal_init(hal_info *hal)
{
	hal->nl_hal = skw_create_socket(WIFI_HAL_SOCK_DEFAULT_PORT);
	if (hal->nl_hal == NULL) {
		ALOGE("%s: create command socket failed", __func__);

		return WIFI_ERROR_UNKNOWN;
	}

	hal->family_nl80211 = genl_ctrl_resolve(hal->nl_hal, "nl80211");
	if (hal->family_nl80211 < 0) {
		nl_socket_free(hal->nl_hal);

		ALOGE("%s: resolve nl80211 id failed", __func__);

		return WIFI_ERROR_UNKNOWN;
	}

	return WIFI_SUCCESS;
}

static void skw_wifi_hal_deinit(hal_info *hal)
{
	if (hal->nl_hal)
		nl_socket_free(hal->nl_hal);
}

static wifi_error skw_wifi_initialize(wifi_handle *handle)
{
	wifi_error err;
	hal_info *hal = NULL;

	ALOGD("%s", __func__);

	hal = (hal_info *)malloc(sizeof(hal_info));
	if (hal == NULL) {
		ALOGE("%s: alloc hal_info failed", __func__);

		return WIFI_ERROR_OUT_OF_MEMORY;
	}

	memset(hal, 0, sizeof(*hal));

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, hal->exit_socks) == -1) {
		ALOGE("socketpair failed");

		free(hal);

		return WIFI_ERROR_UNKNOWN;
	}

	err = skw_wifi_hal_init(hal);
	if (err != WIFI_SUCCESS) {
		free(hal);
		return err;
	}

	err = skw_wifi_event_init(hal);
	if (err != WIFI_SUCCESS) {
		skw_wifi_hal_deinit(hal);
		free(hal);

		return err;
	}

	*handle = (wifi_handle)hal;

	return WIFI_SUCCESS;
}

static void skw_wifi_deinitialize(wifi_handle handle)
{
	hal_info *hal = (hal_info *)handle;

	if (hal->cleaned_up_handler)
		(*(hal->cleaned_up_handler))(handle);

	skw_wifi_hal_deinit(hal);
	skw_wifi_event_deinit(hal);

	if (hal->exit_socks[0])
		close(hal->exit_socks[0]);

	if (hal->exit_socks[1])
		close(hal->exit_socks[1]);

	free(hal);
}

void skw_wifi_cleanup(wifi_handle handle, wifi_cleaned_up_handler clean_handler)
{
	hal_info *hal = (hal_info *)handle;

	ALOGD("%s", __func__);

	hal->cleaned_up_handler = clean_handler;
	hal->exit = true;

	TEMP_FAILURE_RETRY(write(hal->exit_socks[0], "exit", 4));
}

static int skw_socket_handler(hal_info *hal, int events, struct nl_sock *sk)
{
	int ret;
	struct nl_cb *cb = nl_socket_get_cb(sk);

	ret = nl_recvmsgs(sk, cb);

	nl_cb_put(cb);

	return ret;
}

static void skw_wifi_event_loop(wifi_handle handle)
{
	pollfd fd[2];
	hal_info *hal = (hal_info *)handle;

	ALOGD("%s", __func__);

	memset(&fd[0], 0, sizeof(fd));

	fd[0].fd = nl_socket_get_fd(hal->nl_event);
	fd[0].events = POLLIN;

	fd[1].fd = hal->exit_socks[1];
	fd[1].events = POLLIN;

	do {
		fd[0].revents = 0;
		fd[1].revents = 0;

		if (poll(fd, 2, -1) > 0) {
			if (fd[0].revents & POLLIN) {
				skw_socket_handler(hal, fd[0].revents, hal->nl_event);
			}
		}

	} while (!hal->exit);

	skw_wifi_deinitialize(handle);
}

#define POLL_DRIVER_DURATION_US (100000)
#define POLL_DRIVER_MAX_TIME_MS (10000)
static wifi_error skw_wifi_wait_for_driver_ready(void)
{
	int count = (POLL_DRIVER_MAX_TIME_MS * 1000) / POLL_DRIVER_DURATION_US;

	ALOGD("%s", __func__);

	do {
		if ((access("/sys/class/net/wlan0", F_OK)) == 0)
			return WIFI_SUCCESS;

		usleep(POLL_DRIVER_DURATION_US);

	} while(--count > 0);

	ALOGE("Time out waiting on Driver ready ... ");

	return WIFI_ERROR_TIMED_OUT;
}

wifi_error init_wifi_vendor_hal_func_table(wifi_hal_fn *fn)
{
	if (!fn) {
		ALOGE("invalid parameter fn");
		return WIFI_ERROR_UNINITIALIZED;
	}

	fn->wifi_initialize = skw_wifi_initialize;
	fn->wifi_wait_for_driver_ready = skw_wifi_wait_for_driver_ready;
	fn->wifi_cleanup = skw_wifi_cleanup;
	fn->wifi_event_loop = skw_wifi_event_loop;
	fn->wifi_get_error_info = skw_wifi_get_error_info;
	fn->wifi_get_supported_feature_set = skw_wifi_get_supported_feature_set;
	fn->wifi_get_concurrency_matrix = skw_wifi_get_concurrency_matrix;
	fn->wifi_set_scanning_mac_oui = skw_wifi_set_scanning_mac_oui;
	fn->wifi_get_supported_channels = skw_wifi_get_supported_channels;
	fn->wifi_is_epr_supported = skw_wifi_is_epr_supported;
	fn->wifi_get_ifaces = skw_wifi_get_ifaces;
	fn->wifi_get_iface_name = skw_wifi_get_iface_name;
	fn->wifi_set_iface_event_handler = skw_wifi_set_iface_event_handler;
	fn->wifi_reset_iface_event_handler = skw_wifi_reset_iface_event_handler;
	fn->wifi_start_gscan = skw_wifi_start_gscan;
	fn->wifi_stop_gscan = skw_wifi_stop_gscan;
	fn->wifi_get_cached_gscan_results = skw_wifi_get_cached_gscan_results;
	fn->wifi_set_bssid_hotlist = skw_wifi_set_bssid_hotlist;
	fn->wifi_reset_bssid_hotlist = skw_wifi_reset_bssid_hotlist;
	fn->wifi_set_significant_change_handler = skw_wifi_set_significant_change_handler;
	fn->wifi_reset_significant_change_handler = skw_wifi_reset_significant_change_handler;
	fn->wifi_get_gscan_capabilities = skw_wifi_get_gscan_capabilities;
	fn->wifi_set_link_stats = skw_wifi_set_link_stats;
	fn->wifi_get_link_stats = skw_wifi_get_link_stats;
	fn->wifi_clear_link_stats = skw_wifi_clear_link_stats;
	fn->wifi_get_valid_channels = skw_wifi_get_valid_channels;
	fn->wifi_rtt_range_request = skw_wifi_rtt_range_request;
	fn->wifi_rtt_range_cancel = skw_wifi_rtt_range_cancel;
	fn->wifi_get_rtt_capabilities = skw_wifi_get_rtt_capabilities;
	fn->wifi_rtt_get_responder_info = skw_wifi_rtt_get_responder_info;
	fn->wifi_enable_responder = skw_wifi_enable_responder;
	fn->wifi_disable_responder = skw_wifi_disable_responder;
	fn->wifi_set_nodfs_flag = skw_wifi_set_nodfs_flag;
	fn->wifi_start_logging = skw_wifi_start_logging;
	fn->wifi_set_epno_list = skw_wifi_set_epno_list;
	fn->wifi_reset_epno_list = skw_wifi_reset_epno_list;
	fn->wifi_set_country_code = skw_wifi_set_country_code;
	fn->wifi_get_firmware_memory_dump = skw_wifi_get_firmware_memory_dump;
	fn->wifi_set_log_handler = skw_wifi_set_log_handler;
	fn->wifi_reset_log_handler = skw_wifi_reset_log_handler;
	fn->wifi_set_alert_handler = skw_wifi_set_alert_handler;
	fn->wifi_reset_alert_handler = skw_wifi_reset_alert_handler;
	fn->wifi_get_firmware_version = skw_wifi_get_firmware_version;
	fn->wifi_get_ring_buffers_status = skw_wifi_get_ring_buffers_status;
	fn->wifi_get_logger_supported_feature_set = skw_wifi_get_logger_supported_feature_set;
	fn->wifi_get_ring_data = skw_wifi_get_ring_data;
	fn->wifi_enable_tdls = skw_wifi_enable_tdls;
	fn->wifi_disable_tdls = skw_wifi_disable_tdls;
	fn->wifi_get_tdls_status = skw_wifi_get_tdls_status;
	fn->wifi_get_tdls_capabilities = skw_wifi_get_tdls_capabilities;
	fn->wifi_get_driver_version = skw_wifi_get_driver_version;
	fn->wifi_set_passpoint_list = skw_wifi_set_passpoint_list;
	fn->wifi_reset_passpoint_list = skw_wifi_reset_passpoint_list;
	fn->wifi_set_lci = skw_wifi_set_lci;
	fn->wifi_set_lcr = skw_wifi_set_lcr;
	// fn->wifi_start_sending_offloaded_packet = skw_wifi_start_sending_offloaded_packet;
	fn->wifi_stop_sending_offloaded_packet = skw_wifi_stop_sending_offloaded_packet;
	fn->wifi_start_rssi_monitoring = skw_wifi_start_rssi_monitoring;
	fn->wifi_stop_rssi_monitoring = skw_wifi_stop_rssi_monitoring;
	fn->wifi_get_wake_reason_stats = skw_wifi_get_wake_reason_stats;
	fn->wifi_configure_nd_offload = skw_wifi_configure_nd_offload;
	fn->wifi_get_driver_memory_dump = skw_wifi_get_driver_memory_dump;
	fn->wifi_start_pkt_fate_monitoring = skw_wifi_start_pkt_fate_monitoring;
	fn->wifi_get_tx_pkt_fates = skw_wifi_get_tx_pkt_fates;
	fn->wifi_get_rx_pkt_fates = skw_wifi_get_rx_pkt_fates;
	fn->wifi_nan_enable_request = skw_wifi_nan_enable_request;
	fn->wifi_nan_disable_request = skw_wifi_nan_disable_request;
	fn->wifi_nan_publish_request = skw_wifi_nan_publish_request;
	fn->wifi_nan_publish_cancel_request = skw_wifi_nan_publish_cancel_request;
	fn->wifi_nan_subscribe_request = skw_wifi_nan_subscribe_request;
	fn->wifi_nan_subscribe_cancel_request = skw_wifi_nan_subscribe_cancel_request;
	fn->wifi_nan_transmit_followup_request = skw_wifi_nan_transmit_followup_request;
	fn->wifi_nan_stats_request = skw_wifi_nan_stats_request;
	fn->wifi_nan_config_request = skw_wifi_nan_config_request;
	fn->wifi_nan_tca_request = skw_wifi_nan_tca_request;
	fn->wifi_nan_beacon_sdf_payload_request = skw_wifi_nan_beacon_sdf_payload_request;
	fn->wifi_nan_register_handler = skw_wifi_nan_register_handler;
	fn->wifi_nan_get_version = skw_wifi_nan_get_version;
	fn->wifi_nan_get_capabilities = skw_wifi_nan_get_capabilities;
	fn->wifi_nan_data_interface_create = skw_wifi_nan_data_interface_create;
	fn->wifi_nan_data_interface_delete = skw_wifi_nan_data_interface_delete;
	fn->wifi_nan_data_request_initiator = skw_wifi_nan_data_request_initiator;
	fn->wifi_nan_data_indication_response = skw_wifi_nan_data_indication_response;
	fn->wifi_nan_data_end = skw_wifi_nan_data_end;
	fn->wifi_select_tx_power_scenario = skw_wifi_select_tx_power_scenario;
	fn->wifi_reset_tx_power_scenario = skw_wifi_reset_tx_power_scenario;
	fn->wifi_get_packet_filter_capabilities = skw_wifi_get_packet_filter_capabilities;
	fn->wifi_set_packet_filter = skw_wifi_set_packet_filter;
	fn->wifi_read_packet_filter = skw_wifi_read_packet_filter;
	fn->wifi_get_roaming_capabilities = skw_wifi_get_roaming_capabilities;
	fn->wifi_enable_firmware_roaming = skw_wifi_enable_firmware_roaming;
	fn->wifi_configure_roaming = skw_wifi_configure_roaming;
	fn->wifi_set_radio_mode_change_handler = skw_wifi_set_radio_mode_change_handler;
#if __ANDROID_API__ > __ANDROID_API_Q__
	fn->wifi_set_latency_mode = skw_wifi_set_latency_mode;
	fn->wifi_set_thermal_mitigation_mode = skw_wifi_set_thermal_mitigation_mode;
	fn->wifi_map_dscp_access_category = skw_wifi_map_dscp_access_category;
	fn->wifi_reset_dscp_mapping = skw_wifi_reset_dscp_mapping;
	fn->wifi_virtual_interface_create = skw_wifi_virtual_interface_create;
	fn->wifi_virtual_interface_delete = skw_wifi_virtual_interface_delete;
	fn->wifi_set_subsystem_restart_handler = skw_wifi_set_subsystem_restart_handler;
	fn->wifi_get_supported_iface_name = skw_wifi_get_supported_iface_name;
	fn->wifi_early_initialize = skw_wifi_early_initialize;
	fn->wifi_get_chip_feature_set = skw_wifi_get_chip_feature_set;
	fn->wifi_multi_sta_set_primary_connection = skw_wifi_multi_sta_set_primary_connection;
	fn->wifi_multi_sta_set_use_case = skw_wifi_multi_sta_set_use_case;
	fn->wifi_set_coex_unsafe_channels = skw_wifi_set_coex_unsafe_channels;
	fn->wifi_set_voip_mode = skw_wifi_set_voip_mode;
	fn->wifi_twt_register_handler = skw_wifi_twt_register_handler;
	fn->wifi_twt_get_capability = skw_wifi_twt_get_capability;
	fn->wifi_twt_setup_request = skw_wifi_twt_setup_request;
	fn->wifi_twt_teardown_request = skw_wifi_twt_teardown_request;
	fn->wifi_twt_info_frame_request = skw_wifi_twt_info_frame_request;
	fn->wifi_twt_get_stats = skw_wifi_twt_get_stats;
	fn->wifi_twt_clear_stats = skw_wifi_twt_clear_stats;
	fn->wifi_set_dtim_config = skw_wifi_set_dtim_config;
	fn->wifi_get_usable_channels = skw_wifi_get_usable_channels;
	fn->wifi_trigger_subsystem_restart = skw_wifi_trigger_subsystem_restart;
#endif

	return WIFI_SUCCESS;
}
