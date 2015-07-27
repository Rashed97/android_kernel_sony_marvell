/** @file  mlanevent.c
 *
 *  @brief Program to receive events from the driver/firmware of the uAP
 *         driver.
 *
 *   Usage: mlanevent.exe [-option]
 *
 *  Copyright (C) 2008-2009, Marvell International Ltd.
 *  All Rights Reserved
 */
/****************************************************************************
Change log:
    03/18/08: Initial creation
****************************************************************************/

/****************************************************************************
        Header files
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>

#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/if.h>
#include "mlanevent.h"
#ifdef WFD_SUPPORT
#include <arpa/inet.h>
#endif

/****************************************************************************
        Definitions
****************************************************************************/
/** Enable or disable debug outputs */
#define DEBUG   0

/****************************************************************************
        Global variables
****************************************************************************/
/** Termination flag */
int terminate_flag = 0;

/****************************************************************************
        Local functions
****************************************************************************/
/**
 *  @brief Signal handler
 *
 *  @param sig      Received signal number
 *  @return         N/A
 */
void
sig_handler(int sig)
{
    printf("Stopping application.\n");
#if DEBUG
    printf("Process ID of process killed = %d\n", getpid());
#endif
    terminate_flag = 1;
}

/**
 *  @brief Dump hex data
 *
 *  @param p        A pointer to data buffer
 *  @param len      The len of data buffer
 *  @param delim    Deliminator character
 *  @return         Hex integer
 */
static void
hexdump(void *p, t_s32 len, t_s8 delim)
{
    t_s32 i;
    t_u8 *s = p;
    for (i = 0; i < len; i++) {
        if (i != len - 1)
            printf("%02x%c", *s++, delim);
        else
            printf("%02x\n", *s);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
}

/**
 *  @brief Prints a MAC address in colon separated form from raw data
 *
 *  @param raw      A pointer to the hex data buffer
 *  @return         N/A
 */
void
print_mac(t_u8 * raw)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x", (unsigned int) raw[0],
           (unsigned int) raw[1], (unsigned int) raw[2], (unsigned int) raw[3],
           (unsigned int) raw[4], (unsigned int) raw[5]);
    return;
}

/**
 *  @brief Print usage information
 *
 *  @return         N/A
 */
void
print_usage(void)
{
    printf("\n");
    printf("Usage : mlanevent.exe [-v] [-h]\n");
    printf("    -v               : Print version information\n");
    printf("    -h               : Print help information\n");
    printf("\n");
}

/**
 *  @brief Parse and print STA deauthentication event data
 *
 *  @param buffer   Pointer to received event buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_event_sta_deauth(t_u8 * buffer, t_u16 size)
{
    eventbuf_sta_deauth *event_body = NULL;

    if (size < sizeof(eventbuf_sta_deauth)) {
        printf("ERR:Event buffer too small!\n");
        return;
    }
    event_body = (eventbuf_sta_deauth *) buffer;
    event_body->reason_code = uap_le16_to_cpu(event_body->reason_code);
    printf("EVENT: STA_DEAUTH\n");
    printf("Deauthenticated STA MAC: ");
    print_mac(event_body->sta_mac_address);
    printf("\nReason: ");
    switch (event_body->reason_code) {
    case 1:
        printf("Client station leaving the network\n");
        break;
    case 2:
        printf("Client station aged out\n");
        break;
    case 3:
        printf("Client station deauthenticated by user's request\n");
        break;
    case 4:
        printf("Client station authentication failure\n");
        break;
    case 5:
        printf("Client station association failure\n");
        break;
    case 6:
        printf("Client mac address is blocked by ACL filter\n");
        break;
    case 7:
        printf("Client station table is full\n");
        break;
    case 8:
        printf("Client 4-way handshake timeout\n");
        break;
    case 9:
        printf("Client group key handshake timeout\n");
        break;
    default:
        printf("Unspecified\n");
        break;
    }
    return;
}

/**
 *  @brief Prints mgmt frame
 *
 *  @param mgmt_tlv A pointer to mgmt_tlv
 *  @param tlv_len  Length of tlv payload
 *  @return         N/A
 */
void
print_mgmt_frame(MrvlIETypes_MgmtFrameSet_t * mgmt_tlv, int tlv_len)
{
    IEEEtypes_AssocRqst_t *assoc_req = NULL;
    IEEEtypes_ReAssocRqst_t *reassoc_req = NULL;
    IEEEtypes_AssocRsp_t *assoc_resp = NULL;
    t_u16 frm_ctl = 0;
    printf("\nMgmt Frame:\n");
    memcpy(&frm_ctl, &mgmt_tlv->frame_control, sizeof(t_u16));
    printf("FrameControl: 0x%x\n", frm_ctl);
    if (mgmt_tlv->frame_control.type != 0) {
        printf("Frame type=%d subtype=%d:\n", mgmt_tlv->frame_control.type,
               mgmt_tlv->frame_control.sub_type);
        hexdump(mgmt_tlv->frame_contents, tlv_len - sizeof(t_u16), ' ');
        return;
    }
    switch (mgmt_tlv->frame_control.sub_type) {
    case SUBTYPE_ASSOC_REQUEST:
        printf("Assoc Request:\n");
        assoc_req = (IEEEtypes_AssocRqst_t *) mgmt_tlv->frame_contents;
        printf("CapInfo: 0x%x  ListenInterval: 0x%x \n",
               uap_le16_to_cpu(assoc_req->cap_info),
               uap_le16_to_cpu(assoc_req->listen_interval));
        printf("AssocReqIE:\n");
        hexdump(assoc_req->ie_buffer, tlv_len - sizeof(IEEEtypes_AssocRqst_t)
                - sizeof(IEEEtypes_FrameCtl_t), ' ');
        break;
    case SUBTYPE_REASSOC_REQUEST:
        printf("ReAssoc Request:\n");
        reassoc_req = (IEEEtypes_ReAssocRqst_t *) mgmt_tlv->frame_contents;
        printf("CapInfo: 0x%x  ListenInterval: 0x%x \n",
               uap_le16_to_cpu(reassoc_req->cap_info),
               uap_le16_to_cpu(reassoc_req->listen_interval));
        printf("Current AP address: ");
        print_mac(reassoc_req->current_ap_addr);
        printf("\nReAssocReqIE:\n");
        hexdump(reassoc_req->ie_buffer,
                tlv_len - sizeof(IEEEtypes_ReAssocRqst_t)
                - sizeof(IEEEtypes_FrameCtl_t), ' ');
        break;
    case SUBTYPE_ASSOC_RESPONSE:
    case SUBTYPE_REASSOC_RESPONSE:
        if (mgmt_tlv->frame_control.sub_type == SUBTYPE_ASSOC_RESPONSE)
            printf("Assoc Response:\n");
        else
            printf("ReAssoc Response:\n");
        assoc_resp = (IEEEtypes_AssocRsp_t *) mgmt_tlv->frame_contents;
        printf("CapInfo: 0x%x  StatusCode: %d  AID: 0x%x \n",
               uap_le16_to_cpu(assoc_resp->cap_info),
               (int) (uap_le16_to_cpu(assoc_resp->status_code)),
               uap_le16_to_cpu(assoc_resp->aid) & 0x3fff);
        break;
    default:
        printf("Frame subtype = %d:\n", mgmt_tlv->frame_control.sub_type);
        hexdump(mgmt_tlv->frame_contents, tlv_len - sizeof(t_u16), ' ');
        break;
    }
    return;
}

