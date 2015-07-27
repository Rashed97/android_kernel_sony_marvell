/** @file moal_usb.c
  *
  * @brief This file contains the interfaceing to USB bus
  * driver.
  *
  * Copyright (C) 2008-2011, Marvell International Ltd.
  *
  * This software file (the "File") is distributed by Marvell International
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991
  * (the "License").  You may use, redistribute and/or modify this File in
  * accordance with the terms and conditions of the License, a copy of which
  * is available by writing to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
  * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
  * this warranty disclaimer.
  *
  */

/********************************************************
Change log:
    10/21/2008: initial version
********************************************************/

#include	"moal_main.h"
#include	"moal_usb.h"
/********************************************************
		Local Variables
********************************************************/

/** Marvell USB device */
#define MARVELL_USB_DEVICE(vid, pid, name) \
           USB_DEVICE(vid, pid),  \
           .driver_info = (t_ptr)name

/** Name of the USB driver */
const char usbdriver_name[] = "usb8xxx";

/** This structure contains the device signature */
struct usb_device_id woal_usb_table[] = {
    /* Enter the device signature inside */
    {MARVELL_USB_DEVICE
     (USB8797_VID_1, USB8797_PID_1, "Marvell WLAN USB Adapter")},
    {MARVELL_USB_DEVICE
     (USB8797_VID_1, USB8797_PID_2, "Marvell WLAN USB Adapter")},
    {MARVELL_USB_DEVICE
     (USB8782_VID_1, USB8782_PID_1, "Marvell WLAN USB Adapter")},
    /* Terminating entry */
    {},
};

/** This structure contains the device signature */
struct usb_device_id woal_usb_table_skip_fwdnld[] = {
    /* Enter the device signature inside */
    {MARVELL_USB_DEVICE
     (USB8797_VID_1, USB8797_PID_2, "Marvell WLAN USB Adapter")},
    /* Terminating entry */
    {},
};

static mlan_status woal_usb_submit_rx_urb(urb_context * ctx, int size);
static int woal_usb_probe(struct usb_interface *intf,
                          const struct usb_device_id *id);
static void woal_usb_disconnect(struct usb_interface *intf);
static int woal_usb_suspend(struct usb_interface *intf, pm_message_t message);
static int woal_usb_resume(struct usb_interface *intf);

/** woal_usb_driver */
static struct usb_driver REFDATA woal_usb_driver = {
    /* Driver name */
    .name = usbdriver_name,

    /* Probe function name */
    .probe = woal_usb_probe,

    /* Disconnect function name */
    .disconnect = woal_usb_disconnect,

    /* Device signature table */
    .id_table = woal_usb_table,
    /* Suspend function name */
    .suspend = woal_usb_suspend,

    /* Resume function name */
    .resume = woal_usb_resume,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
    /* Driver supports autosuspend */
    .supports_autosuspend = 1,
#endif
};

MODULE_DEVICE_TABLE(usb, woal_usb_table);
MODULE_DEVICE_TABLE(usb, woal_usb_table_skip_fwdnld);

/********************************************************
		Global Variables
********************************************************/

extern int skip_fwdnld;
extern int max_tx_buf;

/********************************************************
		Local Functions
********************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
/**
 *  @brief This function receive packet of the data/cmd/event packet
 *         and pass to MLAN
 *
 *  @param urb		Pointer to struct urb
 *  @param regs		Registers
 *
 *  @return 	   	N/A
 */
static void
woal_usb_receive(struct urb *urb, struct pt_regs *regs)
#else
/**
 * @brief This function receive packet of the data/cmd/event packet
 *         and pass to MLAN
 *
 *  @param urb		Pointer to struct urb
 *
 *  @return 	   	N/A
 */
