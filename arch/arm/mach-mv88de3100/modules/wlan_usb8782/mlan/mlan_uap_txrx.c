/** @file mlan_uap_txrx.c
 *
 *  @brief This file contains AP mode transmit and receive functions
 *
 *  (C) Copyright 2009-2011 Marvell International Ltd. All Rights Reserved
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

/********************************************************
Change log:
    02/05/2009: initial version
********************************************************/

#include "mlan.h"
#include "mlan_util.h"
#include "mlan_fw.h"
#ifdef STA_SUPPORT
#include "mlan_join.h"
#endif
#include "mlan_main.h"
#include "mlan_uap.h"
#include "mlan_wmm.h"
#include "mlan_11n_aggr.h"
#include "mlan_11n_rxreorder.h"

/********************************************************
                Local Functions
********************************************************/

/**
 *  @brief This function processes received packet and forwards it
 *  		to kernel/upper layer
 *
 *  @param pmadapter A pointer to mlan_adapter
 *  @param pmbuf     A pointer to mlan_buffer which includes the received packet
 *
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
static mlan_status
wlan_upload_uap_rx_packet(pmlan_adapter pmadapter, pmlan_buffer pmbuf)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    pmlan_private priv = pmadapter->priv[pmbuf->bss_index];
    UapRxPD *prx_pd;
    ENTER();
    prx_pd = (UapRxPD *) (pmbuf->pbuf + pmbuf->data_offset);

    /* Chop off RxPD */
    pmbuf->data_len -= prx_pd->rx_pkt_offset;
    pmbuf->data_offset += prx_pd->rx_pkt_offset;
    pmbuf->pparent = MNULL;

    DBG_HEXDUMP(MDAT_D, "uAP RxPD", (t_u8 *) prx_pd,
                MIN(sizeof(UapRxPD), MAX_DATA_DUMP_LEN));
    DBG_HEXDUMP(MDAT_D, "uAP Rx Payload",
                ((t_u8 *) prx_pd + prx_pd->rx_pkt_offset),
                MIN(prx_pd->rx_pkt_length, MAX_DATA_DUMP_LEN));

    pmadapter->callbacks.moal_get_system_time(pmadapter->pmoal_handle,
                                              &pmbuf->out_ts_sec,
                                              &pmbuf->out_ts_usec);
    PRINTM_NETINTF(MDATA, priv);
    PRINTM(MDATA, "%lu.%06lu : Data => kernel seq_num=%d tid=%d\n",
           pmbuf->out_ts_sec, pmbuf->out_ts_usec, prx_pd->seq_num,
           prx_pd->priority);
    ret = pmadapter->callbacks.moal_recv_packet(pmadapter->pmoal_handle, pmbuf);
    if (ret == MLAN_STATUS_FAILURE) {
        PRINTM(MERROR, "uAP Rx Error: moal_recv_packet returned error\n");
        pmbuf->status_code = MLAN_ERROR_PKT_INVALID;
    }

    if (ret != MLAN_STATUS_PENDING) {
        pmadapter->callbacks.moal_recv_complete(pmadapter->pmoal_handle, pmbuf,
                                                MLAN_USB_EP_DATA, ret);
    }
    LEAVE();

    return ret;

}

/********************************************************
    Global Functions
********************************************************/
/**
 *  @brief This function fill the txpd for tx packet
 *
 *  @param priv	   A pointer to mlan_private structure
 *  @param pmbuf   A pointer to the mlan_buffer for process
 *
 *  @return 	   headptr or MNULL
 */