/**
 *  @brief Parse and print RSN connect event data
 *
 *  @param buffer   Pointer to received buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_event_rsn_connect(t_u8 * buffer, t_u16 size)
{
    int tlv_buf_left = size;
    t_u16 tlv_type, tlv_len;
    tlvbuf_header *tlv = NULL;
    eventbuf_rsn_connect *event_body = NULL;
    if (size < sizeof(eventbuf_rsn_connect)) {
        printf("ERR:Event buffer too small!\n");
        return;
    }
    event_body = (eventbuf_rsn_connect *) buffer;
    printf("EVENT: RSN_CONNECT\n");
    printf("Station MAC: ");
    print_mac(event_body->sta_mac_address);
    printf("\n");
    tlv_buf_left = size - sizeof(eventbuf_rsn_connect);
    if (tlv_buf_left < (int) sizeof(tlvbuf_header))
        return;
    tlv = (tlvbuf_header *) (buffer + sizeof(eventbuf_rsn_connect));

    while (tlv_buf_left >= (int) sizeof(tlvbuf_header)) {
        tlv_type = uap_le16_to_cpu(tlv->type);
        tlv_len = uap_le16_to_cpu(tlv->len);
        if ((sizeof(tlvbuf_header) + tlv_len) > (unsigned int) tlv_buf_left) {
            printf("wrong tlv: tlvLen=%d, tlvBufLeft=%d\n", tlv_len,
                   tlv_buf_left);
            break;
        }
        switch (tlv_type) {
        case IEEE_WPA_IE:
            printf("WPA IE:\n");
            hexdump((t_u8 *) tlv + sizeof(tlvbuf_header), tlv_len, ' ');
            break;
        case IEEE_RSN_IE:
            printf("RSN IE:\n");
            hexdump((t_u8 *) tlv + sizeof(tlvbuf_header), tlv_len, ' ');
            break;
        default:
            printf("unknown tlv: %d\n", tlv_type);
            break;
        }
        tlv_buf_left -= (sizeof(tlvbuf_header) + tlv_len);
        tlv =
            (tlvbuf_header *) ((t_u8 *) tlv + tlv_len + sizeof(tlvbuf_header));
    }
    return;
}

/**
 *  @brief Parse and print STA associate event data
 *
 *  @param buffer   Pointer to received buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_event_sta_assoc(t_u8 * buffer, t_u16 size)
{
    int tlv_buf_left = size;
    t_u16 tlv_type, tlv_len;
    tlvbuf_header *tlv = NULL;
    MrvlIEtypes_WapiInfoSet_t *wapi_tlv = NULL;
    MrvlIETypes_MgmtFrameSet_t *mgmt_tlv = NULL;
    eventbuf_sta_assoc *event_body = NULL;
    if (size < sizeof(eventbuf_sta_assoc)) {
        printf("ERR:Event buffer too small!\n");
        return;
    }
    event_body = (eventbuf_sta_assoc *) buffer;
    printf("EVENT: STA_ASSOCIATE\n");
    printf("Associated STA MAC: ");
    print_mac(event_body->sta_mac_address);
    printf("\n");
    tlv_buf_left = size - sizeof(eventbuf_sta_assoc);
    if (tlv_buf_left < (int) sizeof(tlvbuf_header))
        return;
    tlv = (tlvbuf_header *) (buffer + sizeof(eventbuf_sta_assoc));

    while (tlv_buf_left >= (int) sizeof(tlvbuf_header)) {
        tlv_type = uap_le16_to_cpu(tlv->type);
        tlv_len = uap_le16_to_cpu(tlv->len);
        if ((sizeof(tlvbuf_header) + tlv_len) > (unsigned int) tlv_buf_left) {
            printf("wrong tlv: tlvLen=%d, tlvBufLeft=%d\n", tlv_len,
                   tlv_buf_left);
            break;
        }
        switch (tlv_type) {
        case MRVL_WAPI_INFO_TLV_ID:
            wapi_tlv = (MrvlIEtypes_WapiInfoSet_t *) tlv;
            printf("WAPI Multicast PN:\n");
            hexdump(wapi_tlv->multicast_PN, tlv_len, ' ');
            break;
        case MRVL_MGMT_FRAME_TLV_ID:
            mgmt_tlv = (MrvlIETypes_MgmtFrameSet_t *) tlv;
            print_mgmt_frame(mgmt_tlv, tlv_len);
            break;
        default:
            printf("unknown tlv: %d\n", tlv_type);
            break;
        }
        tlv_buf_left -= (sizeof(tlvbuf_header) + tlv_len);
        tlv =
            (tlvbuf_header *) ((t_u8 *) tlv + tlv_len + sizeof(tlvbuf_header));
    }
    return;
}

/**
 *  @brief Parse and print BSS start event data
 *
 *  @param buffer   Pointer to received buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_event_bss_start(t_u8 * buffer, t_u16 size)
{
    eventbuf_bss_start *event_body = NULL;

    if (size < sizeof(eventbuf_bss_start)) {
        printf("ERR:Event buffer too small!\n");
        return;
    }
    event_body = (eventbuf_bss_start *) buffer;
    printf("EVENT: BSS_START ");
    printf("BSS MAC: ");
    print_mac(event_body->ap_mac_address);
    printf("\n");
    return;
}

#ifdef WFD_SUPPORT
/**
 *  @brief Print WIFI_WPS IE elements from event payload
 *
 *  @param buffer   Pointer to received buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_wifi_wps_ie_elements(t_u8 * buffer, t_u16 size)
{
    t_u8 *ptr = buffer;
    t_u8 *array_ptr;
    int i;
    t_u16 wps_len = 0, wps_type = 0;
    t_u16 ie_len_wps = size;

    while (ie_len_wps > sizeof(tlvbuf_wps_ie)) {
        memcpy(&wps_type, ptr, sizeof(t_u16));
        memcpy(&wps_len, ptr + 2, sizeof(t_u16));
        endian_convert_tlv_wps_header_in(wps_type, wps_len);
        switch (wps_type) {
        case SC_Version:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                printf("\t WPS Version = 0x%2x\n", *(wps_tlv->data));
            }
            break;
        case SC_Simple_Config_State:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                printf("\t WPS setupstate = 0x%x\n", *(wps_tlv->data));
            }
            break;
        case SC_Request_Type:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                printf("\t WPS RequestType = 0x%x\n", *(wps_tlv->data));
            }
            break;
        case SC_Response_Type:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                printf("\t WPS ResponseType = 0x%x\n", *(wps_tlv->data));
            }
            break;
        case SC_Config_Methods:
            {
                t_u16 wps_config_methods = 0;
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                memcpy(&wps_config_methods, wps_tlv->data, sizeof(t_u16));
                wps_config_methods = ntohs(wps_config_methods);
                printf("\t WPS SpecConfigMethods = 0x%x\n", wps_config_methods);
            }
            break;
        case SC_UUID_E:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                array_ptr = wps_tlv->data;
                printf("\t WPS UUID = ");
                for (i = 0; i < wps_len; i++)
                    printf("0x%02X ", *array_ptr++);
                printf("\n");
            }
            break;
        case SC_Primary_Device_Type:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                array_ptr = wps_tlv->data;
                printf("\t WPS Primary Device Type = ");
                for (i = 0; i < wps_len; i++)
                    printf("0x%02X ", *array_ptr++);
                printf("\n");
            }
            break;
        case SC_RF_Band:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                printf("\t WPS RF Band = 0x%x\n", *(wps_tlv->data));
            }
            break;
        case SC_Association_State:
            {
                t_u16 wps_association_state = 0;
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                memcpy(&wps_association_state, wps_tlv->data, sizeof(t_u16));
                wps_association_state = ntohs(wps_association_state);
                printf("\t WPS Association State = 0x%x\n",
                       wps_association_state);
            }
            break;
        case SC_Configuration_Error:
            {
                t_u16 wps_configuration_error = 0;
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                memcpy(&wps_configuration_error, wps_tlv->data, sizeof(t_u16));
                wps_configuration_error = ntohs(wps_configuration_error);
                printf("\t WPS Configuration Error = 0x%x\n",
                       wps_configuration_error);
            }
            break;
        case SC_Device_Password_ID:
            {
                t_u16 wps_device_password_id = 0;
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                memcpy(&wps_device_password_id, wps_tlv->data, sizeof(t_u16));
                wps_device_password_id = ntohs(wps_device_password_id);
                printf("\t WPS Device Password ID = 0x%x\n",
                       wps_device_password_id);
            }
            break;
        case SC_Device_Name:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                array_ptr = wps_tlv->data;
                printf("\t WPS Device Name = ");
                for (i = 0; i < wps_len; i++)
                    printf("%c", *array_ptr++);
                printf("\n");
            }
            break;
        case SC_Manufacturer:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                array_ptr = wps_tlv->data;
                printf("\t WPS Manufacturer = ");
                for (i = 0; i < wps_len; i++)
                    printf("%c", *array_ptr++);
                printf("\n");
            }
            break;
        case SC_Model_Name:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                array_ptr = wps_tlv->data;
                printf("\t WPS Model Name = ");
                for (i = 0; i < wps_len; i++)
                    printf("%c", *array_ptr++);
                printf("\n");
            }
            break;
        case SC_Model_Number:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                array_ptr = wps_tlv->data;
                printf("\t WPS Model Number = ");
                for (i = 0; i < wps_len; i++)
                    printf("%d", *array_ptr++);
                printf("\n");
            }
            break;
        case SC_Serial_Number:
            {
                tlvbuf_wps_ie *wps_tlv = (tlvbuf_wps_ie *) ptr;
                array_ptr = wps_tlv->data;
                printf("\t WPS Serial Number = ");
                for (i = 0; i < wps_len; i++)
                    printf("%d", *array_ptr++);
                printf("\n");
            }
            break;
        default:
            printf("unknown ie=0x%x, len=%d\n", wps_type, wps_len);
            break;
        }
        ptr += wps_len + sizeof(tlvbuf_wps_ie);
        /* Take care of error condition */
        if (wps_len + sizeof(tlvbuf_wps_ie) <= ie_len_wps)
            ie_len_wps -= wps_len + sizeof(tlvbuf_wps_ie);
        else
            ie_len_wps = 0;
    }
}

