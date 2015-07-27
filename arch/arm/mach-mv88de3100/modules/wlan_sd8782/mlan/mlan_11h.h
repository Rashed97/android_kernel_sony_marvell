/** @file mlan_11h.h
 *
 *  @brief This header file contains data structures and
 *  function declarations of 802.11h
 *
 *  (C) Copyright 2008-2011 Marvell International Ltd. All Rights Reserved
 *
 *  MARVELL CONFIDENTIAL
 *  The source code contained or described herein and all documents related to
 *  the source code ("Material") are owned by Marvell International Ltd or its
 *  suppliers or licensors. Title to the Material remains with Marvell International Ltd
 *  or its suppliers and licensors. The Material contains trade secrets and
 *  proprietary and confidential information of Marvell or its suppliers and
 *  licensors. The Material is protected by worldwide copyright and trade secret
 *  laws and treaty provisions. No part of the Material may be used, copied,
 *  reproduced, modified, published, uploaded, posted, transmitted, distributed,
 *  or disclosed in any way without Marvell's prior express written permission.
 *
 *  No license under any patent, copyright, trade secret or other intellectual
 *  property right is granted to or conferred upon you by disclosure or delivery
 *  of the Materials, either expressly, by implication, inducement, estoppel or
 *  otherwise. Any license under such intellectual property rights must be
 *  express and approved by Marvell in writing.
 *
 */

/*************************************************************
Change Log:
    03/26/2009: initial creation
*************************************************************/

#ifndef _MLAN_11H_
#define _MLAN_11H_

/** 11H OID bitmasks */
#define ENABLE_11H_MASK         MBIT(0)

/**
 *  11H APIs
 */

/** Return 1 if 11h is active in the firmware, 0 if it is inactive */
extern t_bool wlan_11h_is_active(mlan_private * priv);

/** Enable the tx interface and record the new transmit state */
extern void wlan_11h_tx_enable(mlan_private * priv);

/** Disable the tx interface and record the new transmit state */
extern void wlan_11h_tx_disable(mlan_private * priv);

/** Activate 11h extensions in the firmware */
extern mlan_status wlan_11h_activate(mlan_private * priv, t_void * pioctl_buf,
                                     t_bool flag);

/** Initialize the 11h device structure */
extern void wlan_11h_init(mlan_adapter * pmadapter);

/** Cleanup for the 11h device structure */
extern void wlan_11h_cleanup(mlan_adapter * pmadapter);

/** Initialize the 11h interface structure */
extern void wlan_11h_priv_init(mlan_private * pmpriv);

/** Get an initial random channel to start an adhoc network on */
extern t_u8 wlan_11h_get_adhoc_start_channel(mlan_private * priv);

/** Check if radar detection is required on the specified channel */
extern t_bool wlan_11h_radar_detect_required(mlan_private * priv, t_u8 channel);

/** Perform a standard availibility check on the specified channel */
extern t_s32 wlan_11h_radar_detected(mlan_private * priv,
                                     pmlan_ioctl_req pioctl_req, t_u8 channel);

/** Check previously issued radar report for a channel */
extern mlan_status wlan_11h_check_chan_report(mlan_private * priv, t_u8 chan);

/** Add any 11h TLVs necessary to complete an adhoc start command */
extern t_s32 wlan_11h_process_start(mlan_private * priv,
                                    t_u8 ** ppbuffer,
                                    IEEEtypes_CapInfo_t * pcap_info,
                                    t_u32 channel,
                                    wlan_11h_bss_info_t * p11h_bss_info);

/** Add any 11h TLVs necessary to complete a join command (adhoc or infra) */
extern t_s32 wlan_11h_process_join(mlan_private * priv,
                                   t_u8 ** ppbuffer,
                                   IEEEtypes_CapInfo_t * pcap_info,
                                   t_u8 band,
                                   t_u32 channel,
                                   wlan_11h_bss_info_t * p11h_bss_info);

/** Complete the firmware command preparation for an 11h command function */
extern mlan_status wlan_11h_cmd_process(mlan_private * priv,
                                        HostCmd_DS_COMMAND * pcmd_ptr,
                                        const t_void * pinfo_buf);

/** Process the response of an 11h firmware command */
extern mlan_status wlan_11h_cmdresp_process(mlan_private * priv,
                                            const HostCmd_DS_COMMAND * resp);

/** Receive IEs from scan processing and record any needed info for 11h */
extern mlan_status wlan_11h_process_bss_elem(mlan_adapter * pmadapter,
                                             wlan_11h_bss_info_t *
                                             p11h_bss_info,
                                             const t_u8 * pelement);

/** Handler for EVENT_CHANNEL_SWITCH_ANN */
extern mlan_status wlan_11h_handle_event_chanswann(mlan_private * priv);

#endif /*_MLAN_11H_ */