static void
woal_usb_receive(struct urb *urb)
#endif
{
    urb_context *context = NULL;
    moal_handle *handle = NULL;
    mlan_buffer *pmbuf = NULL;
    struct usb_card_rec *cardp = NULL;
    int recv_length;
    int size;
    mlan_status status = MLAN_STATUS_SUCCESS;

    ENTER();

    if (!urb || !urb->context) {
        PRINTM(MERROR, "URB or URB context is not valid in USB Rx\n");
        LEAVE();
        return;
    }
    context = (urb_context *) urb->context;
    handle = context->handle;
    pmbuf = context->pmbuf;
    recv_length = urb->actual_length;

    if (!handle || !handle->card) {
        PRINTM(MERROR,
               "moal handle or card structure is not valid in USB Rx\n");
        LEAVE();
        return;
    }
    cardp = (struct usb_card_rec *) handle->card;
    if (cardp->rx_cmd_ep == context->ep)
        atomic_dec(&cardp->rx_cmd_urb_pending);
    else
        atomic_dec(&cardp->rx_data_urb_pending);

    if (recv_length) {
        if (urb->status || (handle->surprise_removed == MTRUE)) {
            if (handle->surprise_removed || handle->is_suspended) {
                woal_free_mlan_buffer(handle, pmbuf);
                context->pmbuf = NULL;
                goto rx_exit;
            } else {
                PRINTM(MERROR, "EP %d Rx URB status failure: %d\n", context->ep,
                       urb->status);
                /* Do not free mlan_buffer in case of command ep */
                if (cardp->rx_cmd_ep != context->ep)
                    woal_free_mlan_buffer(handle, pmbuf);
                goto setup_for_next;
            }
        }
        pmbuf->data_len = recv_length;
        pmbuf->flags |= MLAN_BUF_FLAG_RX_DEAGGR;
        /* Send packet to MLAN */
        atomic_inc(&handle->rx_pending);
        status = mlan_recv(handle->pmlan_adapter, pmbuf, context->ep);
        PRINTM(MINFO, "Receive length = 0x%x, status=%d\n", recv_length,
               status);
        if (status == MLAN_STATUS_PENDING) {
            queue_work(handle->workqueue, &handle->main_work);
            /* urb for data_ep is re-submitted now */
            /* urb for cmd_ep will be re-submitted in callback
               moal_recv_complete */
            if (cardp->rx_cmd_ep == context->ep)
                goto rx_exit;
        } else {
            atomic_dec(&handle->rx_pending);
            if (status == MLAN_STATUS_FAILURE) {
                PRINTM(MERROR, "MLAN fail to process the receive data\n");
            } else if ((status == MLAN_STATUS_SUCCESS) &&
                       (pmbuf->flags & MLAN_BUF_FLAG_SLEEPCFM_RESP)) {
                pmbuf->flags &= ~MLAN_BUF_FLAG_SLEEPCFM_RESP;
                queue_work(handle->workqueue, &handle->main_work);
            }
            /* Do not free mlan_buffer in case of command ep */
            if (cardp->rx_cmd_ep != context->ep)
                woal_free_mlan_buffer(handle, pmbuf);
        }
    } else if (urb->status) {
        if (!
            ((cardp->rx_data_ep == context->ep) &&
             (cardp->resubmit_urbs == 1))) {
            if (!handle->is_suspended) {
                PRINTM(MMSG, "Card is removed: %d\n", urb->status);
                handle->surprise_removed = MTRUE;
            }
        }
        woal_free_mlan_buffer(handle, pmbuf);
        context->pmbuf = NULL;
        goto rx_exit;
    } else {
        /* Do not free mlan_buffer in case of command ep */
        if (cardp->rx_cmd_ep != context->ep)
            woal_free_mlan_buffer(handle, pmbuf);
        goto setup_for_next;
    }

  setup_for_next:
    if (cardp->rx_cmd_ep == context->ep) {
        size = MLAN_RX_CMD_BUF_SIZE;
    } else {
        if (cardp->rx_deaggr_ctrl.enable) {
            size = cardp->rx_deaggr_ctrl.aggr_max;
            if (cardp->rx_deaggr_ctrl.aggr_mode == MLAN_USB_AGGR_MODE_NUM) {
                size *=
                    MAX(MLAN_USB_MAX_PKT_SIZE,
                        cardp->rx_deaggr_ctrl.aggr_align);
                size = MAX(size, MLAN_USB_RX_DATA_BUF_SIZE);
            }

        } else
            size = MLAN_USB_RX_DATA_BUF_SIZE;
    }
    woal_usb_submit_rx_urb(context, size);

  rx_exit:
    LEAVE();
    return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
/**
 *  @brief  Call back function to handle the status of the Tx data URB
 *
 *  @param urb      Pointer to urb structure
 *  @param regs     Registers
 *
 *  @return         N/A
 */
static void
woal_usb_tx_complete(struct urb *urb, struct pt_regs *regs)
#else
/**
 * @brief  Call back function to handle the status of the Tx data URB
 *
 * @param urb      Pointer to urb structure
 *
 * @return         N/A
 */
static void
woal_usb_tx_complete(struct urb *urb)
#endif
{
    urb_context *context = NULL;
    moal_handle *handle = NULL;
    struct usb_card_rec *cardp = NULL;

    ENTER();

    if (!urb || !urb->context) {
        PRINTM(MERROR, "URB or URB context is not valid in USB Tx complete\n");
        LEAVE();
        return;
    }
    context = (urb_context *) urb->context;
    handle = context->handle;

    if (!handle || !handle->card) {
        PRINTM(MERROR,
               "moal handle or card structure is not valid in USB Tx complete\n");
        LEAVE();
        return;
    }
    cardp = handle->card;

    /* Decrease pending URB counter */
    if (context->ep == cardp->tx_cmd_ep)
        atomic_dec(&cardp->tx_cmd_urb_pending);
    else
        atomic_dec(&cardp->tx_data_urb_pending);

    /* Handle the transmission complete validations */
    if (urb->status) {
        PRINTM(MERROR, "EP %d Tx URB status failure: %d\n", context->ep,
               urb->status);
        mlan_write_data_async_complete(handle->pmlan_adapter, context->pmbuf,
                                       context->ep, MLAN_STATUS_FAILURE);
    } else {
        mlan_write_data_async_complete(handle->pmlan_adapter, context->pmbuf,
                                       context->ep, MLAN_STATUS_SUCCESS);
    }
    queue_work(handle->workqueue, &handle->main_work);

    LEAVE();
    return;
}

/**
 *  @brief This function sets up the data to receive
 *
 *  @param ctx		Pointer to urb_context structure
 *  @param size	        Skb size
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
static mlan_status
woal_usb_submit_rx_urb(urb_context * ctx, int size)
{
    moal_handle *handle = ctx->handle;
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    mlan_status ret = MLAN_STATUS_FAILURE;
    t_u8 *data = NULL;

    ENTER();

    if (handle->surprise_removed || handle->is_suspended) {
        if ((cardp->rx_cmd_ep == ctx->ep) && ctx->pmbuf) {
            woal_free_mlan_buffer(handle, ctx->pmbuf);
            ctx->pmbuf = NULL;
        }
        PRINTM(MERROR, "Card removed/suspended, EP %d Rx URB submit skipped\n",
               ctx->ep);
        goto rx_ret;
    }

    if (cardp->rx_cmd_ep != ctx->ep) {
        if (!(ctx->pmbuf = woal_alloc_mlan_buffer(handle, size))) {
            PRINTM(MERROR, "Fail to submit Rx URB due to no memory/skb\n");
            goto rx_ret;
        }
        ctx->pmbuf->data_offset = MLAN_NET_IP_ALIGN + MLAN_RX_HEADER_LEN;
        data = ctx->pmbuf->pbuf + ctx->pmbuf->data_offset;
    } else {
        ctx->pmbuf->data_offset = 0;
        data = ctx->pmbuf->pbuf + ctx->pmbuf->data_offset;
    }

    usb_fill_bulk_urb(ctx->urb, cardp->udev,
                      usb_rcvbulkpipe(cardp->udev, ctx->ep),
                      data,
                      size - ctx->pmbuf->data_offset,
                      woal_usb_receive, (void *) ctx);
    if (cardp->rx_cmd_ep == ctx->ep)
        atomic_inc(&cardp->rx_cmd_urb_pending);
    else
        atomic_inc(&cardp->rx_data_urb_pending);
    if (usb_submit_urb(ctx->urb, GFP_ATOMIC)) {
        /* Submit URB failure */
        PRINTM(MERROR, "Submit EP %d Rx URB failed: %d\n", ctx->ep, ret);
        woal_free_mlan_buffer(handle, ctx->pmbuf);
        if (cardp->rx_cmd_ep == ctx->ep)
            atomic_dec(&cardp->rx_cmd_urb_pending);
        else
            atomic_dec(&cardp->rx_data_urb_pending);
        ctx->pmbuf = NULL;
        ret = MLAN_STATUS_FAILURE;
    } else {
        ret = MLAN_STATUS_SUCCESS;
    }
  rx_ret:
    LEAVE();
    return ret;
}