t_void *
wlan_ops_uap_process_txpd(IN t_void * priv, IN pmlan_buffer pmbuf)
{
    pmlan_private pmpriv = (pmlan_private) priv;
    UapTxPD *plocal_tx_pd;
    t_u8 *head_ptr = MNULL;
    t_u32 pkt_type;
    t_u32 tx_control;
    ENTER();

    if (!pmbuf->data_len) {
        PRINTM(MERROR, "uAP Tx Error: Invalid packet length: %d\n",
               pmbuf->data_len);
        pmbuf->status_code = MLAN_ERROR_PKT_SIZE_INVALID;
        goto done;
    }
    if (pmbuf->buf_type == MLAN_BUF_TYPE_RAW_DATA) {
        memcpy(pmpriv->adapter, &pkt_type, pmbuf->pbuf + pmbuf->data_offset,
               sizeof(pkt_type));
        memcpy(pmpriv->adapter, &tx_control,
               pmbuf->pbuf + pmbuf->data_offset + sizeof(pkt_type),
               sizeof(tx_control));
        pmbuf->data_offset += sizeof(pkt_type) + sizeof(tx_control);
        pmbuf->data_len -= sizeof(pkt_type) + sizeof(tx_control);
    }
    if (pmbuf->data_offset < (sizeof(UapTxPD) + INTF_HEADER_LEN +
                              DMA_ALIGNMENT)) {
        PRINTM(MERROR, "not enough space for UapTxPD: len=%d, offset=%d\n",
               pmbuf->data_len, pmbuf->data_offset);
        DBG_HEXDUMP(MDAT_D, "drop pkt", pmbuf->pbuf + pmbuf->data_offset,
                    pmbuf->data_len);
        pmbuf->status_code = MLAN_ERROR_PKT_SIZE_INVALID;
        goto done;
    }

    /* head_ptr should be aligned */
    head_ptr =
        pmbuf->pbuf + pmbuf->data_offset - sizeof(UapTxPD) - INTF_HEADER_LEN;
    head_ptr = (t_u8 *) ((t_ptr) head_ptr & ~((t_ptr) (DMA_ALIGNMENT - 1)));

    plocal_tx_pd = (UapTxPD *) (head_ptr + INTF_HEADER_LEN);
    memset(pmpriv->adapter, plocal_tx_pd, 0, sizeof(UapTxPD));

    /* Set the BSS number to TxPD */
    plocal_tx_pd->bss_num = GET_BSS_NUM(pmpriv);
    plocal_tx_pd->bss_type = pmpriv->bss_type;

    plocal_tx_pd->tx_pkt_length = (t_u16) pmbuf->data_len;

    plocal_tx_pd->priority = (t_u8) pmbuf->priority;
    plocal_tx_pd->pkt_delay_2ms =
        wlan_wmm_compute_driver_packet_delay(pmpriv, pmbuf);

    if (plocal_tx_pd->priority < NELEMENTS(pmpriv->wmm.user_pri_pkt_tx_ctrl))
        /*
         * Set the priority specific tx_control field, setting of 0 will
         *   cause the default value to be used later in this function
         */
        plocal_tx_pd->tx_control
            = pmpriv->wmm.user_pri_pkt_tx_ctrl[plocal_tx_pd->priority];

    /* Offset of actual data */
    plocal_tx_pd->tx_pkt_offset =
        (t_u16) ((t_ptr) pmbuf->pbuf + pmbuf->data_offset -
                 (t_ptr) plocal_tx_pd);

    if (!plocal_tx_pd->tx_control) {
        /* TxCtrl set by user or default */
        plocal_tx_pd->tx_control = pmpriv->pkt_tx_ctrl;
    }

    if (pmbuf->buf_type == MLAN_BUF_TYPE_RAW_DATA) {
        plocal_tx_pd->tx_pkt_type = (t_u16) pkt_type;
        plocal_tx_pd->tx_control = tx_control;
    }
    uap_endian_convert_TxPD(plocal_tx_pd);

    /* Adjust the data offset and length to include TxPD in pmbuf */
    pmbuf->data_len += pmbuf->data_offset;
    pmbuf->data_offset = (t_u32) ((t_ptr) head_ptr - (t_ptr) pmbuf->pbuf);
    pmbuf->data_len -= pmbuf->data_offset;

  done:
    LEAVE();
    return head_ptr;
}