/**
 *  @brief Print WFD IE elements from event payload
 *
 *  @param buffer   Pointer to received buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_wfd_ie_elements(t_u8 * buffer, t_u16 size)
{
    t_u8 *ptr = buffer;
    t_u8 *array_ptr, *orig_ptr = NULL;
    int i;
    static t_u16 len = 0;
    static t_u16 saved_len = 0;
    static t_u16 pending_len = 0;
    static t_u8 type = 0;
    static t_u8 next_byte = WFD_OVERLAP_TYPE;
    static t_u8 saved_data[WIFI_IE_MAX_PAYLOAD] = { 0 };
    t_u16 temp;
    t_u16 left_len = size;

    while (left_len > 0) {
        if (next_byte == WFD_OVERLAP_TYPE) {
            type = *ptr;
            next_byte = WFD_OVERLAP_LEN;
            left_len--;
            ptr++;
        }
        if (left_len >= sizeof(len) && next_byte == WFD_OVERLAP_LEN) {
            memcpy(&len, ptr, sizeof(t_u16));
            len = uap_le16_to_cpu(len);
            next_byte = WFD_OVERLAP_DATA;
            left_len -= sizeof(t_u16);
            ptr += sizeof(t_u16);

            /* case when Type, Len in one frame and data in next */
            if (left_len == 0) {
                memcpy(saved_data, ptr - WFD_IE_HEADER_LEN, WFD_IE_HEADER_LEN);
                saved_len = WFD_IE_HEADER_LEN;
                pending_len = len;
            }
        }
        if (left_len > 0 && next_byte == WFD_OVERLAP_DATA) {
            /* copy next data */
            if (pending_len > 0 &&
                (left_len <= (WIFI_IE_MAX_PAYLOAD - saved_len))) {
                memcpy(saved_data + saved_len, ptr, pending_len);
                orig_ptr = ptr;
                ptr = saved_data;
            } else {
                ptr -= WFD_IE_HEADER_LEN;
            }

            if (!pending_len && !orig_ptr && left_len < len) {
                /* save along with type and len */
                memcpy(saved_data, ptr - WFD_IE_HEADER_LEN,
                       left_len + WFD_IE_HEADER_LEN);
                saved_len = left_len + WFD_IE_HEADER_LEN;
                pending_len = len - left_len;
                break;
            }
            switch (type) {
            case TLV_TYPE_WFD_DEVICE_ID:
                {
                    tlvbuf_wfd_device_id *wfd_tlv =
                        (tlvbuf_wfd_device_id *) ptr;
                    printf("\t Device ID - ");
                    print_mac(wfd_tlv->dev_mac_address);
                    printf("\n");
                }
                break;
            case TLV_TYPE_WFD_CAPABILITY:
                {
                    tlvbuf_wfd_capability *wfd_tlv =
                        (tlvbuf_wfd_capability *) ptr;
                    printf("\t Device capability = %d\n",
                           (int) wfd_tlv->dev_capability);
                    printf("\t Group capability = %d\n",
                           (int) wfd_tlv->group_capability);
                }
                break;
            case TLV_TYPE_WFD_GROUPOWNER_INTENT:
                {
                    tlvbuf_wfd_group_owner_intent *wfd_tlv =
                        (tlvbuf_wfd_group_owner_intent *) ptr;
                    printf("\t Group owner intent = %d\n",
                           (int) wfd_tlv->dev_intent);
                }
                break;
            case TLV_TYPE_WFD_MANAGEABILITY:
                {
                    tlvbuf_wfd_manageability *wfd_tlv =
                        (tlvbuf_wfd_manageability *) ptr;
                    printf("\t Manageability = %d\n",
                           (int) wfd_tlv->manageability);
                }
                break;
            case TLV_TYPE_WFD_INVITATION_FLAG:
                {
                    tlvbuf_wfd_invitation_flag *wfd_tlv =
                        (tlvbuf_wfd_invitation_flag *) ptr;
                    printf("\t Invitation Flag = %d\n",
                           (int) wfd_tlv->
                           invitation_flag & INVITATION_FLAG_MASK);
                }
                break;
            case TLV_TYPE_WFD_CHANNEL_LIST:
                {
                    tlvbuf_wfd_channel_list *wfd_tlv =
                        (tlvbuf_wfd_channel_list *) ptr;
                    chan_entry *temp_ptr;
                    printf("\t Country String %c%c", wfd_tlv->country_string[0],
                           wfd_tlv->country_string[1]);
                    if (isalpha(wfd_tlv->country_string[2]))
                        printf("%c", wfd_tlv->country_string[2]);
                    printf("\n");
                    temp_ptr = (chan_entry *) wfd_tlv->wfd_chan_entry_list;
                    temp = uap_le16_to_cpu(wfd_tlv->length) -
                        (sizeof(tlvbuf_wfd_channel_list) - WFD_IE_HEADER_LEN);
                    while (temp) {
                        printf("\t Regulatory_class = %d\n",
                               (int) (temp_ptr->regulatory_class));
                        printf("\t No of channels = %d\n",
                               (int) temp_ptr->num_of_channels);
                        printf("\t Channel list = ");
                        for (i = 0; i < temp_ptr->num_of_channels; i++) {
                            printf("%d ", *(temp_ptr->chan_list + i));
                        }
                        printf("\n");
                        temp -= sizeof(chan_entry) + temp_ptr->num_of_channels;
                        temp_ptr +=
                            sizeof(chan_entry) + temp_ptr->num_of_channels;
                    }
                    printf("\n");
                }
                break;
            case TLV_TYPE_WFD_NOTICE_OF_ABSENCE:
                {
                    tlvbuf_wfd_notice_of_absence *wfd_tlv =
                        (tlvbuf_wfd_notice_of_absence *) ptr;
                    noa_descriptor *temp_ptr;
                    printf("\t Instance of Notice of absence timing %d\n",
                           (int) wfd_tlv->noa_index);
                    printf
                        ("\t CTWindow and Opportunistic power save parameters %d\n",
                         (int) wfd_tlv->ctwindow_opp_ps);
                    temp_ptr =
                        (noa_descriptor *) wfd_tlv->wfd_noa_descriptor_list;
                    temp =
                        uap_le16_to_cpu(wfd_tlv->length) -
                        (sizeof(tlvbuf_wfd_notice_of_absence) -
                         WFD_IE_HEADER_LEN);
                    while (temp) {
                        printf("\t Count or Type = %d\n",
                               (int) temp_ptr->count_type);
                        printf("\t Duration = %ldms\n",
                               uap_le32_to_cpu(temp_ptr->duration));
                        printf("\t Interval = %ldms\n",
                               uap_le32_to_cpu(temp_ptr->interval));
                        printf("\t Start Time = %ld\n",
                               uap_le32_to_cpu(temp_ptr->start_time));
                        printf("\n");
                        temp_ptr += sizeof(noa_descriptor);
                        temp -= sizeof(noa_descriptor);
                    }
                }
                break;
            case TLV_TYPE_WFD_DEVICE_INFO:
                {
                    tlvbuf_wfd_device_info *wfd_tlv =
                        (tlvbuf_wfd_device_info *) ptr;
                    printf("\t Device address - ");
                    print_mac(wfd_tlv->dev_address);
                    printf("\n");
                    printf("\t Config methods - 0x%02X\n",
                           ntohs(wfd_tlv->config_methods));
                    printf
                        ("\t Primay device type = %02d-%02X%02X%02X%02X-%02d\n",
                         (int) ntohs(wfd_tlv->primary_category),
                         (int) wfd_tlv->primary_oui[0],
                         (int) wfd_tlv->primary_oui[1],
                         (int) wfd_tlv->primary_oui[2],
                         (int) wfd_tlv->primary_oui[3],
                         (int) ntohs(wfd_tlv->primary_subcategory));
                    printf("\t Secondary Device Count = %d\n",
                           (int) wfd_tlv->secondary_dev_count);
                    array_ptr = wfd_tlv->secondary_dev_info;
                    for (i = 0; i < wfd_tlv->secondary_dev_count; i++) {
                        memcpy(&temp, array_ptr, sizeof(t_u16));
                        printf("\t Secondary device type = %02d-", ntohs(temp));
                        array_ptr += sizeof(temp);
                        printf("%02X%02X%02X%02X", array_ptr[0],
                               array_ptr[1], array_ptr[2], array_ptr[3]);
                        array_ptr += 4;
                        memcpy(&temp, array_ptr, sizeof(t_u16));
                        printf("-%02d\n", ntohs(temp));
                        array_ptr += sizeof(temp);
                    }
                    /* display device name */
                    array_ptr = wfd_tlv->device_name +
                        wfd_tlv->secondary_dev_count * WPS_DEVICE_TYPE_LEN;
                    if (*(t_u16 *) (((t_u8 *) (&wfd_tlv->device_name_len)) +
                                    wfd_tlv->secondary_dev_count *
                                    WPS_DEVICE_TYPE_LEN))
                        printf("\t Device Name =  ");
                    memcpy(&temp, (((t_u8 *) (&wfd_tlv->device_name_len)) +
                                   wfd_tlv->secondary_dev_count *
                                   WPS_DEVICE_TYPE_LEN), sizeof(t_u16));
                    temp = ntohs(temp);
                    for (i = 0; i < temp; i++)
                        printf("%c", *array_ptr++);
                    printf("\n");
                }
                break;
            case TLV_TYPE_WFD_GROUP_INFO:
                {
                    tlvbuf_wfd_group_info *wfd_tlv =
                        (tlvbuf_wfd_group_info *) ptr;
                    t_u8 wfd_client_dev_length;
                    wfd_client_dev_info *temp_ptr;
                    temp_ptr =
                        (wfd_client_dev_info *) wfd_tlv->wfd_client_dev_list;
                    wfd_client_dev_length = temp_ptr->dev_length;
                    temp =
                        uap_le16_to_cpu(wfd_tlv->length) -
                        wfd_client_dev_length;
                    while (temp) {

                        printf("\t Group Wfd Client Device address - ");
                        print_mac(temp_ptr->wfd_dev_address);
                        printf("\n");
                        printf("\t Group Wfd Client Interface address - ");
                        print_mac(temp_ptr->wfd_intf_address);
                        printf("\n");
                        printf("\t Group Wfd Client Device capability = %d\n",
                               (int) temp_ptr->wfd_dev_capability);
                        printf("\t Group Wfd Client Config methods - 0x%02X\n",
                               ntohs(temp_ptr->config_methods));
                        printf
                            ("\t Group Wfd Client Primay device type = %02d-%02X%02X%02X%02X-%02d\n",
                             (int) ntohs(temp_ptr->primary_category),
                             (int) temp_ptr->primary_oui[0],
                             (int) temp_ptr->primary_oui[1],
                             (int) temp_ptr->primary_oui[2],
                             (int) temp_ptr->primary_oui[3],
                             (int) ntohs(temp_ptr->primary_subcategory));
                        printf
                            ("\t Group Wfd Client Secondary Device Count = %d\n",
                             (int) temp_ptr->wfd_secondary_dev_count);
                        array_ptr = temp_ptr->wfd_secondary_dev_info;
                        for (i = 0; i < temp_ptr->wfd_secondary_dev_count; i++) {
                            memcpy(&temp, array_ptr, sizeof(t_u16));
                            printf
                                ("\t Group Wfd Client Secondary device type = %02d-",
                                 ntohs(temp));
                            array_ptr += sizeof(temp);
                            printf("%02X%02X%02X%02X", array_ptr[0],
                                   array_ptr[1], array_ptr[2], array_ptr[3]);
                            array_ptr += 4;
                            memcpy(&temp, array_ptr, sizeof(t_u16));
                            printf("-%02d\n", ntohs(temp));
                            array_ptr += sizeof(temp);
                        }
                        /* display device name */
                        array_ptr = temp_ptr->wfd_device_name +
                            temp_ptr->wfd_secondary_dev_count *
                            WPS_DEVICE_TYPE_LEN;
                        printf("\t Group Wfd Device Name =  ");
                        memcpy(&temp,
                               (((t_u8 *) (&temp_ptr->wfd_device_name_len)) +
                                temp_ptr->wfd_secondary_dev_count *
                                WPS_DEVICE_TYPE_LEN), sizeof(t_u16));
                        temp = ntohs(temp);
                        for (i = 0; i < temp; i++)
                            printf("%c", *array_ptr++);
                        printf("\n");
                        temp_ptr += wfd_client_dev_length;
                        temp -= wfd_client_dev_length;
                        if (temp_ptr)
                            wfd_client_dev_length = temp_ptr->dev_length;
                    }
                    printf("\n");
                }
                break;
            case TLV_TYPE_WFD_GROUP_ID:
                {
                    tlvbuf_wfd_group_id *wfd_tlv = (tlvbuf_wfd_group_id *) ptr;
                    printf("\t Group address - ");
                    print_mac(wfd_tlv->group_address);
                    printf("\n");
                    printf("\t Group ssid =  ");
                    for (i = 0;
                         (unsigned int) i < uap_le16_to_cpu(wfd_tlv->length)
                         - (sizeof(tlvbuf_wfd_group_id) - WFD_IE_HEADER_LEN);
                         i++)
                        printf("%c", wfd_tlv->group_ssid[i]);
                    printf("\n");
                }
                break;
            case TLV_TYPE_WFD_GROUP_BSS_ID:
                {
                    tlvbuf_wfd_group_bss_id *wfd_tlv =
                        (tlvbuf_wfd_group_bss_id *) ptr;
                    printf("\t Group BSS Id - ");
                    print_mac(wfd_tlv->group_bssid);
                    printf("\n");
                }
                break;
            case TLV_TYPE_WFD_INTERFACE:
                {
                    tlvbuf_wfd_interface *wfd_tlv =
                        (tlvbuf_wfd_interface *) ptr;
                    printf("\t Interface Id - ");
                    print_mac(wfd_tlv->interface_id);
                    printf("\t Interface count = %d",
                           (int) wfd_tlv->interface_count);
                    for (i = 0; i < wfd_tlv->interface_count; i++) {
                        printf("\n\t Interface address [%d]", i + 1);
                        print_mac(&wfd_tlv->interface_idlist[i * ETH_ALEN]);
                    }
                    printf("\n");
                }
                break;
            case TLV_TYPE_WFD_CHANNEL:
                {
                    tlvbuf_wfd_channel *wfd_tlv = (tlvbuf_wfd_channel *) ptr;
                    printf("\t Listen Channel Country String %c%c",
                           wfd_tlv->country_string[0],
                           wfd_tlv->country_string[1]);
                    if (isalpha(wfd_tlv->country_string[2]))
                        printf("%c", wfd_tlv->country_string[2]);
                    printf("\n");
                    printf("\t Listen Channel regulatory class = %d\n",
                           (int) wfd_tlv->regulatory_class);
                    printf("\t Listen Channel number = %d\n",
                           (int) wfd_tlv->channel_number);
                }
                break;

            case TLV_TYPE_WFD_OPCHANNEL:
                {
                    tlvbuf_wfd_channel *wfd_tlv = (tlvbuf_wfd_channel *) ptr;
                    printf("\t Operating Channel Country String %c%c",
                           wfd_tlv->country_string[0],
                           wfd_tlv->country_string[1]);
                    if (isalpha(wfd_tlv->country_string[2]))
                        printf("%c", wfd_tlv->country_string[2]);
                    printf("\n");
                    printf("\t Operating Channel regulatory class = %d\n",
                           (int) wfd_tlv->regulatory_class);
                    printf("\t Operating Channel number = %d\n",
                           (int) wfd_tlv->channel_number);
                }
                break;

            case TLV_TYPE_WFD_CONFIG_TIMEOUT:
                {
                    tlvbuf_wfd_config_timeout *wfd_tlv =
                        (tlvbuf_wfd_config_timeout *) ptr;
                    printf("\t GO configuration timeout = %d msec\n",
                           (int) wfd_tlv->group_config_timeout * 10);
                    printf("\t Client configuration timeout = %d msec\n",
                           (int) wfd_tlv->device_config_timeout * 10);
                }
                break;
            case TLV_TYPE_WFD_EXTENDED_LISTEN_TIME:
                {
                    tlvbuf_wfd_ext_listen_time *wfd_tlv =
                        (tlvbuf_wfd_ext_listen_time *) ptr;
                    printf("\t Availability Period = %d msec\n",
                           (int) wfd_tlv->availability_period);
                    printf("\t Availability Interval = %d msec\n",
                           (int) wfd_tlv->availability_interval);
                }
                break;
            case TLV_TYPE_WFD_INTENDED_ADDRESS:
                {
                    tlvbuf_wfd_intended_addr *wfd_tlv =
                        (tlvbuf_wfd_intended_addr *) ptr;
                    printf("\t Intended Interface Address - ");
                    print_mac(wfd_tlv->group_address);
                    printf("\n");
                }
                break;

            case TLV_TYPE_WFD_STATUS:
                {
                    tlvbuf_wfd_status *wfd_tlv = (tlvbuf_wfd_status *) ptr;
                    printf("\t Status = %d\n", wfd_tlv->status_code);
                }
                break;

            default:
                printf("unknown ie=0x%x, len=%d\n", type, len);
                break;
            }
            next_byte = WFD_OVERLAP_TYPE;
            if (orig_ptr)
                ptr = orig_ptr + pending_len;
        }
        ptr += len + WFD_IE_HEADER_LEN;
        left_len -= len;
    }
    printf("\n");
    return;
}

