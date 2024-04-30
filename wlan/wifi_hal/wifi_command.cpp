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
#include <netlink/genl/genl.h>

#include "wifi_command.h"

static int ackHandler(struct nl_msg *msg, void *arg)
{
	*((int *)arg) = 0;

	return NL_STOP;
}

static int finishHandler(struct nl_msg *msg, void *arg)
{
	*((int *)arg) = 0;

	return NL_SKIP;
}

static int errorHandler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	*((int *)arg) = 0;

	return NL_SKIP;
}

static int msgHandler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attr[NL80211_ATTR_MAX + 1];
	WifiCommand *cmd = (WifiCommand *)arg;
	struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));

	nla_parse(attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	cmd->parser(attr);

	return NL_SKIP;
}

wifi_error sendMsg(struct nl_sock *sk, struct nl_msg *msg, void *arg)
{
	int err;
	struct nl_cb *cb;

       	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (cb == NULL) {
		ALOGE("%s: alloc cb failed");
		return WIFI_ERROR_OUT_OF_MEMORY;
	}

	nl_cb_err(cb, NL_CB_CUSTOM, errorHandler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finishHandler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ackHandler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, msgHandler, arg);

	err = nl_send_auto_complete(sk, msg);
	while (err > 0) {
		int res = nl_recvmsgs(sk, cb);
		if (res < 0)
			ALOGE("%s: recv msg failed: %d", res);
	}

	nl_cb_put(cb);

	return err ? WIFI_ERROR_UNKNOWN : WIFI_SUCCESS;
}

wifi_error WifiCommand::send()
{
	return sendMsg(sock, nlmsg(), (void *)this);
}

WifiCommand::~WifiCommand()
{
	nlmsg_free(msg);
}

WifiCommand::WifiCommand(struct nl_sock *sk, int family_id, int flags, int nl80211_cmd)
{
	sock = sk;

	msg = nlmsg_alloc();
	if (msg)
		genlmsg_put(msg, 0, 0, family_id, 0, flags, nl80211_cmd, 0);
	else
		ALOGE("nlmsg alloc failed, nl80211 cmd: %d", nl80211_cmd);
}