/**
 *  @brief This function processes received packet and forwards it
 *  		to kernel/upper layer
 *
 *  @param adapter   A pointer to mlan_adapter
 *  @param pmbuf     A pointer to mlan_buffer which includes the received packet
 *
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
wlan_ops_uap_process_rx_packet(IN t_void * adapter, IN pmlan_buffer pmbuf)
{
    pmlan_adapter pmadapter = (pmlan_adapter) adapter;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    UapRxPD *prx_pd;
    wlan_mgmt_pkt *puap_pkt_hdr = MNULL;

    RxPacketHdr_t *prx_pkt;
    pmlan_private priv = pmadapter->priv[pmbuf->bss_index];
    t_u8 ta[MLAN_MAC_ADDR_LENGTH];
    t_u16 rx_pkt_type = 0;
    sta_node *sta_ptr = MNULL;

    ENTER();

    prx_pd = (UapRxPD *) (pmbuf->pbuf + pmbuf->data_offset);
    /* Endian conversion */
    uap_endian_convert_RxPD(prx_pd);
    rx_pkt_type = prx_pd->rx_pkt_type;
    prx_pkt = (RxPacketHdr_t *) ((t_u8 *) prx_pd + prx_pd->rx_pkt_offset);

    PRINTM(MINFO, "RX Data: data_len - prx_pd->rx_pkt_offset = %d - %d = %d\n",
           pmbuf->data_len, prx_pd->rx_pkt_offset,
           pmbuf->data_len - prx_pd->rx_pkt_offset);

    if ((prx_pd->rx_pkt_offset + prx_pd->rx_pkt_length) >
        (t_u16) pmbuf->data_len) {
        PRINTM(MERROR,
               "Wrong rx packet: len=%d,rx_pkt_offset=%d,"
               " rx_pkt_length=%d\n", pmbuf->data_len, prx_pd->rx_pkt_offset,
               prx_pd->rx_pkt_length);
        pmbuf->status_code = MLAN_ERROR_PKT_SIZE_INVALID;
        ret = MLAN_STATUS_FAILURE;
        pmadapter->callbacks.moal_recv_complete(pmadapter->pmoal_handle,
                                                pmbuf, MLAN_USB_EP_DATA, ret);
        goto done;
    }
    pmbuf->data_len = prx_pd->rx_pkt_offset + prx_pd->rx_pkt_length;

    if (pmadapter->priv[pmbuf->bss_index]->mgmt_frame_passthru_mask &&
        prx_pd->rx_pkt_type == PKT_TYPE_MGMT_FRAME) {
        /* Check if this is mgmt packet and needs to forwarded to app as an
           event */
        puap_pkt_hdr =
            (wlan_mgmt_pkt *) ((t_u8 *) prx_pd + prx_pd->rx_pkt_offset);
        puap_pkt_hdr->frm_len = wlan_le16_to_cpu(puap_pkt_hdr->frm_len);
        if ((puap_pkt_hdr->wlan_header.
             frm_ctl & IEEE80211_FC_MGMT_FRAME_TYPE_MASK) == 0)
            wlan_process_802dot11_mgmt_pkt(pmadapter->priv[pmbuf->bss_index],
                                           (t_u8 *) & puap_pkt_hdr->wlan_header,
                                           puap_pkt_hdr->frm_len +
                                           sizeof(wlan_mgmt_pkt) -
                                           sizeof(puap_pkt_hdr->frm_len));
        pmadapter->callbacks.moal_recv_complete(pmadapter->pmoal_handle, pmbuf,
                                                MLAN_USB_EP_DATA, ret);
        goto done;
    }

    pmbuf->priority = prx_pd->priority;
    if (prx_pd->rx_pkt_type == PKT_TYPE_AMSDU) {
        pmbuf->data_len = prx_pd->rx_pkt_length;
        pmbuf->data_offset += prx_pd->rx_pkt_offset;
        wlan_11n_deaggregate_pkt(priv, pmbuf);
        goto done;
    }
    memcpy(pmadapter, ta, prx_pkt->eth803_hdr.src_addr, MLAN_MAC_ADDR_LENGTH);
    if ((rx_pkt_type != PKT_TYPE_BAR) && (prx_pd->priority < MAX_NUM_TID)) {
        if ((sta_ptr = wlan_get_station_entry(priv, ta)))
            sta_ptr->rx_seq[prx_pd->priority] = prx_pd->seq_num;
    }
    /* check if UAP enable 11n */
    if (!priv->is_11n_enabled ||
        (!wlan_11n_get_rxreorder_tbl
         ((mlan_private *) priv, prx_pd->priority, ta)
        )) {
        if (priv->pkt_fwd)
            wlan_process_uap_rx_packet(priv, pmbuf);
        else
            wlan_upload_uap_rx_packet(pmadapter, pmbuf);
        goto done;
    }
    /* Reorder and send to OS */
    if ((ret = mlan_11n_rxreorder_pkt(priv, prx_pd->seq_num,
                                      prx_pd->priority, ta,
                                      (t_u8) prx_pd->rx_pkt_type,
                                      (void *) pmbuf))
        || (rx_pkt_type == PKT_TYPE_BAR)) {
        if ((ret =
             pmadapter->callbacks.moal_recv_complete(pmadapter->pmoal_handle,
                                                     pmbuf, MLAN_USB_EP_DATA,
                                                     ret)))
            PRINTM(MERROR, "uAP Rx Error: moal_recv_complete returned error\n");
    }
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function processes received packet and forwards it
 *  		to kernel/upper layer or send back to firmware
 *
 *  @param priv      A pointer to mlan_private
 *  @param pmbuf     A pointer to mlan_buffer which includes the received packet
 *
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
wlan_uap_recv_packet(IN mlan_private * priv, IN pmlan_buffer pmbuf)
{
    pmlan_adapter pmadapter = priv->adapter;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    RxPacketHdr_t *prx_pkt;
    pmlan_buffer newbuf = MNULL;

    ENTER();

    prx_pkt = (RxPacketHdr_t *) ((t_u8 *) pmbuf->pbuf + pmbuf->data_offset);

    DBG_HEXDUMP(MDAT_D, "uap_recv_packet", pmbuf->pbuf + pmbuf->data_offset,
                MIN(pmbuf->data_len, MAX_DATA_DUMP_LEN));

    PRINTM(MDATA, "AMSDU dest %02x:%02x:%02x:%02x:%02x:%02x\n",
           prx_pkt->eth803_hdr.dest_addr[0], prx_pkt->eth803_hdr.dest_addr[1],
           prx_pkt->eth803_hdr.dest_addr[2], prx_pkt->eth803_hdr.dest_addr[3],
           prx_pkt->eth803_hdr.dest_addr[4], prx_pkt->eth803_hdr.dest_addr[5]);

    /* don't do packet forwarding in disconnected state */
    if ((priv->media_connected == MFALSE) ||
        (pmbuf->data_len > MV_ETH_FRAME_LEN))
        goto upload;

    if (prx_pkt->eth803_hdr.dest_addr[0] & 0x01) {
        /* Multicast pkt */
        if ((newbuf =
             wlan_alloc_mlan_buffer(pmadapter, MLAN_TX_DATA_BUF_SIZE_2K, 0,
                                    MOAL_MALLOC_BUFFER))) {
            newbuf->bss_index = pmbuf->bss_index;
            newbuf->buf_type = pmbuf->buf_type;
            newbuf->priority = pmbuf->priority;
            newbuf->in_ts_sec = pmbuf->in_ts_sec;
            newbuf->in_ts_usec = pmbuf->in_ts_usec;
            newbuf->data_offset =
                (sizeof(UapTxPD) + INTF_HEADER_LEN + DMA_ALIGNMENT);
            pmadapter->pending_bridge_pkts++;
            newbuf->flags |= MLAN_BUF_FLAG_BRIDGE_BUF;

            /* copy the data */
            memcpy(pmadapter, (t_u8 *) newbuf->pbuf + newbuf->data_offset,
                   pmbuf->pbuf + pmbuf->data_offset, pmbuf->data_len);
            newbuf->data_len = pmbuf->data_len;
            wlan_wmm_add_buf_txqueue(pmadapter, newbuf);
            if (pmadapter->pending_bridge_pkts > RX_HIGH_THRESHOLD)
                wlan_drop_tx_pkts(priv);
        }
    } else {
        if (wlan_get_station_entry(priv, prx_pkt->eth803_hdr.dest_addr)) {
            /* Intra BSS packet */
            if ((newbuf =
                 wlan_alloc_mlan_buffer(pmadapter, MLAN_TX_DATA_BUF_SIZE_2K, 0,
                                        MOAL_MALLOC_BUFFER))) {
                newbuf->bss_index = pmbuf->bss_index;
                newbuf->buf_type = pmbuf->buf_type;
                newbuf->priority = pmbuf->priority;
                newbuf->in_ts_sec = pmbuf->in_ts_sec;
                newbuf->in_ts_usec = pmbuf->in_ts_usec;
                newbuf->data_offset =
                    (sizeof(UapTxPD) + INTF_HEADER_LEN + DMA_ALIGNMENT);
                pmadapter->pending_bridge_pkts++;
                newbuf->flags |= MLAN_BUF_FLAG_BRIDGE_BUF;

                /* copy the data */
                memcpy(pmadapter, (t_u8 *) newbuf->pbuf + newbuf->data_offset,
                       pmbuf->pbuf + pmbuf->data_offset, pmbuf->data_len);
                newbuf->data_len = pmbuf->data_len;
                wlan_wmm_add_buf_txqueue(pmadapter, newbuf);
                if (pmadapter->pending_bridge_pkts > RX_HIGH_THRESHOLD)
                    wlan_drop_tx_pkts(priv);
            }
            goto done;
        }
    }
  upload:
    /** send packet to moal */
    ret = pmadapter->callbacks.moal_recv_packet(pmadapter->pmoal_handle, pmbuf);
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function processes received packet and forwards it
 *  		to kernel/upper layer or send back to firmware
 *
 *  @param priv      A pointer to mlan_private
 *  @param pmbuf     A pointer to mlan_buffer which includes the received packet
 *
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
wlan_process_uap_rx_packet(IN mlan_private * priv, IN pmlan_buffer pmbuf)
{
    pmlan_adapter pmadapter = priv->adapter;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    UapRxPD *prx_pd;
    RxPacketHdr_t *prx_pkt;
    pmlan_buffer newbuf = MNULL;

    ENTER();

    prx_pd = (UapRxPD *) (pmbuf->pbuf + pmbuf->data_offset);
    prx_pkt = (RxPacketHdr_t *) ((t_u8 *) prx_pd + prx_pd->rx_pkt_offset);

    DBG_HEXDUMP(MDAT_D, "uAP RxPD", prx_pd,
                MIN(sizeof(UapRxPD), MAX_DATA_DUMP_LEN));
    DBG_HEXDUMP(MDAT_D, "uAP Rx Payload",
                ((t_u8 *) prx_pd + prx_pd->rx_pkt_offset),
                MIN(prx_pd->rx_pkt_length, MAX_DATA_DUMP_LEN));

    PRINTM(MINFO, "RX Data: data_len - prx_pd->rx_pkt_offset = %d - %d = %d\n",
           pmbuf->data_len, prx_pd->rx_pkt_offset,
           pmbuf->data_len - prx_pd->rx_pkt_offset);
    PRINTM(MDATA, "Rx dest %02x:%02x:%02x:%02x:%02x:%02x\n",
           prx_pkt->eth803_hdr.dest_addr[0], prx_pkt->eth803_hdr.dest_addr[1],
           prx_pkt->eth803_hdr.dest_addr[2], prx_pkt->eth803_hdr.dest_addr[3],
           prx_pkt->eth803_hdr.dest_addr[4], prx_pkt->eth803_hdr.dest_addr[5]);

    /* don't do packet forwarding in disconnected state */
    /* don't do packet forwarding when packet > 1514 */
    if ((priv->media_connected == MFALSE) ||
        ((pmbuf->data_len - prx_pd->rx_pkt_offset) > MV_ETH_FRAME_LEN))
        goto upload;

    if (prx_pkt->eth803_hdr.dest_addr[0] & 0x01) {
        /* Multicast pkt */
        if ((newbuf =
             wlan_alloc_mlan_buffer(pmadapter, MLAN_TX_DATA_BUF_SIZE_2K, 0,
                                    MOAL_MALLOC_BUFFER))) {
            newbuf->bss_index = pmbuf->bss_index;
            newbuf->buf_type = pmbuf->buf_type;
            newbuf->priority = pmbuf->priority;
            newbuf->in_ts_sec = pmbuf->in_ts_sec;
            newbuf->in_ts_usec = pmbuf->in_ts_usec;
            newbuf->data_offset =
                (sizeof(UapTxPD) + INTF_HEADER_LEN + DMA_ALIGNMENT);
            pmadapter->pending_bridge_pkts++;
            newbuf->flags |= MLAN_BUF_FLAG_BRIDGE_BUF;

            /* copy the data, skip rxpd */
            memcpy(pmadapter, (t_u8 *) newbuf->pbuf + newbuf->data_offset,
                   pmbuf->pbuf + pmbuf->data_offset + prx_pd->rx_pkt_offset,
                   pmbuf->data_len - prx_pd->rx_pkt_offset);
            newbuf->data_len = pmbuf->data_len - prx_pd->rx_pkt_offset;
            wlan_wmm_add_buf_txqueue(pmadapter, newbuf);
            if (pmadapter->pending_bridge_pkts > RX_HIGH_THRESHOLD)
                wlan_drop_tx_pkts(priv);
        }
    } else {
        if (wlan_get_station_entry(priv, prx_pkt->eth803_hdr.dest_addr)) {
            /* Forwarding Intra-BSS packet */
            pmbuf->data_len -= prx_pd->rx_pkt_offset;
            pmbuf->data_offset += prx_pd->rx_pkt_offset;
            pmbuf->flags |= MLAN_BUF_FLAG_BRIDGE_BUF;
            pmadapter->pending_bridge_pkts++;
            wlan_wmm_add_buf_txqueue(pmadapter, pmbuf);
            if (pmadapter->pending_bridge_pkts > RX_HIGH_THRESHOLD)
                wlan_drop_tx_pkts(priv);
            pmadapter->callbacks.moal_recv_complete(pmadapter->pmoal_handle,
                                                    MNULL,
                                                    MLAN_USB_EP_DATA, ret);
            goto done;
        }
    }
  upload:
    /* Chop off RxPD */
    pmbuf->data_len -= prx_pd->rx_pkt_offset;
    pmbuf->data_offset += prx_pd->rx_pkt_offset;
    pmbuf->pparent = MNULL;

    pmadapter->callbacks.moal_get_system_time(pmadapter->pmoal_handle,
                                              &pmbuf->out_ts_sec,
                                              &pmbuf->out_ts_usec);
    PRINTM_NETINTF(MDATA, priv);
    PRINTM(MDATA, "%lu.%06lu : Data => kernel seq_num=%d tid=%d\n",
           pmbuf->out_ts_sec, pmbuf->out_ts_usec, prx_pd->seq_num,
           prx_pd->priority);
    ret = pmadapter->callbacks.moal_recv_packet(pmadapter->pmoal_handle, pmbuf);
    if (ret == MLAN_STATUS_FAILURE) {
        PRINTM(MERROR, "uAP Rx Error: moal_recv_packet returned error\n");
        pmbuf->status_code = MLAN_ERROR_PKT_INVALID;
    }

    if (ret != MLAN_STATUS_PENDING) {
        pmadapter->callbacks.moal_recv_complete(pmadapter->pmoal_handle, pmbuf,
                                                MLAN_USB_EP_DATA, ret);
    }
  done:
    LEAVE();
    return ret;
}