/**
 *  @brief Parse and print WFD generic event data
 *
 *  @param buffer   Pointer to received buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_event_wfd_generic(t_u8 * buffer, t_u16 size)
{
    const t_u8 wifi_oui[3] = { 0x50, 0x6F, 0x9A };
    const t_u8 wps_oui[3] = { 0x00, 0x50, 0xF2 };
    apeventbuf_wfd_generic *wfd_event;
    wfd_ie_header *wfd_wps_header;
    t_u16 wfd_wps_len = 0;
    printf("EVENT: WFD \n");
    wfd_event = (apeventbuf_wfd_generic *) (buffer);
    printf("Event length = %d\n", uap_le16_to_cpu(wfd_event->event_length));
    printf("Event Type = ");
    switch (uap_le16_to_cpu(wfd_event->event_type)) {
    case 0:
        printf("Negotiation Request\n");
        break;
    case 1:
        printf("Negotiation Response\n");
        break;
    case 2:
        printf("Negotiation Result\n");
        break;
    case 3:
        printf("Invitation Request\n");
        break;
    case 4:
        printf("Invitation Response\n");
        break;
    case 5:
        printf("Discoverability Request\n");
        break;
    case 6:
        printf("Discoverability Response\n");
        break;
    case 7:
        printf("Provision Discovery Request\n");
        break;
    case 8:
        printf("Provision Discovery Response\n");
        break;
    case 14:
        printf("Peer Detected event\n");
        break;
    case 15:
        printf("Client associated event\n");
        break;
    case 16:
        printf("FW debug event: %s\n", wfd_event->entire_ie_list);
        return;
    default:
        printf("Unknown\n");
        break;
    }
    switch (uap_le16_to_cpu(wfd_event->event_sub_type)) {
    case 0:
        break;
    case 1:
        printf("Event SubType = Group Owner Role\n");
        break;
    case 2:
        printf("Event SubType = Client Role\n");
        break;
    default:
        printf("Event SubType = Unknown\n");
        break;
    }
    printf("Peer Mac Address - ");
    print_mac(wfd_event->peer_mac_addr);
    printf("\n");
    /* Print rest of IE elements */
    wfd_wps_header = (wfd_ie_header *) (wfd_event->entire_ie_list);
    wfd_wps_len = uap_le16_to_cpu(wfd_event->event_length)
        - sizeof(apeventbuf_wfd_generic);

    while (wfd_wps_len >= sizeof(wfd_ie_header)) {
        if (!memcmp(wfd_wps_header->oui, wifi_oui, sizeof(wifi_oui)) ||
            !(memcmp(wfd_wps_header->oui, wps_oui, sizeof(wps_oui)))) {
            switch (wfd_wps_header->oui_type) {
            case WFD_OUI_TYPE:
                print_wfd_ie_elements(wfd_wps_header->ie_list,
                                      wfd_wps_header->ie_length -
                                      sizeof(wfd_wps_header->oui)
                                      - sizeof(wfd_wps_header->oui_type));
                printf("\n");
                break;
            case WIFI_WPS_OUI_TYPE:
                print_wifi_wps_ie_elements(wfd_wps_header->ie_list,
                                           wfd_wps_header->ie_length -
                                           sizeof(wfd_wps_header->oui)
                                           - sizeof(wfd_wps_header->oui_type));
                printf("\n");
                break;
            }
        }
        wfd_wps_len -= wfd_wps_header->ie_length + IE_HEADER_LEN;
        wfd_wps_header = (wfd_ie_header *) (((t_u8 *) wfd_wps_header) +
                                            wfd_wps_header->ie_length +
                                            IE_HEADER_LEN);
    }
}