/********************************************************
		Global Functions
********************************************************/

/**
 *  @brief  Free Tx/Rx urb, skb and Rx buffer
 *
 *  @param cardp	Pointer usb_card_rec
 *
 *  @return 	   	N/A
 */
void
woal_usb_free(struct usb_card_rec *cardp)
{
    int i;

    ENTER();

    /* Unlink Rx cmd URB */
    if (atomic_read(&cardp->rx_cmd_urb_pending) && cardp->rx_cmd.urb) {
        usb_kill_urb(cardp->rx_cmd.urb);
    }
    /* Unlink Rx data URBs */
    if (atomic_read(&cardp->rx_data_urb_pending)) {
        for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
            if (cardp->rx_data_list[i].urb)
                usb_kill_urb(cardp->rx_data_list[i].urb);
        }
    }

    /* Free Rx data URBs */
    for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
        if (cardp->rx_data_list[i].urb) {
            usb_free_urb(cardp->rx_data_list[i].urb);
            cardp->rx_data_list[i].urb = NULL;
        }
    }
    /* Free Rx cmd URB */
    if (cardp->rx_cmd.urb) {
        usb_free_urb(cardp->rx_cmd.urb);
        cardp->rx_cmd.urb = NULL;
    }

    /* Free Tx data URBs */
    for (i = 0; i < MVUSB_TX_HIGH_WMARK; i++) {
        if (cardp->tx_data_list[i].urb) {
            usb_free_urb(cardp->tx_data_list[i].urb);
            cardp->tx_data_list[i].urb = NULL;
        }
    }
    /* Free Tx cmd URB */
    if (cardp->tx_cmd.urb) {
        usb_free_urb(cardp->tx_cmd.urb);
        cardp->tx_cmd.urb = NULL;
    }

    LEAVE();
    return;
}

/**
 *  @brief Sets the configuration values
 *
 *  @param intf		Pointer to usb_interface
 *  @param id		Pointer to usb_device_id
 *
 *  @return 	   	Address of variable usb_cardp, error code otherwise
 */