/**
 *  @brief Parse and print WFD service discovery event data
 *
 *  @param buffer   Pointer to received buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_event_wfd_service_discovery(t_u8 * buffer, t_u16 size)
{
    unsigned int i;
    t_u16 event_len = 0;
    t_u16 dns_len = 0, dns_type;
    t_u8 action = 0;            /* req = 0, resp = 1 */
    apeventbuf_wfd_discovery_request *wfd_disc_req;
    apeventbuf_wfd_discovery_response *wfd_disc_resp;
    printf("EVENT: WFD SERVICE DISCOVERY\n");
    memcpy(&event_len, buffer, sizeof(t_u16));
    event_len = uap_le16_to_cpu(event_len);
    printf("Event length = %d\n", event_len);
    printf("Service Discovery packet:\n");
    /* check request /response Byte at offset 2+6+2 */
    action = *(buffer + sizeof(t_u16) + ETH_ALEN + sizeof(t_u8));
    if (action == WFD_DISCOVERY_REQUEST_ACTION) {
        wfd_disc_req =
            (apeventbuf_wfd_discovery_request *) (buffer + sizeof(t_u16));
        printf("\t Peer Mac Address - ");
        print_mac(wfd_disc_req->peer_mac_addr);
        printf("\n\t Category = %d\n", wfd_disc_req->category);
        printf("\t Action = %d\n", wfd_disc_req->action);
        printf("\t Dialog taken = %d\n", wfd_disc_req->dialog_taken);
        printf("\t Advertize protocol IE - 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
               wfd_disc_req->advertize_protocol_ie[0],
               wfd_disc_req->advertize_protocol_ie[1],
               wfd_disc_req->advertize_protocol_ie[2],
               wfd_disc_req->advertize_protocol_ie[3]);
        printf("\t Request query length = %d\n",
               uap_le16_to_cpu(wfd_disc_req->query_len));
        printf("\t Information Id - 0x%02x, 0x%02x\n", wfd_disc_req->info_id[0],
               wfd_disc_req->info_id[1]);
        printf("\t Request length = %d\n",
               uap_le16_to_cpu(wfd_disc_req->request_len));
        printf("\t OUI - 0x%02x, 0x%02x, 0x%02x\n", wfd_disc_req->oui[0],
               wfd_disc_req->oui[1], wfd_disc_req->oui[2]);
        printf("\t OUI sub type = %d\n", wfd_disc_req->oui_sub_type);
        printf("\t Service update Indicator = %d\n",
               uap_le16_to_cpu(wfd_disc_req->service_update_indicator));
        printf("\t Vendor length = %d\n",
               uap_le16_to_cpu(wfd_disc_req->vendor_len));
        printf("\t Service protocol = %d\n", wfd_disc_req->service_protocol);
        printf("\t Service transaction Id = %d\n",
               wfd_disc_req->service_transaction_id);
        printf("\t Query Data = ");
        if (wfd_disc_req->service_protocol == 1) {
            printf(" * Bonjour * \n");
            printf("\t\t DNS = ");
            dns_len = uap_le16_to_cpu(wfd_disc_req->vendor_len) -
                WFD_DISCOVERY_BONJOUR_FIXED_LEN;
            for (i = 0; i < dns_len; i++)
                printf("%02x ",
                       (int) *(wfd_disc_req->disc_query.u.bonjour.dns + i));
            memcpy(&dns_type,
                   (&wfd_disc_req->disc_query.u.bonjour.dns_type + dns_len),
                   sizeof(dns_type));
            dns_type = uap_le16_to_cpu(dns_type);
            printf("\n\t\t DNS Type = %d\n", dns_type);
            printf("\t\t Version = %d\n",
                   *(&wfd_disc_req->disc_query.u.bonjour.version + dns_len));
        } else if (wfd_disc_req->service_protocol == 2) {
            printf(" * uPnP * \n");
            printf("\t\t Version = %d\n",
                   wfd_disc_req->disc_query.u.upnp.version);
            dns_len =
                uap_le16_to_cpu(wfd_disc_req->vendor_len) -
                WFD_DISCOVERY_UPNP_FIXED_LEN;
            printf("\t\t Value = ");
            for (i = 0; i < dns_len; i++)
                printf("%02x ",
                       (int) *(wfd_disc_req->disc_query.u.upnp.value + i));
        }
        printf("\n");
    } else if (action == WFD_DISCOVERY_RESPONSE_ACTION) {
        wfd_disc_resp =
            (apeventbuf_wfd_discovery_response *) (buffer + sizeof(t_u16));
        printf("\t Peer Mac Address - ");
        print_mac(wfd_disc_resp->peer_mac_addr);
        printf("\n\t Category = %d\n", wfd_disc_resp->category);
        printf("\t Action = %d\n", wfd_disc_resp->action);
        printf("\t Dialog taken = %d\n", wfd_disc_resp->dialog_taken);
        printf("\t Status code = %d\n", wfd_disc_resp->status_code);
        printf("\t GAS reply - 0x%02x\n",
               uap_le16_to_cpu(wfd_disc_resp->gas_reply));
        printf("\t Advertize protocol IE - 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
               wfd_disc_resp->advertize_protocol_ie[0],
               wfd_disc_resp->advertize_protocol_ie[1],
               wfd_disc_resp->advertize_protocol_ie[2],
               wfd_disc_resp->advertize_protocol_ie[3]);
        printf("\t Response query length = %d\n",
               uap_le16_to_cpu(wfd_disc_resp->query_len));
        printf("\t Information Id - 0x%02x, 0x%02x\n",
               wfd_disc_resp->info_id[0], wfd_disc_resp->info_id[1]);
        printf("\t Response length = %d\n",
               uap_le16_to_cpu(wfd_disc_resp->response_len));
        printf("\t OUI - 0x%02x, 0x%02x, 0x%02x\n", wfd_disc_resp->oui[0],
               wfd_disc_resp->oui[1], wfd_disc_resp->oui[2]);
        printf("\t OUI sub type = %d\n", wfd_disc_resp->oui_sub_type);
        printf("\t Service update Indicator = %d\n",
               uap_le16_to_cpu(wfd_disc_resp->service_update_indicator));
        printf("\t Vendor length = %d\n",
               uap_le16_to_cpu(wfd_disc_resp->vendor_len));
        printf("\t Service protocol = %d\n", wfd_disc_resp->service_protocol);
        printf("\t Service transaction Id = %d\n",
               wfd_disc_resp->service_transaction_id);
        printf("\t Status Code = %d\n", wfd_disc_resp->disc_status_code);
        printf("\t Response Data = ");
        if (wfd_disc_resp->service_protocol == 1) {
            printf(" * Bonjour * \n");
#if 0
            printf("\t\t DNS = ");
            dns_len = uap_le16_to_cpu(wfd_disc_resp->vendor_len) -
                (WFD_DISCOVERY_BONJOUR_FIXED_LEN + 1);
            for (i = 0; i < dns_len; i++)
                printf("%c", *(wfd_disc_resp->disc_resp.u.bonjour.dns + i));
            memcpy(&dns_type, (&wfd_disc_req->disc_query.u.bonjour.dns_type
                               + dns_len), sizeof(dns_type));
            dns_type = uap_le16_to_cpu(dns_type);
            printf("\n\t\t DNS Type = %d\n", dns_type);
            printf("\t\t Version = %d\n",
                   *(&wfd_disc_resp->disc_resp.u.bonjour.version + dns_len));
#endif
        } else if (wfd_disc_resp->service_protocol == 2) {
            printf(" * uPnP * \n");
            printf("\t\t Version = %d\n",
                   wfd_disc_resp->disc_resp.u.upnp.version);
            dns_len =
                uap_le16_to_cpu(wfd_disc_resp->vendor_len) -
                WFD_DISCOVERY_UPNP_FIXED_LEN;
            printf("\t\t Value = ");
            for (i = 0; i < dns_len; i++)
                printf("%02x ",
                       (int) *(wfd_disc_resp->disc_resp.u.upnp.value + i));
        }
        printf("\n");
    }
}
#endif

/**
 *  @brief Prints station reject state
 *
 *  @param state    Fail state
 *  @return         N/A
 */
void
print_reject_state(t_u8 state)
{
    switch (state) {
    case REJECT_STATE_FAIL_EAPOL_2:
        printf("Reject state: FAIL_EAPOL_2\n");
        break;
    case REJECT_STATE_FAIL_EAPOL_4:
        printf("Reject state: FAIL_EAPOL_4:\n");
        break;
    case REJECT_STATE_FAIL_EAPOL_GROUP_2:
        printf("Reject state: FAIL_EAPOL_GROUP_2\n");
        break;
    default:
        printf("ERR: unknown reject state %d\n", state);
        break;
    }
    return;
}

/**
 *  @brief Prints station reject reason
 *
 *  @param reason   Reason code
 *  @return         N/A
 */
void
print_reject_reason(t_u16 reason)
{
    switch (reason) {
    case IEEEtypes_REASON_INVALID_IE:
        printf("Reject reason: Invalid IE\n");
        break;
    case IEEEtypes_REASON_MIC_FAILURE:
        printf("Reject reason: Mic Failure\n");
        break;
    default:
        printf("Reject reason: %d\n", reason);
        break;
    }
    return;
}

/**
 *  @brief Prints EAPOL state
 *
 *  @param state    Eapol state
 *  @return         N/A
 */
void
print_eapol_state(t_u8 state)
{
    switch (state) {
    case EAPOL_START:
        printf("Eapol state: EAPOL_START\n");
        break;
    case EAPOL_WAIT_PWK2:
        printf("Eapol state: EAPOL_WAIT_PWK2\n");
        break;
    case EAPOL_WAIT_PWK4:
        printf("Eapol state: EAPOL_WAIT_PWK4\n");
        break;
    case EAPOL_WAIT_GTK2:
        printf("Eapol state: EAPOL_WAIT_GTK2\n");
        break;
    case EAPOL_END:
        printf("Eapol state: EAPOL_END\n");
        break;
    default:
        printf("ERR: unknow eapol state%d\n", state);
        break;
    }
    return;
}

/**
 *  @brief Parse and print debug event data
 *
 *  @param buffer   Pointer to received buffer
 *  @param size     Length of the received event data
 *  @return         N/A
 */