static int
woal_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    struct usb_device *udev;
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    int i;
    struct usb_card_rec *usb_cardp = NULL;

    ENTER();

    udev = interface_to_usbdev(intf);
    usb_cardp = kmalloc(sizeof(struct usb_card_rec), GFP_KERNEL);
    memset((t_u8 *) usb_cardp, 0, sizeof(struct usb_card_rec));
    if (!usb_cardp) {
        LEAVE();
        return (-ENOMEM);
    }

    /* Check probe is for our device */
    for (i = 0; woal_usb_table[i].idVendor; i++) {
        if (woal_cpu_to_le16(udev->descriptor.idVendor) ==
            woal_usb_table[i].idVendor &&
            woal_cpu_to_le16(udev->descriptor.idProduct) ==
            woal_usb_table[i].idProduct) {
            PRINTM(MMSG, "VID/PID = %X/%X, Boot2 version = %X\n",
                   woal_cpu_to_le16(udev->descriptor.idVendor),
                   woal_cpu_to_le16(udev->descriptor.idProduct),
                   woal_cpu_to_le16(udev->descriptor.bcdDevice));
            switch (woal_cpu_to_le16(udev->descriptor.idProduct)) {
            case USB8797_PID_1:
                /* If skip FW is set, we must return error so the next driver
                   can download the FW */
                if (skip_fwdnld)
                    goto error;
                else
                    usb_cardp->boot_state = USB_FW_DNLD;
                break;
            case USB8797_PID_2:
                usb_cardp->boot_state = USB_FW_READY;
                break;
            }
            switch (woal_cpu_to_le16(udev->descriptor.idProduct)) {
            case USB8782_PID_1:
                usb_cardp->card_type = CARD_TYPE_USB8782;
                break;
            case USB8797_PID_1:
            case USB8797_PID_2:
                usb_cardp->card_type = CARD_TYPE_USB8797;
                break;
            default:
                PRINTM(MERROR, "Invalid card type detected\n");
                break;
            }
            break;
        }
    }

    if (woal_usb_table[i].idVendor) {
        usb_cardp->udev = udev;
        iface_desc = intf->cur_altsetting;
        usb_cardp->intf = intf;

        PRINTM(MINFO, "bcdUSB = 0x%X bDeviceClass = 0x%X"
               " bDeviceSubClass = 0x%X, bDeviceProtocol = 0x%X\n",
               woal_cpu_to_le16(udev->descriptor.bcdUSB),
               udev->descriptor.bDeviceClass,
               udev->descriptor.bDeviceSubClass,
               udev->descriptor.bDeviceProtocol);

        for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
            endpoint = &iface_desc->endpoint[i].desc;
            if (usb_endpoint_is_bulk_in(endpoint) &&
                usb_endpoint_num(endpoint) == MLAN_USB_EP_CMD_EVENT) {
                /* We found a bulk in command/event endpoint */
                PRINTM(MINFO, "Bulk IN: max packet size = %d, address = %d\n",
                       woal_le16_to_cpu(endpoint->wMaxPacketSize),
                       endpoint->bEndpointAddress);
                usb_cardp->rx_cmd_ep =
                    (endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
                atomic_set(&usb_cardp->rx_cmd_urb_pending, 0);
            }
            if (usb_endpoint_is_bulk_in(endpoint) &&
                usb_endpoint_num(endpoint) == MLAN_USB_EP_DATA) {
                /* We found a bulk in data endpoint */
                PRINTM(MINFO, "Bulk IN: max packet size = %d, address = %d\n",
                       woal_le16_to_cpu(endpoint->wMaxPacketSize),
                       endpoint->bEndpointAddress);
                usb_cardp->rx_data_ep =
                    (endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
                atomic_set(&usb_cardp->rx_data_urb_pending, 0);
            }
            if (usb_endpoint_is_bulk_out(endpoint) &&
                usb_endpoint_num(endpoint) == MLAN_USB_EP_DATA) {
                /* We found a bulk out data endpoint */
                PRINTM(MINFO, "Bulk OUT: max packet size = %d, address = %d\n",
                       woal_le16_to_cpu(endpoint->wMaxPacketSize),
                       endpoint->bEndpointAddress);
                usb_cardp->tx_data_ep = endpoint->bEndpointAddress;
                atomic_set(&usb_cardp->tx_data_urb_pending, 0);
            }
            if (usb_endpoint_is_bulk_out(endpoint) &&
                usb_endpoint_num(endpoint) == MLAN_USB_EP_CMD_EVENT) {
                /* We found a bulk out command/event endpoint */
                PRINTM(MINFO, "Bulk OUT: max packet size = %d, address = %d\n",
                       woal_le16_to_cpu(endpoint->wMaxPacketSize),
                       endpoint->bEndpointAddress);
                usb_cardp->tx_cmd_ep = endpoint->bEndpointAddress;
                atomic_set(&usb_cardp->tx_cmd_urb_pending, 0);
                usb_cardp->bulk_out_maxpktsize =
                    woal_le16_to_cpu(endpoint->wMaxPacketSize);
            }
        }

        if (usb_cardp->boot_state == USB_FW_DNLD) {
            if (!usb_cardp->tx_cmd_ep || !usb_cardp->rx_cmd_ep)
                goto error;
        } else if (usb_cardp->boot_state == USB_FW_READY) {
            if (!usb_cardp->tx_cmd_ep || !usb_cardp->tx_data_ep ||
                !usb_cardp->rx_cmd_ep || !usb_cardp->rx_data_ep) {
                PRINTM(MERROR, "%s: invalid endpoint assignment\n",
                       __FUNCTION__);
                goto error;
            }
        }

        usb_cardp->tx_aggr_ctrl.enable = MFALSE;
        usb_cardp->tx_aggr_ctrl.aggr_mode = MLAN_USB_AGGR_MODE_NUM;
        usb_cardp->tx_aggr_ctrl.aggr_align =
            MAX(max_tx_buf, MLAN_USB_TX_AGGR_ALIGN);
        usb_cardp->tx_aggr_ctrl.aggr_max = MLAN_USB_TX_MAX_AGGR_NUM;
        usb_cardp->tx_aggr_ctrl.aggr_tmo = MLAN_USB_TX_AGGR_TIMEOUT_MSEC * 1000;
        usb_cardp->rx_deaggr_ctrl.enable = MFALSE;
        usb_cardp->rx_deaggr_ctrl.aggr_mode = MLAN_USB_AGGR_MODE_NUM;
        usb_cardp->rx_deaggr_ctrl.aggr_align = MLAN_USB_RX_ALIGN_SIZE;
        usb_cardp->rx_deaggr_ctrl.aggr_max = MLAN_USB_RX_MAX_AGGR_NUM;
        usb_cardp->rx_deaggr_ctrl.aggr_tmo = MLAN_USB_RX_DEAGGR_TIMEOUT_USEC;
        usb_set_intfdata(intf, usb_cardp);
        /* At this point wlan_add_card() will be called */
        if (!(woal_add_card(usb_cardp))) {
            PRINTM(MERROR, "%s: woal_add_card failed\n", __FUNCTION__);
            goto error;
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
        /* Note: From 2.6.34.2 onwards, remote wakeup is NOT enabled by
           default. So drivers wanting remote wakeup will have to enable this
           using - device_set_wakeup_enable(&udev->dev, 1); It has been
           observed that some cards having device attr = 0xa0 do not support
           remote wakeup. These cards come immediately out of the suspend when
           power/wakeup file is set to 'enabled'. To support all types of cards
           i.e. with/without remote wakeup, we are NOT setting the
           'power/wakeup' file from here. Also in principle, we are not
           supposed to change the wakeup policy, which is purely a userspace
           decision. */
        // if (udev->actconfig->desc.bmAttributes & USB_CONFIG_ATT_WAKEUP)
        // intf->needs_remote_wakeup = 1;
#endif
        usb_get_dev(udev);
        LEAVE();
        return 0;
    } else {
        PRINTM(MINFO, "Discard the Probe request\n");
        PRINTM(MINFO, "VID = 0x%X PID = 0x%X\n",
               woal_cpu_to_le16(udev->descriptor.idVendor),
               woal_cpu_to_le16(udev->descriptor.idProduct));
    }
  error:
    /* Reset device */
    if (usb_cardp->card_type == CARD_TYPE_USB8782)
        usb_reset_device(udev);
    kfree(usb_cardp);
    usb_cardp = NULL;
    LEAVE();
    return (-ENXIO);
}

/**
 *  @brief Free resource and cleanup
 *
 *  @param intf		Pointer to usb_interface
 *
 *  @return 	   	N/A
 */
static void
woal_usb_disconnect(struct usb_interface *intf)
{
    struct usb_card_rec *cardp = usb_get_intfdata(intf);
    moal_handle *phandle = NULL;
    ENTER();
    if (!cardp || !cardp->phandle) {
        PRINTM(MERROR, "Card or phandle is not valid\n");
        LEAVE();
        return;
    }
    phandle = (moal_handle *) cardp->phandle;

    /*
     * Update Surprise removed to TRUE
     *  Free all the URB's allocated
     */
    phandle->surprise_removed = MTRUE;

    /* Card is removed and we can call wlan_remove_card */
    PRINTM(MINFO, "Call remove card\n");
    woal_remove_card(cardp);

    /* Unlink and free urb */
    woal_usb_free(cardp);

    /* Reset device */
    if (cardp->card_type == CARD_TYPE_USB8782)
        usb_reset_device(cardp->udev);
    usb_set_intfdata(intf, NULL);
    usb_put_dev(interface_to_usbdev(intf));
    kfree(cardp);
    LEAVE();
    return;
}

/**
 *  @brief Handle suspend
 *
 *  @param intf		  Pointer to usb_interface
 *  @param message	  Pointer to pm_message_t structure
 *
 *  @return 	   	  MLAN_STATUS_SUCCESS
 */
static int
woal_usb_suspend(struct usb_interface *intf, pm_message_t message)
{
    struct usb_card_rec *cardp = usb_get_intfdata(intf);
    moal_handle *handle = NULL;
    int i;

    ENTER();

    if (!cardp || !cardp->phandle) {
        PRINTM(MERROR, "Card or moal_handle structure is not valid\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }
    handle = cardp->phandle;
    if (handle->is_suspended == MTRUE) {
        PRINTM(MWARN, "Device already suspended\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }

    /* Enable Host Sleep */
    woal_enable_hs(woal_get_priv(handle, MLAN_BSS_ROLE_ANY));

    /* Indicate device suspended */
    /* The flag must be set here before the usb_kill_urb() calls. Reason: In
       the complete handlers, urb->status(= -ENOENT) and 'is_suspended' flag is
       used in combination to distinguish between a suspended state and a
       'disconnect' one. */
    handle->is_suspended = MTRUE;
    for (i = 0; i < handle->priv_num; i++)
        netif_carrier_off(handle->priv[i]->netdev);

    /* Unlink Rx cmd URB */
    if (atomic_read(&cardp->rx_cmd_urb_pending) && cardp->rx_cmd.urb) {
        usb_kill_urb(cardp->rx_cmd.urb);
    }
    /* Unlink Rx data URBs */
    if (atomic_read(&cardp->rx_data_urb_pending)) {
        for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
            if (cardp->rx_data_list[i].urb) {
                usb_kill_urb(cardp->rx_data_list[i].urb);
                usb_init_urb(cardp->rx_data_list[i].urb);
            }
        }
    }

    /* Unlink Tx data URBs */
    for (i = 0; i < MVUSB_TX_HIGH_WMARK; i++) {
        if (cardp->tx_data_list[i].urb) {
            usb_kill_urb(cardp->tx_data_list[i].urb);
        }
    }
    /* Unlink Tx cmd URB */
    if (cardp->tx_cmd.urb) {
        usb_kill_urb(cardp->tx_cmd.urb);
    }

    handle->suspend_wait_q_woken = MTRUE;
    wake_up_interruptible(&handle->suspend_wait_q);

    LEAVE();
    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Handle resume
 *
 *  @param intf		  Pointer to usb_interface
 *
 *  @return 	   	  MLAN_STATUS_SUCCESS
 */
static int
woal_usb_resume(struct usb_interface *intf)
{
    struct usb_card_rec *cardp = usb_get_intfdata(intf);
    moal_handle *handle = NULL;
    int i;

    ENTER();

    if (!cardp || !cardp->phandle) {
        PRINTM(MERROR, "Card or adapter structure is not valid\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }
    handle = cardp->phandle;

    if (handle->is_suspended == MFALSE) {
        PRINTM(MWARN, "Device already resumed\n");
        LEAVE();
        return MLAN_STATUS_SUCCESS;
    }

    /* Indicate device resumed. The netdev queue will be resumed only after
       the urbs have been resubmitted */
    handle->is_suspended = MFALSE;

    if (!atomic_read(&cardp->rx_data_urb_pending)) {
        /* Submit multiple Rx data URBs */
        woal_usb_submit_rx_data_urbs(handle);
    }
    if (!atomic_read(&cardp->rx_cmd_urb_pending)) {
        if ((cardp->rx_cmd.pmbuf =
             woal_alloc_mlan_buffer(handle, MLAN_RX_CMD_BUF_SIZE)))
            woal_usb_submit_rx_urb(&cardp->rx_cmd, MLAN_RX_CMD_BUF_SIZE);
    }

    for (i = 0; i < handle->priv_num; i++)
        if (handle->priv[i]->media_connected == MTRUE)
            netif_carrier_on(handle->priv[i]->netdev);

    /* Disable Host Sleep */
    if (handle->hs_activated)
        woal_cancel_hs(woal_get_priv(handle, MLAN_BSS_ROLE_ANY), MOAL_NO_WAIT);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#ifdef CONFIG_PM
    /* Resume handler may be called due to remote wakeup, force to exit suspend
       anyway */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
    cardp->udev->autosuspend_disabled = 1;
#else
    cardp->udev->dev.power.runtime_auto = 0;
#endif /* < 2.6.35 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
    cardp->udev->autoresume_disabled = 0;
#endif /* < 2.6.33 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)
    atomic_inc(&(cardp->udev)->dev.power.usage_count);
#endif /* >= 2.6.34 */
#endif /* CONFIG_PM */
#endif /* >= 2.6.24 */

    LEAVE();
    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function initialize the tx URBs
 *
 *  @param handle 	Pointer to moal_handle structure
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_usb_tx_init(moal_handle * handle)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    int i;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    cardp->tx_cmd.handle = handle;
    cardp->tx_cmd.ep = cardp->tx_cmd_ep;
    /* Allocate URB for command */
    if (!(cardp->tx_cmd.urb = usb_alloc_urb(0, GFP_KERNEL))) {
        PRINTM(MERROR, "Tx command URB allocation failed\n");
        ret = MLAN_STATUS_FAILURE;
        goto init_exit;
    }

    cardp->tx_data_ix = 0;
    for (i = 0; i < MVUSB_TX_HIGH_WMARK; i++) {
        cardp->tx_data_list[i].handle = handle;
        cardp->tx_data_list[i].ep = cardp->tx_data_ep;
        /* Allocate URB for data */
        if (!(cardp->tx_data_list[i].urb = usb_alloc_urb(0, GFP_KERNEL))) {
            PRINTM(MERROR, "Tx data URB allocation failed\n");
            ret = MLAN_STATUS_FAILURE;
            goto init_exit;
        }
    }

  init_exit:
    LEAVE();
    return ret;
}

/**
 *  @brief This function submits the rx data URBs
 *
 *  @param handle 	Pointer to moal_handle structure
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_usb_submit_rx_data_urbs(moal_handle * handle)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    int i;
    mlan_status ret = MLAN_STATUS_FAILURE;
    t_u16 buffer_len = MLAN_USB_RX_DATA_BUF_SIZE;

    ENTER();

    if (cardp->rx_deaggr_ctrl.enable) {
        buffer_len = cardp->rx_deaggr_ctrl.aggr_max;
        if (cardp->rx_deaggr_ctrl.aggr_mode == MLAN_USB_AGGR_MODE_NUM) {
            buffer_len *=
                MAX(MLAN_USB_MAX_PKT_SIZE, cardp->rx_deaggr_ctrl.aggr_align);
            buffer_len = MAX(buffer_len, MLAN_USB_RX_DATA_BUF_SIZE);
        }
    }

    for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
        /* Submit Rx data URB */
        if (!cardp->rx_data_list[i].pmbuf) {
            if (woal_usb_submit_rx_urb(&cardp->rx_data_list[i], buffer_len))
                continue;
        }
        ret = MLAN_STATUS_SUCCESS;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief This function initialize the rx URBs and submit them
 *
 *  @param handle 	Pointer to moal_handle structure
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_usb_rx_init(moal_handle * handle)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    int i;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    cardp->rx_cmd.handle = handle;
    cardp->rx_cmd.ep = cardp->rx_cmd_ep;
    /* Allocate URB for command/event */
    if (!(cardp->rx_cmd.urb = usb_alloc_urb(0, GFP_KERNEL))) {
        PRINTM(MERROR, "Rx command URB allocation failed\n");
        ret = MLAN_STATUS_FAILURE;
        goto init_exit;
    }

    if ((cardp->rx_cmd.pmbuf = woal_alloc_mlan_buffer(handle,
                                                      MLAN_RX_CMD_BUF_SIZE))) {
        /* Submit Rx command URB */
        if (woal_usb_submit_rx_urb(&cardp->rx_cmd, MLAN_RX_CMD_BUF_SIZE)) {
            ret = MLAN_STATUS_FAILURE;
            goto init_exit;
        }
    }
    for (i = 0; i < MVUSB_RX_DATA_URB; i++) {
        cardp->rx_data_list[i].handle = handle;
        cardp->rx_data_list[i].ep = cardp->rx_data_ep;
        /* Allocate URB for data */
        if (!(cardp->rx_data_list[i].urb = usb_alloc_urb(0, GFP_KERNEL))) {
            PRINTM(MERROR, "Rx data URB allocation failed\n");
            ret = MLAN_STATUS_FAILURE;
            goto init_exit;
        }
    }
    if ((ret = woal_usb_submit_rx_data_urbs(handle))) {
        PRINTM(MERROR, "Rx data URB submission failed\n");
        goto init_exit;
    }

  init_exit:
    LEAVE();
    return ret;
}

/**
 *  @brief  This function downloads data blocks to device
 *
 *  @param handle	Pointer to moal_handle structure
 *  @param pmbuf	Pointer to mlan_buffer structure
 *  @param ep		Endpoint to send
 *  @param timeout 	Timeout value in milliseconds (if 0 the wait is forever)
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_write_data_sync(moal_handle * handle, mlan_buffer * pmbuf, t_u8 ep,
                     t_u32 timeout)
{
    struct usb_card_rec *cardp = handle->card;
    t_u8 *data = (t_u8 *) (pmbuf->pbuf + pmbuf->data_offset);
    t_u32 length = pmbuf->data_len;
    int actual_length;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    if (length % cardp->bulk_out_maxpktsize == 0)
        length++;
    /* Send the data block */
    if ((ret = usb_bulk_msg(cardp->udev, usb_sndbulkpipe(cardp->udev,
                                                         ep), (t_u8 *) data,
                            length, &actual_length, timeout))) {
        PRINTM(MERROR, "usb_blk_msg for send failed, ret %d\n", ret);
        ret = MLAN_STATUS_FAILURE;
    }
    pmbuf->data_len = actual_length;

    return ret;
}

/**
 *  @brief  This function read data blocks to device
 *
 *  @param handle	Pointer to moal_handle structure
 *  @param pmbuf	Pointer to mlan_buffer structure
 *  @param ep		Endpoint to receive
 *  @param timeout 	Timeout value in milliseconds (if 0 the wait is forever)
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_read_data_sync(moal_handle * handle, mlan_buffer * pmbuf, t_u8 ep,
                    t_u32 timeout)
{
    struct usb_card_rec *cardp = handle->card;
    t_u8 *data = (t_u8 *) (pmbuf->pbuf + pmbuf->data_offset);
    t_u32 buf_len = pmbuf->data_len;
    int actual_length;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    ENTER();
    /* Receive the data response */
    if ((ret = usb_bulk_msg(cardp->udev, usb_rcvbulkpipe(cardp->udev,
                                                         ep), data,
                            buf_len, &actual_length, timeout))) {
        PRINTM(MERROR, "usb_bulk_msg failed: %d\n", ret);
        ret = MLAN_STATUS_FAILURE;
    }
    pmbuf->data_len = actual_length;
    DBG_HEXDUMP(MIF_D, "read sync", data, actual_length);
    LEAVE();
    return ret;
}

/**
 *  @brief  This function downloads data/command packet to device
 *
 *  @param handle	Pointer to moal_handle structure
 *  @param pmbuf	Pointer to mlan_buffer structure
 *  @param ep		Endpoint to send
 *
 *  @return 	   	MLAN_STATUS_PENDING or MLAN_STATUS_FAILURE or MLAN_STATUS_RESOURCE
 */
mlan_status
woal_write_data_async(moal_handle * handle, mlan_buffer * pmbuf, t_u8 ep)
{
    struct usb_card_rec *cardp = handle->card;
    urb_context *context = NULL;
    t_u8 *data = (t_u8 *) (pmbuf->pbuf + pmbuf->data_offset);
    struct urb *tx_urb = NULL;
    mlan_status ret = MLAN_STATUS_SUCCESS;
    t_u32 data_len = pmbuf->data_len;

    ENTER();

    /* Check if device is removed */
    if (handle->surprise_removed) {
        PRINTM(MERROR, "Device removed\n");
        ret = MLAN_STATUS_FAILURE;
        goto tx_ret;
    }

    if ((ep == cardp->tx_data_ep) &&
        (atomic_read(&cardp->tx_data_urb_pending) >= MVUSB_TX_HIGH_WMARK)) {
        ret = MLAN_STATUS_RESOURCE;
        goto tx_ret;
    }

    PRINTM(MINFO, "woal_write_data_async: ep=%d\n", ep);

    if (ep == cardp->tx_cmd_ep) {
        context = &cardp->tx_cmd;
    } else {
        if (cardp->tx_data_ix >= MVUSB_TX_HIGH_WMARK)
            cardp->tx_data_ix = 0;
        context = &cardp->tx_data_list[cardp->tx_data_ix++];
    }

    context->handle = handle;
    context->ep = ep;
    context->pmbuf = pmbuf;
    tx_urb = context->urb;

    /*
     * Use USB API usb_fill_bulk_urb() to set the
     * configuration information of the Tx bulk URB
     * and initialize the Tx callback
     */
    usb_fill_bulk_urb(tx_urb, cardp->udev,
                      usb_sndbulkpipe(cardp->udev, ep),
                      data, data_len, woal_usb_tx_complete, (void *) context);
    tx_urb->transfer_flags |= URB_ZERO_PACKET;
    if (ep == cardp->tx_cmd_ep)
        atomic_inc(&cardp->tx_cmd_urb_pending);
    else
        atomic_inc(&cardp->tx_data_urb_pending);
    if (usb_submit_urb(tx_urb, GFP_ATOMIC)) {
        /* Submit URB failure */
        PRINTM(MERROR, "Submit EP %d Tx URB failed: %d\n", ep, ret);
        if (ep == cardp->tx_cmd_ep)
            atomic_dec(&cardp->tx_cmd_urb_pending);
        else {
            atomic_dec(&cardp->tx_data_urb_pending);
            if (cardp->tx_data_ix)
                cardp->tx_data_ix--;
            else
                cardp->tx_data_ix = MVUSB_TX_HIGH_WMARK;
        }
        ret = MLAN_STATUS_FAILURE;
    } else {
        if (ep == cardp->tx_data_ep &&
            (atomic_read(&cardp->tx_data_urb_pending) == MVUSB_TX_HIGH_WMARK))
            ret = MLAN_STATUS_PRESOURCE;
        else
            ret = MLAN_STATUS_SUCCESS;
    }

  tx_ret:

    if (!ret)
        ret = MLAN_STATUS_PENDING;

    LEAVE();
    return ret;
}

/**
 *  @brief  This function register usb device and initialize parameter
 *
 *  @param handle	Pointer to moal_handle structure
 *
 *  @return 	   	MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_register_dev(moal_handle * handle)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;
    mlan_status ret = MLAN_STATUS_SUCCESS;

    ENTER();

    cardp->phandle = handle;
    handle->hotplug_device = &(cardp->udev->dev);
    LEAVE();
    return ret;
}

/**
 *  @brief This function registers driver.
 *
 *  @return 	 MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
mlan_status
woal_bus_register(void)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    ENTER();

    if (skip_fwdnld) {
        woal_usb_driver.id_table = woal_usb_table_skip_fwdnld;
    }
    /*
     * API registers the Marvell USB driver
     * to the USB system
     */
    if (usb_register(&woal_usb_driver)) {
        PRINTM(MFATAL, "USB Driver Registration Failed \n");
        ret = MLAN_STATUS_FAILURE;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief This function removes usb driver.
 *
 *  @return 	   	N/A
 */
void
woal_bus_unregister(void)
{
    ENTER();
    /* API unregisters the driver from USB subsystem */
    usb_deregister(&woal_usb_driver);
    LEAVE();
    return;
}

/**
 *  @brief This function makes USB device to suspend.
 *
 *  @param handle  A pointer to moal_handle structure
 *
 *  @return             0 --success, otherwise fail
 */
int
woal_enter_usb_suspend(moal_handle * handle)
{
    struct usb_device *udev = ((struct usb_card_rec *) (handle->card))->udev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    struct usb_interface *intf = ((struct usb_card_rec *) (handle->card))->intf;
#endif /* < 2.6.34 */
#endif /* >= 2.6.24 */

    ENTER();

    PRINTM(MIOCTL, "USB suspend ioctl (state %d)\n", udev->state);

    if (handle->is_suspended == MTRUE) {
        PRINTM(MERROR, "Device already suspended\n");
        LEAVE();
        return -EFAULT;
    }

    handle->suspend_wait_q_woken = MFALSE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#ifdef CONFIG_PM
    /* Enter into USB suspend */
    usb_lock_device(udev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38)
    udev->autosuspend_delay = 0;        /* Autosuspend delay in jiffies */
#else
    pm_runtime_set_autosuspend_delay(&udev->dev, 0);    /* Autosuspend delay in
                                                           jiffies */
#endif /* < 2.6.38 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    udev->autosuspend_disabled = 0;     /* /sys/bus/usb/devices/.../power/level
                                           < auto */
#endif /* < 2.6.34 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
    udev->autoresume_disabled = 0;
#endif /* < 2.6.33 */
    usb_unlock_device(udev);
#endif /* CONFIG_PM */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    intf->pm_usage_cnt = 1;
#else
    atomic_set(&intf->pm_usage_cnt, 1);
#endif /* < 2.6.32 */
    usb_autopm_put_interface(intf);
#else
#ifdef CONFIG_PM
    usb_lock_device(udev);
    atomic_set(&udev->dev.power.usage_count, 1);
    usb_enable_autosuspend(udev);
    usb_unlock_device(udev);
#endif /* CONFIG_PM */
#endif /* < 2.6.34 */
#endif /* >= 2.6.24 */

    /* Wait for suspend to complete */
    wait_event_interruptible(handle->suspend_wait_q,
                             handle->suspend_wait_q_woken);

    LEAVE();
    return 0;
}

/**
 *  @brief This function makes USB device to resume.
 *
 *  @param handle  A pointer to moal_handle structure
 *
 *  @return             0 --success, otherwise fail
 */
int
woal_exit_usb_suspend(moal_handle * handle)
{
    struct usb_device *udev = ((struct usb_card_rec *) (handle->card))->udev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    struct usb_interface *intf = ((struct usb_card_rec *) (handle->card))->intf;
#endif /* < 2.6.34 */
#endif /* >= 2.6.24 */

    ENTER();

    PRINTM(MIOCTL, "USB resume ioctl (state %d)\n", udev->state);

    if (handle->is_suspended == MFALSE || udev->state != USB_STATE_SUSPENDED) {
        PRINTM(MERROR, "Device already resumed\n");
        LEAVE();
        return -EFAULT;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#ifdef CONFIG_PM
    /* Exit from USB suspend */
    usb_lock_device(udev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    udev->autosuspend_disabled = 1;     /* /sys/bus/usb/devices/.../power/level
                                           < on */
#endif /* < 2.6.34 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33)
    udev->autoresume_disabled = 0;
#endif /* < 2.6.33 */
    usb_unlock_device(udev);
#endif /* CONFIG_PM */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
    intf->pm_usage_cnt = 0;
#else
    atomic_set(&intf->pm_usage_cnt, 0);
#endif /* < 2.6.32 */
    usb_autopm_get_interface(intf);
#else
#ifdef CONFIG_PM
    usb_lock_device(udev);
    atomic_set(&udev->dev.power.usage_count, 0);
    usb_disable_autosuspend(udev);
    usb_unlock_device(udev);
#endif /* CONFIG_PM */
#endif /* < 2.6.34 */
#endif /* >= 2.6.24 */

    LEAVE();
    return 0;
}

/**
 *  @brief This function will submit rx urb.
 *
 *  @param handle   Pointer to moal_handle
 *  @param ep       Endpoint to re-submit urb
 *
 *  @return 	   	N/A
 */
void
woal_submit_rx_urb(moal_handle * handle, t_u8 ep)
{
    struct usb_card_rec *cardp = (struct usb_card_rec *) handle->card;

    ENTER();

    if ((ep == cardp->rx_cmd_ep) && (!atomic_read(&cardp->rx_cmd_urb_pending))) {
        woal_usb_submit_rx_urb(&cardp->rx_cmd, MLAN_RX_CMD_BUF_SIZE);
    }

    LEAVE();
    return;
}