void
print_event_debug(t_u8 * buffer, t_u16 size)
{
    eventbuf_debug *event_body = NULL;
    if (size < sizeof(eventbuf_debug)) {
        printf("ERR:Event buffer too small!\n");
        return;
    }
    event_body = (eventbuf_debug *) buffer;
    printf("Debug Event Type: %s\n",
           (event_body->debug_type == 0) ? "EVENT" : "INFO");
    printf("%s log:\n",
           (uap_le32_to_cpu(event_body->debug_id_major) ==
            DEBUG_ID_MAJ_AUTHENTICATOR) ? "Authenticator" : "Assoc_agent");
    if (uap_le32_to_cpu(event_body->debug_id_major) ==
        DEBUG_ID_MAJ_AUTHENTICATOR) {
        switch (uap_le32_to_cpu(event_body->debug_id_minor)) {
        case DEBUG_MAJ_AUTH_MIN_PWK1:
            printf("EAPOL Key message 1 (PWK):\n");
            hexdump((t_u8 *) & event_body->info.eapol_pwkmsg,
                    sizeof(eapol_keymsg_debug_t), ' ');
            break;
        case DEBUG_MAJ_AUTH_MIN_PWK2:
            printf("EAPOL Key message 2 (PWK):\n");
            hexdump((t_u8 *) & event_body->info.eapol_pwkmsg,
                    sizeof(eapol_keymsg_debug_t), ' ');
            break;
        case DEBUG_MAJ_AUTH_MIN_PWK3:
            printf("EAPOL Key message 3 (PWK):\n");
            hexdump((t_u8 *) & event_body->info.eapol_pwkmsg,
                    sizeof(eapol_keymsg_debug_t), ' ');
            break;
        case DEBUG_MAJ_AUTH_MIN_PWK4:
            printf("EAPOL Key message 4: (PWK)\n");
            hexdump((t_u8 *) & event_body->info.eapol_pwkmsg,
                    sizeof(eapol_keymsg_debug_t), ' ');
            break;
        case DEBUG_MAJ_AUTH_MIN_GWK1:
            printf("EAPOL Key message 1: (GWK)\n");
            hexdump((t_u8 *) & event_body->info.eapol_pwkmsg,
                    sizeof(eapol_keymsg_debug_t), ' ');
            break;
        case DEBUG_MAJ_AUTH_MIN_GWK2:
            printf("EAPOL Key message 2: (GWK)\n");
            hexdump((t_u8 *) & event_body->info.eapol_pwkmsg,
                    sizeof(eapol_keymsg_debug_t), ' ');
            break;
        case DEBUG_MAJ_AUTH_MIN_STA_REJ:
            printf("Reject STA MAC: ");
            print_mac(event_body->info.sta_reject.sta_mac_addr);
            printf("\n");
            print_reject_state(event_body->info.sta_reject.reject_state);
            print_reject_reason(uap_le16_to_cpu
                                (event_body->info.sta_reject.reject_reason));
            break;
        case DEBUG_MAJ_AUTH_MIN_EAPOL_TR:
            printf("STA MAC: ");
            print_mac(event_body->info.eapol_state.sta_mac_addr);
            printf("\n");
            print_eapol_state(event_body->info.eapol_state.eapol_state);
            break;
        default:
            printf("ERR: unknow debug_id_minor: %d\n",
                   (int) uap_le32_to_cpu(event_body->debug_id_minor));
            hexdump(buffer, size, ' ');
            return;
        }
    } else if (uap_le32_to_cpu(event_body->debug_id_major) ==
               DEBUG_ID_MAJ_ASSOC_AGENT) {
        switch (uap_le32_to_cpu(event_body->debug_id_minor)) {
        case DEBUG_ID_MAJ_ASSOC_MIN_WPA_IE:
            printf("STA MAC: ");
            print_mac(event_body->info.wpaie.sta_mac_addr);
            printf("\n");
            printf("wpa ie:\n");
            hexdump(event_body->info.wpaie.wpa_ie, MAX_WPA_IE_LEN, ' ');
            break;
        case DEBUG_ID_MAJ_ASSOC_MIN_STA_REJ:
            printf("Reject STA MAC: ");
            print_mac(event_body->info.sta_reject.sta_mac_addr);
            printf("\n");
            print_reject_state(event_body->info.sta_reject.reject_state);
            print_reject_reason(uap_le16_to_cpu
                                (event_body->info.sta_reject.reject_reason));
            break;
        default:
            printf("ERR: unknow debug_id_minor: %d\n",
                   (int) uap_le32_to_cpu(event_body->debug_id_minor));
            hexdump(buffer, size, ' ');
            return;
        }
    }
    return;
}

/**
 *  @brief Parse and print received event information
 *
 *  @param event    Pointer to received event
 *  @param size     Length of the received event
 *  @return         N/A
 */
void
print_event(event_header * event, t_u16 size)
{
    t_u32 event_id = event->event_id;
    switch (event_id) {
    case MICRO_AP_EV_ID_STA_DEAUTH:
        print_event_sta_deauth(event->event_data, size - EVENT_ID_LEN);
        break;
    case MICRO_AP_EV_ID_STA_ASSOC:
        print_event_sta_assoc(event->event_data, size - EVENT_ID_LEN);
        break;
    case MICRO_AP_EV_ID_BSS_START:
        print_event_bss_start(event->event_data, size - EVENT_ID_LEN);
        break;
    case MICRO_AP_EV_ID_DEBUG:
        print_event_debug(event->event_data, size - EVENT_ID_LEN);
        break;
    case MICRO_AP_EV_BSS_IDLE:
        printf("EVENT: BSS_IDLE\n");
        break;
    case MICRO_AP_EV_BSS_ACTIVE:
        printf("EVENT: BSS_ACTIVE\n");
        break;
    case MICRO_AP_EV_RSN_CONNECT:
        print_event_rsn_connect(event->event_data, size - EVENT_ID_LEN);
        break;
#ifdef WFD_SUPPORT
    case EVENT_WFD_GENERIC:
        print_event_wfd_generic(event->event_data, size - EVENT_ID_LEN);
        break;
    case EVENT_WFD_SERVICE_DISCOVERY:
        print_event_wfd_service_discovery(event->event_data,
                                          size - EVENT_ID_LEN);
        break;
#endif

    case UAP_EVENT_ID_DRV_HS_ACTIVATED:
        printf("EVENT: HS_ACTIVATED\n");
        break;
    case UAP_EVENT_ID_DRV_HS_DEACTIVATED:
        printf("EVENT: HS_DEACTIVATED\n");
        break;
    case UAP_EVENT_ID_HS_WAKEUP:
        printf("EVENT: HS_WAKEUP\n");
        break;
    case UAP_EVENT_HOST_SLEEP_AWAKE:
        break;

#ifdef UAP_SUPPORT
    case UAP_EVENT_ID_DRV_MGMT_FRAME:
        printf("EVENT: Mgmt frame from FW\n");
        hexdump((void *) event, size, ' ');
        break;
#endif
    case MICRO_AP_EV_WMM_STATUS_CHANGE:
        printf("EVENT: WMM_STATUS_CHANGE\n");
        break;
    default:
        printf("ERR:Undefined event type (0x%X). Dumping event buffer:\n",
               (unsigned int) event_id);
        hexdump((void *) event, size, ' ');
        break;
    }
    return;
}

/**
 *  @brief Read event data from netlink socket
 *
 *  @param sk_fd    Netlink socket handler
 *  @param buffer   Pointer to the data buffer
 *  @param nlh      Pointer to netlink message header
 *  @param msg      Pointer to message header
 *  @return         Number of bytes read or MLAN_EVENT_FAILURE
 */
int
read_event_netlink_socket(int sk_fd, unsigned char *buffer,
                          struct nlmsghdr *nlh, struct msghdr *msg)
{
    int count = -1;
    count = recvmsg(sk_fd, msg, 0);
#if DEBUG
    printf("DBG:Waiting for message from NETLINK.\n");
#endif
    if (count < 0) {
        printf("ERR:NETLINK read failed!\n");
        terminate_flag++;
        return MLAN_EVENT_FAILURE;
    }
#if DEBUG
    printf("DBG:Received message payload (%d)\n", count);
#endif
    if (count > NLMSG_SPACE(NL_MAX_PAYLOAD)) {
        printf("ERR:Buffer overflow!\n");
        return MLAN_EVENT_FAILURE;
    }
    memset(buffer, 0, NL_MAX_PAYLOAD);
    memcpy(buffer, NLMSG_DATA(nlh), count - NLMSG_HDRLEN);
#if DEBUG
    hexdump(buffer, count - NLMSG_HDRLEN, ' ');
#endif
    return count - NLMSG_HDRLEN;
}

/**
 *  @brief Configure and read event data from netlink socket
 *
 *  @param sk_fd    Netlink socket handler
 *  @param buffer   Pointer to the data buffer
 *  @param timeout  Socket listen timeout value
 *  @param nlh      Pointer to netlink message header
 *  @param msg      Pointer to message header
 *  @return         Number of bytes read or MLAN_EVENT_FAILURE
 */
int
read_event(int sk_fd, unsigned char *buffer, int timeout, struct nlmsghdr *nlh,
           struct msghdr *msg)
{
    struct timeval tv;
    fd_set rfds;
    int ret = MLAN_EVENT_FAILURE;

    /* Setup read fds */
    FD_ZERO(&rfds);
    FD_SET(sk_fd, &rfds);

    /* Initialize timeout value */
    if (timeout != 0)
        tv.tv_sec = timeout;
    else
        tv.tv_sec = UAP_RECV_WAIT_DEFAULT;
    tv.tv_usec = 0;

    /* Wait for reply */
    ret = select(sk_fd + 1, &rfds, NULL, NULL, &tv);
    if (ret == -1) {
        /* Error */
        terminate_flag++;
        return MLAN_EVENT_FAILURE;
    } else if (!ret) {
        /* Timeout. Try again */
        return MLAN_EVENT_FAILURE;
    }
    if (!FD_ISSET(sk_fd, &rfds)) {
        /* Unexpected error. Try again */
        return MLAN_EVENT_FAILURE;
    }

    /* Success */
    ret = read_event_netlink_socket(sk_fd, buffer, nlh, msg);
    return ret;
}

/* Command line options */
static const struct option long_opts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

/**
 *  @brief Determine the netlink number
 *
 *  @return         Netlink number to use
 */
static int
get_netlink_num(void)
{
    FILE *fp;
    int netlink_num = NETLINK_MARVELL;
    char str[64];
    char *srch = "netlink_num";

    /* Try to open /proc/mwlan/config */
    fp = fopen("/proc/mwlan/config", "r");
    if (fp) {
        while (!feof(fp)) {
            fgets(str, sizeof(str), fp);
            if (strncmp(str, srch, strlen(srch)) == 0) {
                netlink_num = atoi(str + strlen(srch) + 1);
                break;
            }
        }
        fclose(fp);
    }

    printf("Netlink number = %d\n", netlink_num);
    return netlink_num;
}

/****************************************************************************
        Global functions
****************************************************************************/
/**
 *  @brief The main function
 *
 *  @param argc     Number of arguments
 *  @param argv     Pointer to the arguments
 *  @return         0 or 1
 */
int
main(int argc, char *argv[])
{
    int opt;
    int nl_sk = 0;
    struct nlmsghdr *nlh = NULL;
    struct sockaddr_nl src_addr, dest_addr;
    struct msghdr msg;
    struct iovec iov;
    unsigned char *buffer = NULL;
    struct timeval current_time;
    struct tm *timeinfo;
    int num_events = 0;
    event_header *event = NULL;
    int ret = MLAN_EVENT_FAILURE;
    int netlink_num = 0;
    char if_name[IFNAMSIZ + 1];
    t_u32 event_id = 0;

    /* Check command line options */
    while ((opt = getopt_long(argc, argv, "hvt", long_opts, NULL)) > 0) {
        switch (opt) {
        case 'h':
            print_usage();
            return 0;
        case 'v':
            printf("mlanevent version : %s\n", MLAN_EVENT_VERSION);
            return 0;
            break;
        default:
            print_usage();
            return 1;
        }
    }
    if (optind < argc) {
        fputs("Too many arguments.\n", stderr);
        print_usage();
        return 1;
    }

    netlink_num = get_netlink_num();

    /* Open netlink socket */
    nl_sk = socket(PF_NETLINK, SOCK_RAW, netlink_num);
    if (nl_sk < 0) {
        printf("ERR:Could not open netlink socket.\n");
        ret = MLAN_EVENT_FAILURE;
        goto done;
    }

    /* Set source address */
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* Our PID */
    src_addr.nl_groups = NL_MULTICAST_GROUP;

    /* Bind socket with source address */
    if (bind(nl_sk, (struct sockaddr *) &src_addr, sizeof(src_addr)) < 0) {
        printf("ERR:Could not bind socket!\n");
        ret = MLAN_EVENT_FAILURE;
        goto done;
    }

    /* Set destination address */
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;       /* Kernel */
    dest_addr.nl_groups = NL_MULTICAST_GROUP;

    /* Initialize netlink header */
    nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(NL_MAX_PAYLOAD));
    if (!nlh) {
        printf("ERR: Could not alloc buffer\n");
        ret = MLAN_EVENT_FAILURE;
        goto done;
    }
    memset(nlh, 0, NLMSG_SPACE(NL_MAX_PAYLOAD));

    /* Initialize I/O vector */
    iov.iov_base = (void *) nlh;
    iov.iov_len = NLMSG_SPACE(NL_MAX_PAYLOAD);

    /* Initialize message header */
    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_name = (void *) &dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    /* Initialize receive buffer */
    buffer = malloc(NL_MAX_PAYLOAD);
    if (!buffer) {
        printf("ERR: Could not alloc buffer\n");
        ret = MLAN_EVENT_FAILURE;
        goto done;
    }
    memset(buffer, 0, sizeof(buffer));

    gettimeofday(&current_time, NULL);

    printf("\n");
    printf("**********************************************\n");
    if ((timeinfo = localtime(&(current_time.tv_sec))))
        printf("mlanevent start time : %s", asctime(timeinfo));
    printf("                      %u usecs\n",
           (unsigned int) current_time.tv_usec);
    printf("**********************************************\n");

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGALRM, sig_handler);
    while (1) {
        if (terminate_flag) {
            printf("Stopping!\n");
            break;
        }
        ret = read_event(nl_sk, buffer, 0, nlh, &msg);

        /* No result. Loop again */
        if (ret == MLAN_EVENT_FAILURE) {
            continue;
        }
        if (ret == 0) {
            /* Zero bytes received */
            printf("ERR:Received zero bytes!\n");
            continue;
        }
        num_events++;
        gettimeofday(&current_time, NULL);
        printf("\n");
        printf("============================================\n");
        printf("Received event");
        if ((timeinfo = localtime(&(current_time.tv_sec))))
            printf(": %s", asctime(timeinfo));
        printf("                     %u usecs\n",
               (unsigned int) current_time.tv_usec);
        printf("============================================\n");

        memcpy(&event_id, buffer, sizeof(event_id));
        if (((event_id & 0xFF000000) == 0x80000000) ||
            ((event_id & 0xFF000000) == 0)) {
            event = (event_header *) buffer;
        } else {
            memset(if_name, 0, IFNAMSIZ + 1);
            memcpy(if_name, buffer, IFNAMSIZ);
            printf("EVENT for interface %s\n", if_name);
            event = (event_header *) (buffer + IFNAMSIZ);
            ret -= IFNAMSIZ;
        }
#if DEBUG
        printf("DBG:Received buffer =\n");
        hexdump(buffer, ret, ' ');
#endif
        print_event(event, ret);
        fflush(stdout);
    }
    gettimeofday(&current_time, NULL);
    printf("\n");
    printf("*********************************************\n");
    if ((timeinfo = localtime(&(current_time.tv_sec))))
        printf("mlanevent end time  : %s", asctime(timeinfo));
    printf("                     %u usecs\n",
           (unsigned int) current_time.tv_usec);
    printf("Total events       : %u\n", num_events);
    printf("*********************************************\n");
  done:
    if (buffer)
        free(buffer);
    if (nl_sk > 0)
        close(nl_sk);
    if (nlh)
        free(nlh);
    return 0;
}
