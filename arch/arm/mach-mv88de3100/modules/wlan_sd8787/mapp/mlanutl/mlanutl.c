/** @file  mlanutl.c
  *
  * @brief Program to control parameters in the mlandriver
  *
  * Usage: mlanutl mlanX cmd [...]
  *
  * (C) Copyright 2011-2012 Marvell International Ltd. All Rights Reserved
  *
  * MARVELL CONFIDENTIAL
  * The source code contained or described herein and all documents related to
  * the source code ("Material") are owned by Marvell International Ltd or its
  * suppliers or licensors. Title to the Material remains with Marvell International Ltd
  * or its suppliers and licensors. The Material contains trade secrets and
  * proprietary and confidential information of Marvell or its suppliers and
  * licensors. The Material is protected by worldwide copyright and trade secret
  * laws and treaty provisions. No part of the Material may be used, copied,
  * reproduced, modified, published, uploaded, posted, transmitted, distributed,
  * or disclosed in any way without Marvell's prior express written permission.
  *
  * No license under any patent, copyright, trade secret or other intellectual
  * property right is granted to or conferred upon you by disclosure or delivery
  * of the Materials, either expressly, by implication, inducement, estoppel or
  * otherwise. Any license under such intellectual property rights must be
  * express and approved by Marvell in writing.
  *
  */
/************************************************************************
Change log:
     11/04/2011: initial version
************************************************************************/

#include    "mlanutl.h"

/** mlanutl version number */
#define MLANUTL_VER "M1.1"

/** Initial number of total private ioctl calls */
#define IW_INIT_PRIV_NUM    128
/** Maximum number of total private ioctl calls supported */
#define IW_MAX_PRIV_NUM     1024

/** Marvell private command identifier */
#define CMD_MARVELL         "MRVL_CMD"
/********************************************************
        Local Variables
********************************************************/
#define BAND_B       (1U << 0)
#define BAND_G       (1U << 1)
#define BAND_A       (1U << 2)
#define BAND_GN      (1U << 3)
#define BAND_AN      (1U << 4)

static t_s8 *band[] = {
    "B",
    "G",
    "A",
    "GN",
    "AN",
    "GAC",
    "AAC",
};

static t_s8 *adhoc_bw[] = {
    "20 MHz",
    "HT 40 MHz above",
    " ",
    "HT 40 MHz below",
    "VHT 80 MHz",
};

#ifdef DEBUG_LEVEL1
#define MMSG        MBIT(0)
#define MFATAL      MBIT(1)
#define MERROR      MBIT(2)
#define MDATA       MBIT(3)
#define MCMND       MBIT(4)
#define MEVENT      MBIT(5)
#define MINTR       MBIT(6)
#define MIOCTL      MBIT(7)

#define MDAT_D      MBIT(16)
#define MCMD_D      MBIT(17)
#define MEVT_D      MBIT(18)
#define MFW_D       MBIT(19)
#define MIF_D       MBIT(20)

#define MENTRY      MBIT(28)
#define MWARN       MBIT(29)
#define MINFO       MBIT(30)
#define MHEX_DUMP   MBIT(31)
#endif

static int process_version(int argc, char *argv[]);
static int process_bandcfg(int argc, char *argv[]);
static int process_hostcmd(int argc, char *argv[]);
static int process_httxcfg(int argc, char *argv[]);
static int process_htcapinfo(int argc, char *argv[]);
static int process_addbapara(int argc, char *argv[]);
static int process_aggrpriotbl(int argc, char *argv[]);
static int process_addbareject(int argc, char *argv[]);
static int process_delba(int argc, char *argv[]);
static int process_datarate(int argc, char *argv[]);
static int process_txratecfg(int argc, char *argv[]);
static int process_passphrase(int argc, char *argv[]);
static int process_deauth(int argc, char *argv[]);
#ifdef UAP_SUPPORT
static int process_getstalist(int argc, char *argv[]);
#endif
#if defined(WIFI_DIRECT_SUPPORT)
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
static int process_bssrole(int argc, char *argv[]);
#endif
#endif
#ifdef STA_SUPPORT
static int process_setuserscan(int argc, char *argv[]);
static int process_getscantable(int argc, char *argv[]);
#endif
static int process_deepsleep(int argc, char *argv[]);
static int process_ipaddr(int argc, char *argv[]);
static int process_countrycode(int argc, char *argv[]);
#ifdef STA_SUPPORT
static int process_listeninterval(int argc, char *argv[]);
#endif
#ifdef DEBUG_LEVEL1
static int process_drvdbg(int argc, char *argv[]);
#endif
static int process_hscfg(int argc, char *argv[]);
static int process_hssetpara(int argc, char *argv[]);
static int process_wakeupresaon(int argc, char *argv[]);
static int process_scancfg(int argc, char *argv[]);

struct command_node command_list[] = {
    {"version", process_version},
    {"bandcfg", process_bandcfg},
    {"hostcmd", process_hostcmd},
    {"httxcfg", process_httxcfg},
    {"htcapinfo", process_htcapinfo},
    {"addbapara", process_addbapara},
    {"aggrpriotbl", process_aggrpriotbl},
    {"addbareject", process_addbareject},
    {"delba", process_delba},
    {"getdatarate", process_datarate},
    {"txratecfg", process_txratecfg},
    {"passphrase", process_passphrase},
    {"deauth", process_deauth},
#ifdef UAP_SUPPORT
    {"getstalist", process_getstalist},
#endif
#if defined(WIFI_DIRECT_SUPPORT)
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
    {"bssrole", process_bssrole},
#endif
#endif
#ifdef STA_SUPPORT
    {"setuserscan", process_setuserscan},
    {"getscantable", process_getscantable},
#endif
    {"deepsleep", process_deepsleep},
    {"ipaddr", process_ipaddr},
    {"countrycode", process_countrycode},
#ifdef STA_SUPPORT
    {"listeninterval", process_listeninterval},
#endif
#ifdef DEBUG_LEVEL1
    {"drvdbg", process_drvdbg},
#endif
    {"hscfg", process_hscfg},
    {"hssetpara", process_hssetpara},
    {"wakeupreason", process_wakeupresaon},
    {"scancfg", process_scancfg},
};

static t_s8 *usage[] = {
    "Usage: ",
    "   mlanutl -v  (version)",
    "   mlanutl <ifname> <cmd> [...]",
    "   where",
    "   ifname : wireless network interface name, such as mlanX or uapX",
    "   cmd : version, bandcfg",
    "         hostcmd",
    "         httxcfg",
    "         htcapinfo",
    "         addbapara",
    "         aggrpriotbl",
    "         addbareject",
    "         delba",
    "         getdatarate",
    "         txratecfg",
    "         MACADDR, SETUSPENDOPT, BTCOEXMODE, BTCOEXSCAN-START, BTCOEXSCAN-STOP",
#ifdef STA_SUPPORT
    "         RSSI, LINKSPEED, GETPOWER, SCAN-ACTIVE, SCAN-PASSIVE",
    "         COUNTRY",
    "         POWERMODE, CSCAN, GETBAND, SETBAND, BGSCAN-START, BGSCAN-CONFIG. BGSCAN-STOP",
    "         RXFILTER-START, RXFILTER-STOP, RXFILTER-ADD, RXFILTER-REMOVE, QOSINFO",
    "         SLEEPPD, SET_AP_WPS_P2P_IE, P2P_DEV_ADDR, P2P_GET_NOA",
#endif
    "         START, STOP",
#ifdef UAP_SUPPORT
    "         AP_BSS_START, AP_BSS_STOP, AP_SET_CFG, WL_FW_RELOAD, AP_GET_STA_LIST",
#endif
    "         passphrase",
    "         deauth",
#ifdef UAP_SUPPORT
    "         getstalist",
#endif
#if defined(WIFI_DIRECT_SUPPORT)
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
    "         bssrole",
#endif
#endif
#ifdef STA_SUPPORT
    "         setuserscan",
    "         getscantable",
#endif
    "         deepsleep",
    "         ipaddr",
    "         countrycode",
#ifdef STA_SUPPORT
    "         listeninterval",
#endif
#ifdef DEBUG_LEVEL1
    "         drvdbg",
#endif
    "         hscfg",
    "         hssetpara",
    "         scancfg",
};

/** Socket */
t_s32 sockfd;
/** Device name */
t_s8 dev_name[IFNAMSIZ + 1];

/********************************************************
        Global Variables
********************************************************/

/********************************************************
        Local Functions
********************************************************/
/**
 *  @brief Convert char to hex integer
 *
 *  @param chr      Char to convert
 *  @return         Hex integer or 0
 */
int
hexval(t_s32 chr)
{
    if (chr >= '0' && chr <= '9')
        return chr - '0';
    if (chr >= 'A' && chr <= 'F')
        return chr - 'A' + 10;
    if (chr >= 'a' && chr <= 'f')
        return chr - 'a' + 10;

    return 0;
}

/**
 *  @brief Hump hex data
 *
 *  @param prompt   A pointer prompt buffer
 *  @param p        A pointer to data buffer
 *  @param len      The len of data buffer
 *  @param delim    Delim char
 *  @return         Hex integer
 */
t_void
hexdump(t_s8 * prompt, t_void * p, t_s32 len, t_s8 delim)
{
    t_s32 i;
    t_u8 *s = p;

    if (prompt) {
        printf("%s: len=%d\n", prompt, (int) len);
    }
    for (i = 0; i < len; i++) {
        if (i != len - 1)
            printf("%02x%c", *s++, delim);
        else
            printf("%02x\n", *s);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

/**
 *  @brief Convert char to hex integer
 *
 *  @param chr      Char
 *  @return         Hex integer
 */
t_u8
hexc2bin(t_s8 chr)
{
    if (chr >= '0' && chr <= '9')
        chr -= '0';
    else if (chr >= 'A' && chr <= 'F')
        chr -= ('A' - 10);
    else if (chr >= 'a' && chr <= 'f')
        chr -= ('a' - 10);

    return chr;
}

/**
 *  @brief Convert string to hex integer
 *
 *  @param s        A pointer string buffer
 *  @return         Hex integer
 */
t_u32
a2hex(t_s8 * s)
{
    t_u32 val = 0;

    if (!strncasecmp("0x", s, 2)) {
        s += 2;
    }

    while (*s && isxdigit(*s)) {
        val = (val << 4) + hexc2bin(*s++);
    }

    return val;
}

/*
 *  @brief Convert String to integer
 *
 *  @param value    A pointer to string
 *  @return         Integer
 */
t_u32
a2hex_or_atoi(t_s8 * value)
{
    if (value[0] == '0' && (value[1] == 'X' || value[1] == 'x')) {
        return a2hex(value + 2);
    } else if (isdigit(*value)) {
        return atoi(value);
    } else {
        return *value;
    }
}

/**
 *  @brief Convert string to hex
 *
 *  @param ptr      A pointer to data buffer
 *  @param chr      A pointer to return integer
 *  @return         A pointer to next data field
 */
t_s8 *
convert2hex(t_s8 * ptr, t_u8 * chr)
{
    t_u8 val;

    for (val = 0; *ptr && isxdigit(*ptr); ptr++) {
        val = (val * 16) + hexval(*ptr);
    }

    *chr = val;

    return ptr;
}

/**
 *  @brief Check the Hex String
 *  @param s  A pointer to the string
 *  @return   MLAN_STATUS_SUCCESS --HexString, MLAN_STATUS_FAILURE --not HexString
 */
int
ishexstring(t_s8 * s)
{
    int ret = MLAN_STATUS_FAILURE;
    t_s32 tmp;

    if (!strncasecmp("0x", s, 2)) {
        s += 2;
    }
    while (*s) {
        tmp = toupper(*s);
        if (((tmp >= 'A') && (tmp <= 'F')) || ((tmp >= '0') && (tmp <= '9'))) {
            ret = MLAN_STATUS_SUCCESS;
        } else {
            ret = MLAN_STATUS_FAILURE;
            break;
        }
        s++;
    }

    return ret;
}

/**
 *  @brief Convert String to Integer
 *  @param buf      A pointer to the string
 *  @return         Integer
 */
int
atoval(t_s8 * buf)
{
    if (!strncasecmp(buf, "0x", 2))
        return a2hex(buf + 2);
    else if (!ishexstring(buf))
        return a2hex(buf);
    else
        return atoi(buf);
}

/**
 *  @brief Display usage
 *
 *  @return       NA
 */
static t_void
display_usage(t_void)
{
    t_u32 i;
    for (i = 0; i < NELEMENTS(usage); i++)
        fprintf(stderr, "%s\n", usage[i]);
}

/**
 *  @brief Find and execute command
 *
 *  @param argc     Number of arguments
 *  @param argv     A pointer to arguments array
 *  @return         MLAN_STATUS_SUCCESS for success, otherwise failure
 */
static int
process_command(int argc, char *argv[])
{
    int i = 0, ret = MLAN_STATUS_FAILURE;
    struct command_node *node = NULL;

    for (i = 0; i < NELEMENTS(command_list); i++) {
        node = &command_list[i];
        if (!strcasecmp(node->name, argv[2])) {
            ret = node->handler(argc, argv);
            break;
        }
    }

    return ret;
}

/**
 *  @brief Prepare command buffer
 *  @param buffer   Command buffer to be filled
 *  @param cmd      Command id
 *  @param num      Number of arguments
 *  @param args     Arguments list
 *  @return         MLAN_STATUS_SUCCESS
 */
static int
prepare_buffer(t_u8 * buffer, t_s8 * cmd, t_u32 num, char *args[])
{
    t_u8 *pos = NULL;
    unsigned int i = 0;

    memset(buffer, 0, BUFFER_LENGTH);

    /* Flag it for our use */
    pos = buffer;
    strncpy((char *) pos, CMD_MARVELL, strlen(CMD_MARVELL));
    pos += (strlen(CMD_MARVELL));

    /* Insert command */
    strncpy((char *) pos, (char *) cmd, strlen(cmd));
    pos += (strlen(cmd));

    /* Insert arguments */
    for (i = 0; i < num; i++) {
        strncpy((char *) pos, args[i], strlen(args[i]));
        pos += strlen(args[i]);
        if (i < (num - 1)) {
            strncpy((char *) pos, " ", strlen(" "));
            pos += 1;
        }
    }

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process version
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_version(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], 0, NULL);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: version fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    printf("Version string received: %s\n", cmd->buf);

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process band configuration
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_bandcfg(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    int i;
    struct eth_priv_cmd *cmd = NULL;
    struct eth_priv_bandcfg *bandcfg = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: bandcfg fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    bandcfg = (struct eth_priv_bandcfg *) (cmd->buf);
    if (argc == 3) {
        /* GET operation */
        printf("Band Configuration:\n");
        printf("  Infra Band: %d (", (int) bandcfg->config_bands);
        for (i = 0; i < 6; i++) {
            if ((bandcfg->config_bands >> i) & 0x1)
                printf(" %s", band[i]);
        }
        printf(" )\n");
        printf("  Adhoc Start Band: %d (", (int) bandcfg->adhoc_start_band);
        for (i = 0; i < 6; i++) {
            if ((bandcfg->adhoc_start_band >> i) & 0x1)
                printf(" %s", band[i]);
        }
        printf(" )\n");
        printf("  Adhoc Start Channel: %d\n", (int) bandcfg->adhoc_channel);
        printf("  Adhoc Band Width: %d (%s)\n",
               (int) bandcfg->adhoc_chan_bandwidth,
               adhoc_bw[bandcfg->adhoc_chan_bandwidth]);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Get one line from the File
 *
 *  @param fp       File handler
 *  @param str      Storage location for data.
 *  @param size     Maximum number of characters to read.
 *  @param lineno   A pointer to return current line number
 *  @return         returns string or NULL
 */
char *
mlan_config_get_line(FILE * fp, t_s8 * str, t_s32 size, int *lineno)
{
    char *start, *end;
    int out, next_line;

    if (!fp || !str)
        return NULL;

    do {
      read_line:
        if (!fgets(str, size, fp))
            break;
        start = str;
        start[size - 1] = '\0';
        end = start + strlen(str);
        (*lineno)++;

        out = 1;
        while (out && (start < end)) {
            next_line = 0;
            /* Remove empty lines and lines starting with # */
            switch (start[0]) {
            case ' ':          /* White space */
            case '\t':         /* Tab */
                start++;
                break;
            case '#':
            case '\n':
            case '\0':
                next_line = 1;
                break;
            case '\r':
                if (start[1] == '\n')
                    next_line = 1;
                else
                    start++;
                break;
            default:
                out = 0;
                break;
            }
            if (next_line)
                goto read_line;
        }

        /* Remove # comments unless they are within a double quoted string.
           Remove trailing white space. */
        if ((end = strstr(start, "\""))) {
            if (!(end = strstr(end + 1, "\"")))
                end = start;
        } else
            end = start;

        if ((end = strstr(end + 1, "#")))
            *end-- = '\0';
        else
            end = start + strlen(start) - 1;

        out = 1;
        while (out && (start < end)) {
            switch (*end) {
            case ' ':          /* White space */
            case '\t':         /* Tab */
            case '\n':
            case '\r':
                *end = '\0';
                end--;
                break;
            default:
                out = 0;
                break;
            }
        }

        if (start == '\0')
            continue;

        return start;
    } while (1);

    return NULL;
}

/**
 *  @brief get hostcmd data
 *
 *  @param ln           A pointer to line number
 *  @param buf          A pointer to hostcmd data
 *  @param size         A pointer to the return size of hostcmd buffer
 *  @return             MLAN_STATUS_SUCCESS
 */
static int
mlan_get_hostcmd_data(FILE * fp, int *ln, t_u8 * buf, t_u16 * size)
{
    t_s32 errors = 0, i;
    t_s8 line[256], *pos, *pos1, *pos2, *pos3;
    t_u16 len;

    while ((pos = mlan_config_get_line(fp, line, sizeof(line), ln))) {
        (*ln)++;
        if (strcmp(pos, "}") == 0) {
            break;
        }

        pos1 = strchr(pos, ':');
        if (pos1 == NULL) {
            printf("Line %d: Invalid hostcmd line '%s'\n", *ln, pos);
            errors++;
            continue;
        }
        *pos1++ = '\0';

        pos2 = strchr(pos1, '=');
        if (pos2 == NULL) {
            printf("Line %d: Invalid hostcmd line '%s'\n", *ln, pos);
            errors++;
            continue;
        }
        *pos2++ = '\0';

        len = a2hex_or_atoi(pos1);
        if (len < 1 || len > BUFFER_LENGTH) {
            printf("Line %d: Invalid hostcmd line '%s'\n", *ln, pos);
            errors++;
            continue;
        }

        *size += len;

        if (*pos2 == '"') {
            pos2++;
            if ((pos3 = strchr(pos2, '"')) == NULL) {
                printf("Line %d: invalid quotation '%s'\n", *ln, pos);
                errors++;
                continue;
            }
            *pos3 = '\0';
            memset(buf, 0, len);
            memmove(buf, pos2, MIN(strlen(pos2), len));
            buf += len;
        } else if (*pos2 == '\'') {
            pos2++;
            if ((pos3 = strchr(pos2, '\'')) == NULL) {
                printf("Line %d: invalid quotation '%s'\n", *ln, pos);
                errors++;
                continue;
            }
            *pos3 = ',';
            for (i = 0; i < len; i++) {
                if ((pos3 = strchr(pos2, ',')) != NULL) {
                    *pos3 = '\0';
                    *buf++ = (t_u8) a2hex_or_atoi(pos2);
                    pos2 = pos3 + 1;
                } else
                    *buf++ = 0;
            }
        } else if (*pos2 == '{') {
            t_u16 tlvlen = 0, tmp_tlvlen;
            mlan_get_hostcmd_data(fp, ln, buf + len, &tlvlen);
            tmp_tlvlen = tlvlen;
            while (len--) {
                *buf++ = (t_u8) (tmp_tlvlen & 0xff);
                tmp_tlvlen >>= 8;
            }
            *size += tlvlen;
            buf += tlvlen;
        } else {
            t_u32 value = a2hex_or_atoi(pos2);
            while (len--) {
                *buf++ = (t_u8) (value & 0xff);
                value >>= 8;
            }
        }
    }
    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Prepare host-command buffer
 *  @param fp       File handler
 *  @param cmd_name Command name
 *  @param buf      A pointer to comand buffer
 *  @return         MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
prepare_host_cmd_buffer(FILE * fp, char *cmd_name, t_u8 * buf)
{
    t_s8 line[256], cmdname[256], *pos, cmdcode[10];
    HostCmd_DS_GEN *hostcmd;
    t_u32 hostcmd_size = 0;
    int ln = 0;
    int cmdname_found = 0, cmdcode_found = 0;

    hostcmd = (HostCmd_DS_GEN *) (buf + sizeof(t_u32));
    hostcmd->command = 0xffff;

    snprintf(cmdname, sizeof(cmdname), "%s={", cmd_name);
    cmdname_found = 0;
    while ((pos = mlan_config_get_line(fp, line, sizeof(line), &ln))) {
        if (strcmp(pos, cmdname) == 0) {
            cmdname_found = 1;
            snprintf(cmdcode, sizeof(cmdcode), "CmdCode=");
            cmdcode_found = 0;
            while ((pos = mlan_config_get_line(fp, line, sizeof(line), &ln))) {
                if (strncmp(pos, cmdcode, strlen(cmdcode)) == 0) {
                    cmdcode_found = 1;
                    hostcmd->command = a2hex_or_atoi(pos + strlen(cmdcode));
                    hostcmd->size = S_DS_GEN;
                    mlan_get_hostcmd_data(fp, &ln, buf + hostcmd->size,
                                          &hostcmd->size);
                    break;
                }
            }
            if (!cmdcode_found) {
                fprintf(stderr, "mlanutl: CmdCode not found in conf file\n");
                return MLAN_STATUS_FAILURE;
            }
            break;
        }
    }

    if (!cmdname_found) {
        fprintf(stderr, "mlanutl: cmdname '%s' is not found in conf file\n",
                cmd_name);
        return MLAN_STATUS_FAILURE;
    }

    hostcmd->seq_num = 0;
    hostcmd->result = 0;
    hostcmd->command = cpu_to_le16(hostcmd->command);
    hostcmd->size = cpu_to_le16(hostcmd->size);

    hostcmd_size = (t_u32) (hostcmd->size);
    memcpy(buf, (t_u8 *) & hostcmd_size, sizeof(t_u32));

    return MLAN_STATUS_SUCCESS;
}

#if defined(UAP_SUPPORT)
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
#endif

/**
 *  @brief Process host_cmd response
 *  @param buf      A pointer to the response buffer
 *  @return         MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
process_host_cmd_resp(char *cmd_name, t_u8 * buf)
{
    t_u32 hostcmd_size = 0;
    HostCmd_DS_GEN *hostcmd = NULL;
    int ret = MLAN_STATUS_SUCCESS;

    buf += strlen(CMD_MARVELL) + strlen(cmd_name);
    memcpy((t_u8 *) & hostcmd_size, buf, sizeof(t_u32));
    buf += sizeof(t_u32);

    hostcmd = (HostCmd_DS_GEN *) buf;
    hostcmd->command = le16_to_cpu(hostcmd->command);
    hostcmd->size = le16_to_cpu(hostcmd->size);
    hostcmd->seq_num = le16_to_cpu(hostcmd->seq_num);
    hostcmd->result = le16_to_cpu(hostcmd->result);

    hostcmd->command &= ~HostCmd_RET_BIT;
    if (!hostcmd->result) {
        switch (hostcmd->command) {
        case HostCmd_CMD_CFG_DATA:
            {
                HostCmd_DS_802_11_CFG_DATA *pstcfgData =
                    (HostCmd_DS_802_11_CFG_DATA *) (buf + S_DS_GEN);
                pstcfgData->data_len = le16_to_cpu(pstcfgData->data_len);
                pstcfgData->action = le16_to_cpu(pstcfgData->action);

                if (pstcfgData->action == HostCmd_ACT_GEN_GET) {
                    hexdump("cfgdata", pstcfgData->data, pstcfgData->data_len,
                            ' ');
                }
                break;
            }
        case HostCmd_CMD_802_11_TPC_ADAPT_REQ:
            {
                mlan_ioctl_11h_tpc_resp *tpcIoctlResp =
                    (mlan_ioctl_11h_tpc_resp *) (buf + S_DS_GEN);
                if (tpcIoctlResp->status_code == 0) {
                    printf
                        ("tpcrequest:  txPower(%d), linkMargin(%d), rssi(%d)\n",
                         tpcIoctlResp->tx_power, tpcIoctlResp->link_margin,
                         tpcIoctlResp->rssi);
                } else {
                    printf("tpcrequest:  failure, status = %d\n",
                           tpcIoctlResp->status_code);
                }
                break;
            }
        case HostCmd_CMD_802_11_CRYPTO:
            {
                t_u16 alg =
                    le16_to_cpu((t_u16) * (buf + S_DS_GEN + sizeof(t_u16)));
                if (alg != CIPHER_TEST_AES_CCM) {
                    HostCmd_DS_802_11_CRYPTO *cmd =
                        (HostCmd_DS_802_11_CRYPTO *) (buf + S_DS_GEN);
                    cmd->encdec = le16_to_cpu(cmd->encdec);
                    cmd->algorithm = le16_to_cpu(cmd->algorithm);
                    cmd->key_IV_length = le16_to_cpu(cmd->key_IV_length);
                    cmd->key_length = le16_to_cpu(cmd->key_length);
                    cmd->data.header.type = le16_to_cpu(cmd->data.header.type);
                    cmd->data.header.len = le16_to_cpu(cmd->data.header.len);

                    printf("crypto_result: encdec=%d algorithm=%d,KeyIVLen=%d,"
                           " KeyLen=%d,dataLen=%d\n",
                           cmd->encdec, cmd->algorithm, cmd->key_IV_length,
                           cmd->key_length, cmd->data.header.len);
                    hexdump("KeyIV", cmd->keyIV, cmd->key_IV_length, ' ');
                    hexdump("Key", cmd->key, cmd->key_length, ' ');
                    hexdump("Data", cmd->data.data, cmd->data.header.len, ' ');
                } else {
                    HostCmd_DS_802_11_CRYPTO_AES_CCM *cmd_aes_ccm =
                        (HostCmd_DS_802_11_CRYPTO_AES_CCM *) (buf + S_DS_GEN);

                    cmd_aes_ccm->encdec = le16_to_cpu(cmd_aes_ccm->encdec);
                    cmd_aes_ccm->algorithm
                        = le16_to_cpu(cmd_aes_ccm->algorithm);
                    cmd_aes_ccm->key_length
                        = le16_to_cpu(cmd_aes_ccm->key_length);
                    cmd_aes_ccm->nonce_length
                        = le16_to_cpu(cmd_aes_ccm->nonce_length);
                    cmd_aes_ccm->AAD_length
                        = le16_to_cpu(cmd_aes_ccm->AAD_length);
                    cmd_aes_ccm->data.header.type
                        = le16_to_cpu(cmd_aes_ccm->data.header.type);
                    cmd_aes_ccm->data.header.len
                        = le16_to_cpu(cmd_aes_ccm->data.header.len);

                    printf("crypto_result: encdec=%d algorithm=%d, KeyLen=%d,"
                           " NonceLen=%d,AADLen=%d,dataLen=%d\n",
                           cmd_aes_ccm->encdec,
                           cmd_aes_ccm->algorithm,
                           cmd_aes_ccm->key_length,
                           cmd_aes_ccm->nonce_length,
                           cmd_aes_ccm->AAD_length,
                           cmd_aes_ccm->data.header.len);

                    hexdump("Key", cmd_aes_ccm->key, cmd_aes_ccm->key_length,
                            ' ');
                    hexdump("Nonce", cmd_aes_ccm->nonce,
                            cmd_aes_ccm->nonce_length, ' ');
                    hexdump("AAD", cmd_aes_ccm->AAD, cmd_aes_ccm->AAD_length,
                            ' ');
                    hexdump("Data", cmd_aes_ccm->data.data,
                            cmd_aes_ccm->data.header.len, ' ');
                }
                break;
            }
        case HostCmd_CMD_802_11_AUTO_TX:
            {
                HostCmd_DS_802_11_AUTO_TX *at =
                    (HostCmd_DS_802_11_AUTO_TX *) (buf + S_DS_GEN);

                if (le16_to_cpu(at->action) == HostCmd_ACT_GEN_GET) {
                    if (S_DS_GEN + sizeof(at->action) == hostcmd->size) {
                        printf("auto_tx not configured\n");

                    } else {
                        MrvlIEtypesHeader_t *header = &at->auto_tx.header;

                        header->type = le16_to_cpu(header->type);
                        header->len = le16_to_cpu(header->len);

                        if ((S_DS_GEN + sizeof(at->action)
                             + sizeof(MrvlIEtypesHeader_t)
                             + header->len == hostcmd->size) &&
                            (header->type == TLV_TYPE_AUTO_TX)) {

                            AutoTx_MacFrame_t *atmf
                                = &at->auto_tx.auto_tx_mac_frame;

                            printf("Interval: %d second(s)\n",
                                   le16_to_cpu(atmf->interval));
                            printf("Priority: %#x\n", atmf->priority);
                            printf("Frame Length: %d\n",
                                   le16_to_cpu(atmf->frame_len));
                            printf("Dest Mac Address: "
                                   "%02x:%02x:%02x:%02x:%02x:%02x\n",
                                   atmf->dest_mac_addr[0],
                                   atmf->dest_mac_addr[1],
                                   atmf->dest_mac_addr[2],
                                   atmf->dest_mac_addr[3],
                                   atmf->dest_mac_addr[4],
                                   atmf->dest_mac_addr[5]);
                            printf("Src Mac Address: "
                                   "%02x:%02x:%02x:%02x:%02x:%02x\n",
                                   atmf->src_mac_addr[0],
                                   atmf->src_mac_addr[1],
                                   atmf->src_mac_addr[2],
                                   atmf->src_mac_addr[3],
                                   atmf->src_mac_addr[4],
                                   atmf->src_mac_addr[5]);

                            hexdump("Frame Payload", atmf->payload,
                                    le16_to_cpu(atmf->frame_len)
                                    - MLAN_MAC_ADDR_LENGTH * 2, ' ');
                        } else {
                            printf("incorrect auto_tx command response\n");
                        }
                    }
                }
                break;
            }
        case HostCmd_CMD_802_11_SUBSCRIBE_EVENT:
            {
                HostCmd_DS_802_11_SUBSCRIBE_EVENT *se =
                    (HostCmd_DS_802_11_SUBSCRIBE_EVENT *) (buf + S_DS_GEN);
                if (le16_to_cpu(se->action) == HostCmd_ACT_GEN_GET) {
                    int len =
                        S_DS_GEN + sizeof(HostCmd_DS_802_11_SUBSCRIBE_EVENT);
                    printf("\nEvent\t\tValue\tFreq\tsubscribed\n\n");
                    while (len < hostcmd->size) {
                        MrvlIEtypesHeader_t *header =
                            (MrvlIEtypesHeader_t *) (buf + len);
                        switch (le16_to_cpu(header->type)) {
                        case TLV_TYPE_RSSI_LOW:
                            {
                                MrvlIEtypes_RssiThreshold_t *low_rssi =
                                    (MrvlIEtypes_RssiThreshold_t *) (buf + len);
                                printf("Beacon Low RSSI\t%d\t%d\t%s\n",
                                       low_rssi->RSSI_value,
                                       low_rssi->RSSI_freq,
                                       (le16_to_cpu(se->events) & 0x0001) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_SNR_LOW:
                            {
                                MrvlIEtypes_SnrThreshold_t *low_snr =
                                    (MrvlIEtypes_SnrThreshold_t *) (buf + len);
                                printf("Beacon Low SNR\t%d\t%d\t%s\n",
                                       low_snr->SNR_value,
                                       low_snr->SNR_freq,
                                       (le16_to_cpu(se->events) & 0x0002) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_FAILCOUNT:
                            {
                                MrvlIEtypes_FailureCount_t *failure_count =
                                    (MrvlIEtypes_FailureCount_t *) (buf + len);
                                printf("Failure Count\t%d\t%d\t%s\n",
                                       failure_count->fail_value,
                                       failure_count->fail_freq,
                                       (le16_to_cpu(se->events) & 0x0004) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_BCNMISS:
                            {
                                MrvlIEtypes_BeaconsMissed_t *bcn_missed =
                                    (MrvlIEtypes_BeaconsMissed_t *) (buf + len);
                                printf("Beacon Missed\t%d\tN/A\t%s\n",
                                       bcn_missed->beacon_missed,
                                       (le16_to_cpu(se->events) & 0x0008) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_RSSI_HIGH:
                            {
                                MrvlIEtypes_RssiThreshold_t *high_rssi =
                                    (MrvlIEtypes_RssiThreshold_t *) (buf + len);
                                printf("Bcn High RSSI\t%d\t%d\t%s\n",
                                       high_rssi->RSSI_value,
                                       high_rssi->RSSI_freq,
                                       (le16_to_cpu(se->events) & 0x0010) ?
                                       "yes" : "no");
                                break;
                            }

                        case TLV_TYPE_SNR_HIGH:
                            {
                                MrvlIEtypes_SnrThreshold_t *high_snr =
                                    (MrvlIEtypes_SnrThreshold_t *) (buf + len);
                                printf("Beacon High SNR\t%d\t%d\t%s\n",
                                       high_snr->SNR_value,
                                       high_snr->SNR_freq,
                                       (le16_to_cpu(se->events) & 0x0020) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_RSSI_LOW_DATA:
                            {
                                MrvlIEtypes_RssiThreshold_t *low_rssi =
                                    (MrvlIEtypes_RssiThreshold_t *) (buf + len);
                                printf("Data Low RSSI\t%d\t%d\t%s\n",
                                       low_rssi->RSSI_value,
                                       low_rssi->RSSI_freq,
                                       (le16_to_cpu(se->events) & 0x0040) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_SNR_LOW_DATA:
                            {
                                MrvlIEtypes_SnrThreshold_t *low_snr =
                                    (MrvlIEtypes_SnrThreshold_t *) (buf + len);
                                printf("Data Low SNR\t%d\t%d\t%s\n",
                                       low_snr->SNR_value,
                                       low_snr->SNR_freq,
                                       (le16_to_cpu(se->events) & 0x0080) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_RSSI_HIGH_DATA:
                            {
                                MrvlIEtypes_RssiThreshold_t *high_rssi =
                                    (MrvlIEtypes_RssiThreshold_t *) (buf + len);
                                printf("Data High RSSI\t%d\t%d\t%s\n",
                                       high_rssi->RSSI_value,
                                       high_rssi->RSSI_freq,
                                       (le16_to_cpu(se->events) & 0x0100) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_SNR_HIGH_DATA:
                            {
                                MrvlIEtypes_SnrThreshold_t *high_snr =
                                    (MrvlIEtypes_SnrThreshold_t *) (buf + len);
                                printf("Data High SNR\t%d\t%d\t%s\n",
                                       high_snr->SNR_value,
                                       high_snr->SNR_freq,
                                       (le16_to_cpu(se->events) & 0x0200) ?
                                       "yes" : "no");
                                break;
                            }
                        case TLV_TYPE_LINK_QUALITY:
                            {
                                MrvlIEtypes_LinkQuality_t *link_qual =
                                    (MrvlIEtypes_LinkQuality_t *) (buf + len);
                                printf("Link Quality Parameters:\n");
                                printf("------------------------\n");
                                printf("Link Quality Event Subscribed\t%s\n",
                                       (le16_to_cpu(se->events) & 0x0400) ?
                                       "yes" : "no");
                                printf("Link SNR Threshold   = %d\n",
                                       le16_to_cpu(link_qual->link_SNR_thrs));
                                printf("Link SNR Frequency   = %d\n",
                                       le16_to_cpu(link_qual->link_SNR_freq));
                                printf("Min Rate Value       = %d\n",
                                       le16_to_cpu(link_qual->min_rate_val));
                                printf("Min Rate Frequency   = %d\n",
                                       le16_to_cpu(link_qual->min_rate_freq));
                                printf("Tx Latency Value     = %d\n",
                                       le32_to_cpu(link_qual->tx_latency_val));
                                printf("Tx Latency Threshold = %d\n",
                                       le32_to_cpu(link_qual->tx_latency_thrs));

                                break;
                            }
                        case TLV_TYPE_PRE_BEACON_LOST:
                            {
                                MrvlIEtypes_PreBeaconLost_t *pre_bcn_lost =
                                    (MrvlIEtypes_PreBeaconLost_t *) (buf + len);
                                printf("------------------------\n");
                                printf("Pre-Beacon Lost Event Subscribed\t%s\n",
                                       (le16_to_cpu(se->events) & 0x0800) ?
                                       "yes" : "no");
                                printf("Pre-Beacon Lost: %d\n",
                                       pre_bcn_lost->pre_beacon_lost);
                                break;
                            }
                        default:
                            printf("Unknown subscribed event TLV Type=%#x,"
                                   " Len=%d\n",
                                   le16_to_cpu(header->type),
                                   le16_to_cpu(header->len));
                            break;
                        }

                        len += (sizeof(MrvlIEtypesHeader_t)
                                + le16_to_cpu(header->len));
                    }
                }
                break;
            }
        case HostCmd_CMD_MAC_REG_ACCESS:
        case HostCmd_CMD_BBP_REG_ACCESS:
        case HostCmd_CMD_RF_REG_ACCESS:
        case HostCmd_CMD_CAU_REG_ACCESS:
            {
                HostCmd_DS_REG *preg = (HostCmd_DS_REG *) (buf + S_DS_GEN);
                preg->action = le16_to_cpu(preg->action);
                if (preg->action == HostCmd_ACT_GEN_GET) {
                    preg->value = le32_to_cpu(preg->value);
                    printf("value = 0x%08x\n", preg->value);
                }
                break;
            }
        case HostCmd_CMD_MEM_ACCESS:
            {
                HostCmd_DS_MEM *pmem = (HostCmd_DS_MEM *) (buf + S_DS_GEN);
                pmem->action = le16_to_cpu(pmem->action);
                if (pmem->action == HostCmd_ACT_GEN_GET) {
                    pmem->value = le32_to_cpu(pmem->value);
                    printf("value = 0x%08x\n", pmem->value);
                }
                break;
            }
        default:
            printf("HOSTCMD_RESP: CmdCode=%#04x, Size=%#04x,"
                   " SeqNum=%#04x, Result=%#04x\n",
                   hostcmd->command, hostcmd->size,
                   hostcmd->seq_num, hostcmd->result);
            hexdump("payload",
                    (t_void *) (buf + S_DS_GEN), hostcmd->size - S_DS_GEN, ' ');
            break;
        }
    } else {
        printf("HOSTCMD failed: CmdCode=%#04x, Size=%#04x,"
               " SeqNum=%#04x, Result=%#04x\n",
               hostcmd->command, hostcmd->size,
               hostcmd->seq_num, hostcmd->result);
    }
    return ret;
}

/**
 *  @brief Process hostcmd command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_hostcmd(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    FILE *fp = NULL;
    t_s8 cmdname[256];

    if (argc < 5) {
        printf("Error: invalid no of arguments\n");
        printf("Syntax: ./mlanutl mlanX hostcmd <hostcmd.conf> <cmdname>\n");
        return MLAN_STATUS_FAILURE;
    }

    if ((fp = fopen(argv[3], "r")) == NULL) {
        fprintf(stderr, "Cannot open file %s\n", argv[3]);
        return MLAN_STATUS_FAILURE;
    }

    snprintf(cmdname, sizeof(cmdname), "%s", argv[4]);

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    /* Prepare the hostcmd buffer */
    prepare_buffer(buffer, argv[2], 0, NULL);
    if (MLAN_STATUS_FAILURE ==
        prepare_host_cmd_buffer(fp, cmdname,
                                buffer + strlen(CMD_MARVELL) +
                                strlen(argv[2]))) {
        fclose(fp);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }
    fclose(fp);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: hostcmd fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    process_host_cmd_resp(argv[2], cmd->buf);

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process HT Tx configuration
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_httxcfg(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    t_u32 *data = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: httxcfg fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    if (argc == 3) {
        /* Get result */
        data = (t_u32 *) cmd->buf;
        printf("HT Tx cfg: \n");
        printf("    BG band:  0x%08x\n", data[0]);
        printf("     A band:  0x%08x\n", data[1]);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process HT capability configuration
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_htcapinfo(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct eth_priv_htcapinfo *ht_cap = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: htcapinfo fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    if (argc == 3) {
        ht_cap = (struct eth_priv_htcapinfo *) cmd->buf;
        printf("HT cap info: \n");
        printf("    BG band:  0x%08x\n", ht_cap->ht_cap_info_bg);
        printf("     A band:  0x%08x\n", ht_cap->ht_cap_info_a);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process HT Add BA parameters
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_addbapara(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    struct eth_priv_addba *addba = NULL;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: addbapara fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    if (argc == 3) {
        /* Get */
        addba = (struct eth_priv_addba *) cmd->buf;
        printf("Add BA configuration: \n");
        printf("    Time out : %d\n", addba->time_out);
        printf("    TX window: %d\n", addba->tx_win_size);
        printf("    RX window: %d\n", addba->rx_win_size);
        printf("    TX AMSDU : %d\n", addba->tx_amsdu);
        printf("    RX AMSDU : %d\n", addba->rx_amsdu);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process Aggregation priority table parameters
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_aggrpriotbl(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    int i;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: aggrpriotbl fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    if (argc == 3) {
        /* Get */
        printf("Aggregation priority table cfg: \n");
        printf("    TID      AMPDU      AMSDU \n");
        for (i = 0; i < MAX_NUM_TID; i++) {
            printf("     %d        %3d        %3d \n",
                   i, cmd->buf[2 * i], cmd->buf[2 * i + 1]);
        }
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process HT Add BA reject configurations
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_addbareject(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    int i;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: addbareject fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    if (argc == 3) {
        /* Get */
        printf("Add BA reject configuration: \n");
        printf("    TID      Reject \n");
        for (i = 0; i < MAX_NUM_TID; i++) {
            printf("     %d        %d\n", i, cmd->buf[i]);
        }
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process HT Del BA command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_delba(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: delba fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

static t_s8 *rate_format[3] = { "LG", "HT", "VHT" };

static t_s8 *lg_rate[] = { "1 Mbps", "2 Mbps", "5.5 Mbps", "11 Mbps",
    "6 Mbps", "9 Mbps", "12 Mbps", "18 Mbps",
    "24 Mbps", "36 Mbps", "48 Mbps", "54 Mbps"
};

/**
 *  @brief Process Get data rate
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_datarate(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct eth_priv_data_rate *datarate = NULL;
    struct ifreq ifr;
    t_s8 *bw[] = { "20 MHz", "40 MHz", "80 MHz", "160 MHz" };

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], 0, NULL);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: getdatarate fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    datarate = (struct eth_priv_data_rate *) (cmd->buf);
    printf("Data Rate:\n");
    printf("  TX: \n");
    if (datarate->tx_data_rate < 12) {
        printf("    Type: %s\n", rate_format[0]);
        /* LG */
        printf("    Rate: %s\n", lg_rate[datarate->tx_data_rate]);
    } else {
        /* HT */
        printf("    Type: %s\n", rate_format[1]);
        if (datarate->tx_bw <= 2)
            printf("    BW:   %s\n", bw[datarate->tx_bw]);
        if (datarate->tx_gi == 0)
            printf("    GI:   Long\n");
        else
            printf("    GI:   Short\n");
        printf("    MCS:  MCS %d\n", (int) (datarate->tx_data_rate - 12));
    }

    printf("  RX: \n");
    if (datarate->rx_data_rate < 12) {
        printf("    Type: %s\n", rate_format[0]);
        /* LG */
        printf("    Rate: %s\n", lg_rate[datarate->rx_data_rate]);
    } else {
        /* HT */
        printf("    Type: %s\n", rate_format[1]);
        if (datarate->rx_bw <= 2)
            printf("    BW:   %s\n", bw[datarate->rx_bw]);
        if (datarate->rx_gi == 0)
            printf("    GI:   Long\n");
        else
            printf("    GI:   Short\n");
        printf("    MCS:  MCS %d\n", (int) (datarate->rx_data_rate - 12));
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process tx rate configuration
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_txratecfg(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct eth_priv_tx_rate_cfg *txratecfg = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: txratecfg fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    txratecfg = (struct eth_priv_tx_rate_cfg *) (cmd->buf);
    if (argc == 3) {
        /* GET operation */
        printf("Tx Rate Configuration: \n");
        /* format */
        if (txratecfg->rate_format == 0xFF) {
            printf("    Type:       0xFF (Auto)\n");
        } else if (txratecfg->rate_format <= 2) {
            printf("    Type:       %d (%s)\n", txratecfg->rate_format,
                   rate_format[txratecfg->rate_format]);
            if (txratecfg->rate_format == 0)
                printf("    Rate Index: %d (%s)\n", txratecfg->rate_index,
                       lg_rate[txratecfg->rate_index]);
            else if (txratecfg->rate_format >= 1)
                printf("    MCS Index:  %d\n", (int) txratecfg->rate_index);
        } else {
            printf("    Unknown rate format.\n");
        }
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process passphrase command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_passphrase(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    /* The argument being a string, this requires special handling */
    prepare_buffer(buffer, argv[2], 0, NULL);
    if (argc >= 4) {
        strcpy((char *) (buffer + strlen(CMD_MARVELL) + strlen(argv[2])),
               argv[3]);
    }

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: passphrase fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    printf("Passphrase Configuration: %s\n", (char *) (cmd->buf));

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process deauth command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_deauth(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    /* The argument being a string, this requires special handling */
    prepare_buffer(buffer, argv[2], 0, NULL);
    if (argc >= 4) {
        strcpy((char *) (buffer + strlen(CMD_MARVELL) + strlen(argv[2])),
               argv[3]);
    }

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: deauth fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

#ifdef UAP_SUPPORT
/**
 *  @brief Process getstalist command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_getstalist(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    struct eth_priv_getstalist *list = NULL;
    int i = 0, rssi = 0;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    /* The argument being a string, this requires special handling */
    prepare_buffer(buffer, argv[2], 0, NULL);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: getstalist fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }
    list =
        (struct eth_priv_getstalist *) (buffer + strlen(CMD_MARVELL) +
                                        strlen(argv[2]));

    printf("Number of STA = %d\n\n", list->sta_count);

    for (i = 0; i < list->sta_count; i++) {
        printf("STA %d information:\n", i + 1);
        printf("=====================\n");
        printf("MAC Address: ");
        print_mac(list->client_info[i].mac_address);
        printf("\nPower mfg status: %s\n",
               (list->client_info[i].power_mfg_status ==
                0) ? "active" : "power save");

        /** On some platform, s8 is same as unsigned char*/
        rssi = (int) list->client_info[i].rssi;
        if (rssi > 0x7f)
            rssi = -(256 - rssi);
        printf("Rssi : %d dBm\n\n", rssi);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}
#endif

#if defined(WIFI_DIRECT_SUPPORT)
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
/**
 *  @brief Process BSS role command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_bssrole(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: bssrole fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    if (argc == 3) {
        /* GET operation */
        printf("BSS role: %d\n", buffer[0]);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}
#endif
#endif

#ifdef STA_SUPPORT
/**
 *  @brief Helper function for process_getscantable_idx
 *
 *  @param pbuf     A pointer to the buffer
 *  @param buf_len  Buffer length
 *
 *  @return         NA
 *
 */
static void
dump_scan_elems(const t_u8 * pbuf, uint buf_len)
{
    uint idx;
    uint marker = 2 + pbuf[1];

    for (idx = 0; idx < buf_len; idx++) {
        if (idx % 0x10 == 0) {
            printf("\n%04x: ", idx);
        }

        if (idx == marker) {
            printf("|");
            marker = idx + pbuf[idx + 1] + 2;
        } else {
            printf(" ");
        }

        printf("%02x ", pbuf[idx]);
    }

    printf("\n");
}

/**
 *  @brief Helper function for process_getscantable_idx
 *  Find next element
 *
 *  @param pp_ie_out    Pointer of a IEEEtypes_Generic_t structure pointer
 *  @param p_buf_left   Integer pointer, which contains the number of left p_buf
 *
 *  @return             MLAN_STATUS_SUCCESS on success, otherwise MLAN_STATUS_FAILURE
 */
static int
scantable_elem_next(IEEEtypes_Generic_t ** pp_ie_out, int *p_buf_left)
{
    IEEEtypes_Generic_t *pie_gen;
    t_u8 *p_next;

    if (*p_buf_left < 2) {
        return MLAN_STATUS_FAILURE;
    }

    pie_gen = *pp_ie_out;

    p_next = (t_u8 *) pie_gen + (pie_gen->ieee_hdr.len
                                 + sizeof(pie_gen->ieee_hdr));
    *p_buf_left -= (p_next - (t_u8 *) pie_gen);

    *pp_ie_out = (IEEEtypes_Generic_t *) p_next;

    if (*p_buf_left <= 0) {
        return MLAN_STATUS_FAILURE;
    }

    return MLAN_STATUS_SUCCESS;
}

 /**
  *  @brief Helper function for process_getscantable_idx
  *         scantable find element
  *
  *  @param ie_buf       Pointer of the IE buffer
  *  @param ie_buf_len   IE buffer length
  *  @param ie_type      IE type
  *  @param ppie_out     Pointer to the IEEEtypes_Generic_t structure pointer
  *  @return             MLAN_STATUS_SUCCESS on success, otherwise MLAN_STATUS_FAILURE
  */
static int
scantable_find_elem(t_u8 * ie_buf,
                    unsigned int ie_buf_len,
                    IEEEtypes_ElementId_e ie_type,
                    IEEEtypes_Generic_t ** ppie_out)
{
    int found;
    unsigned int ie_buf_left;

    ie_buf_left = ie_buf_len;

    found = FALSE;

    *ppie_out = (IEEEtypes_Generic_t *) ie_buf;

    do {
        found = ((*ppie_out)->ieee_hdr.element_id == ie_type);

    } while (!found &&
             (scantable_elem_next(ppie_out, (int *) &ie_buf_left) == 0));

    if (!found) {
        *ppie_out = NULL;
    }

    return (found ? MLAN_STATUS_SUCCESS : MLAN_STATUS_FAILURE);
}

 /**
  *  @brief Helper function for process_getscantable_idx
  *         It gets SSID from IE
  *
  *  @param ie_buf       IE buffer
  *  @param ie_buf_len   IE buffer length
  *  @param pssid        SSID
  *  @param ssid_buf_max Size of SSID
  *  @return             MLAN_STATUS_SUCCESS on success, otherwise MLAN_STATUS_FAILURE
  */
static int
scantable_get_ssid_from_ie(t_u8 * ie_buf,
                           unsigned int ie_buf_len,
                           t_u8 * pssid, unsigned int ssid_buf_max)
{
    int retval;
    IEEEtypes_Generic_t *pie_gen;

    retval = scantable_find_elem(ie_buf, ie_buf_len, SSID, &pie_gen);

    memcpy(pssid, pie_gen->data, MIN(pie_gen->ieee_hdr.len, ssid_buf_max));

    return retval;
}

/**
 *  @brief Display detailed information for a specific scan table entry
 *
 *  @param prsp_info_req    Scan table entry request structure
 *  @return         MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
process_getscantable_idx(char *cmd_name,
                         wlan_ioctl_get_scan_table_info * prsp_info_req)
{
    int ret = 0;
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    t_u8 *pcurrent;
    char ssid[33];
    t_u16 tmp_cap;
    t_u8 tsf[8];
    t_u16 beacon_interval;
    t_u16 cap_info;
    wlan_ioctl_get_scan_table_info *prsp_info;

    wlan_get_scan_table_fixed fixed_fields;
    t_u32 fixed_field_length;
    t_u32 bss_info_length;

    memset(ssid, 0x00, sizeof(ssid));

    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, cmd_name, 0, NULL);

    prsp_info =
        (wlan_ioctl_get_scan_table_info *) (buffer + strlen(CMD_MARVELL) +
                                            strlen(cmd_name));

    memcpy(prsp_info, prsp_info_req, sizeof(wlan_ioctl_get_scan_table_info));

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /*
     * Set up and execute the ioctl call
     */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        if (errno == EAGAIN) {
            ret = -EAGAIN;
        } else {
            perror("mlanutl");
            fprintf(stderr, "mlanutl: getscantable fail\n");
            ret = MLAN_STATUS_FAILURE;
        }
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return ret;
    }

    prsp_info = (wlan_ioctl_get_scan_table_info *) buffer;
    if (prsp_info->scan_number == 0) {
        printf("mlanutl: getscantable ioctl - index out of range\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return -EINVAL;
    }

    pcurrent = prsp_info->scan_table_entry_buf;

    memcpy((t_u8 *) & fixed_field_length,
           (t_u8 *) pcurrent, sizeof(fixed_field_length));
    pcurrent += sizeof(fixed_field_length);

    memcpy((t_u8 *) & bss_info_length,
           (t_u8 *) pcurrent, sizeof(bss_info_length));
    pcurrent += sizeof(bss_info_length);

    memcpy((t_u8 *) & fixed_fields, (t_u8 *) pcurrent, sizeof(fixed_fields));
    pcurrent += fixed_field_length;

    /* Time stamp is 8 byte long */
    memcpy(tsf, pcurrent, sizeof(tsf));
    pcurrent += sizeof(tsf);
    bss_info_length -= sizeof(tsf);

    /* Beacon interval is 2 byte long */
    memcpy(&beacon_interval, pcurrent, sizeof(beacon_interval));
    /* Endian convert needed here */
    beacon_interval = le16_to_cpu(beacon_interval);
    pcurrent += sizeof(beacon_interval);
    bss_info_length -= sizeof(beacon_interval);

    /* Capability information is 2 byte long */
    memcpy(&cap_info, pcurrent, sizeof(cap_info));
    /* Endian convert needed here */
    cap_info = le16_to_cpu(cap_info);
    pcurrent += sizeof(cap_info);
    bss_info_length -= sizeof(cap_info);

    scantable_get_ssid_from_ie(pcurrent,
                               bss_info_length, (t_u8 *) ssid, sizeof(ssid));

    printf("\n*** [%s], %02x:%02x:%02x:%02x:%02x:%2x\n",
           ssid,
           fixed_fields.bssid[0],
           fixed_fields.bssid[1],
           fixed_fields.bssid[2],
           fixed_fields.bssid[3], fixed_fields.bssid[4], fixed_fields.bssid[5]);
    memcpy(&tmp_cap, &cap_info, sizeof(tmp_cap));
    printf("Channel = %d, SS = %d, CapInfo = 0x%04x, BcnIntvl = %d\n",
           fixed_fields.channel,
           255 - fixed_fields.rssi, tmp_cap, beacon_interval);

    printf("TSF Values: AP(0x%02x%02x%02x%02x%02x%02x%02x%02x), ",
           tsf[7], tsf[6], tsf[5], tsf[4], tsf[3], tsf[2], tsf[1], tsf[0]);

    printf("Network(0x%016llx)\n", fixed_fields.network_tsf);
    printf("\n");
    printf("Element Data (%d bytes)\n", (int) bss_info_length);
    printf("------------");
    dump_scan_elems(pcurrent, bss_info_length);
    printf("\n");

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process getscantable command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_getscantable(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    unsigned int scan_start;
    int idx, ret = 0;

    t_u8 *pcurrent;
    t_u8 *pnext;
    IEEEtypes_ElementId_e *pelement_id;
    t_u8 *pelement_len;
    int ssid_idx;
    t_u8 *pbyte;
    char ssid[33];
    int ssid_len = 0;

    IEEEtypes_CapInfo_t cap_info;
    t_u16 tmp_cap;
    t_u8 tsf[8];
    t_u16 beacon_interval;

    IEEEtypes_VendorSpecific_t *pwpa_ie;
    const t_u8 wpa_oui[4] = { 0x00, 0x50, 0xf2, 0x01 };

    IEEEtypes_WmmParameter_t *pwmm_ie;
    const t_u8 wmm_oui[4] = { 0x00, 0x50, 0xf2, 0x02 };
    IEEEtypes_VendorSpecific_t *pwps_ie;
    const t_u8 wps_oui[4] = { 0x00, 0x50, 0xf2, 0x04 };
    char wmm_cap;
    char wps_cap;
    char dot11k_cap;
    char dot11r_cap;
    char priv_cap;
    char ht_cap;

    int displayed_info;

    wlan_ioctl_get_scan_table_info rsp_info_req;
    wlan_ioctl_get_scan_table_info *prsp_info;

    wlan_get_scan_table_fixed fixed_fields;
    t_u32 fixed_field_length;
    t_u32 bss_info_length;

    memset(&cap_info, 0x00, sizeof(cap_info));

    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    if (argc > 3 && (strcmp(argv[3], "tsf") != 0)
        && (strcmp(argv[3], "help") != 0)) {

        idx = strtol(argv[3], NULL, 10);

        if (idx >= 0) {
            rsp_info_req.scan_number = idx;
            ret = process_getscantable_idx(argv[2], &rsp_info_req);
            if (buffer)
                free(buffer);
            if (cmd)
                free(cmd);
            return ret;
        }
    }

    displayed_info = FALSE;
    scan_start = 1;

    do {
        prepare_buffer(buffer, argv[2], 0, NULL);
        prsp_info =
            (wlan_ioctl_get_scan_table_info *) (buffer + strlen(CMD_MARVELL) +
                                                strlen(argv[2]));

        prsp_info->scan_number = scan_start;

        /*
         * Set up and execute the ioctl call
         */
        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
        ifr.ifr_ifru.ifru_data = (void *) cmd;

        if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
            if (errno == EAGAIN) {
                ret = -EAGAIN;
            } else {
                perror("mlanutl");
                fprintf(stderr, "mlanutl: getscantable fail\n");
                ret = MLAN_STATUS_FAILURE;
            }
            if (cmd)
                free(cmd);
            if (buffer)
                free(buffer);
            return ret;
        }

        prsp_info = (wlan_ioctl_get_scan_table_info *) buffer;
        pcurrent = 0;
        pnext = prsp_info->scan_table_entry_buf;

        if (scan_start == 1) {
            printf("---------------------------------------");
            printf("---------------------------------------\n");
            printf("# | ch  | ss  |       bssid       |   cap    |   SSID \n");
            printf("---------------------------------------");
            printf("---------------------------------------\n");
        }

        for (idx = 0; (unsigned int) idx < prsp_info->scan_number; idx++) {

            /*
             * Set pcurrent to pnext in case pad bytes are at the end
             *   of the last IE we processed.
             */
            pcurrent = pnext;

            memcpy((t_u8 *) & fixed_field_length,
                   (t_u8 *) pcurrent, sizeof(fixed_field_length));
            pcurrent += sizeof(fixed_field_length);

            memcpy((t_u8 *) & bss_info_length,
                   (t_u8 *) pcurrent, sizeof(bss_info_length));
            pcurrent += sizeof(bss_info_length);

            memcpy((t_u8 *) & fixed_fields,
                   (t_u8 *) pcurrent, sizeof(fixed_fields));
            pcurrent += fixed_field_length;

            /* Set next to be the start of the next scan entry */
            pnext = pcurrent + bss_info_length;

            printf("%02u| %03d | %03d | %02x:%02x:%02x:%02x:%02x:%02x |",
                   scan_start + idx,
                   fixed_fields.channel,
                   255 - fixed_fields.rssi,
                   fixed_fields.bssid[0],
                   fixed_fields.bssid[1],
                   fixed_fields.bssid[2],
                   fixed_fields.bssid[3],
                   fixed_fields.bssid[4], fixed_fields.bssid[5]);

            displayed_info = TRUE;

            if (bss_info_length >=
                (sizeof(tsf) + sizeof(beacon_interval) + sizeof(cap_info))) {
                /* Time stamp is 8 byte long */
                memcpy(tsf, pcurrent, sizeof(tsf));
                pcurrent += sizeof(tsf);
                bss_info_length -= sizeof(tsf);

                /* Beacon interval is 2 byte long */
                memcpy(&beacon_interval, pcurrent, sizeof(beacon_interval));
                /* Endian convert needed here */
                beacon_interval = le16_to_cpu(beacon_interval);
                pcurrent += sizeof(beacon_interval);
                bss_info_length -= sizeof(beacon_interval);

                /* Capability information is 2 byte long */
                memcpy(&tmp_cap, pcurrent, sizeof(tmp_cap));
                /* Endian convert needed here */
                tmp_cap = le16_to_cpu(tmp_cap);
                memcpy(&cap_info, &tmp_cap, sizeof(cap_info));
                pcurrent += sizeof(cap_info);
                bss_info_length -= sizeof(cap_info);
            }

            wmm_cap = ' ';      /* M (WMM), C (WMM-Call Admission Control) */
            wps_cap = ' ';      /* "S" */
            dot11k_cap = ' ';   /* "K" */
            dot11r_cap = ' ';   /* "R" */
            ht_cap = ' ';       /* "N" */

            /* "P" for Privacy (WEP) since "W" is WPA, and "2" is RSN/WPA2 */
            priv_cap = cap_info.privacy ? 'P' : ' ';

            memset(ssid, 0, MRVDRV_MAX_SSID_LENGTH + 1);
            ssid_len = 0;
            while (bss_info_length >= 2) {
                pelement_id = (IEEEtypes_ElementId_e *) pcurrent;
                pelement_len = pcurrent + 1;
                pcurrent += 2;

                switch (*pelement_id) {

                case SSID:
                    if (*pelement_len &&
                        *pelement_len <= MRVDRV_MAX_SSID_LENGTH) {
                        memcpy(ssid, pcurrent, *pelement_len);
                        ssid_len = *pelement_len;
                    }
                    break;

                case WPA_IE:
                    pwpa_ie = (IEEEtypes_VendorSpecific_t *) pelement_id;
                    if ((memcmp
                         (pwpa_ie->vend_hdr.oui, wpa_oui,
                          sizeof(pwpa_ie->vend_hdr.oui)) == 0)
                        && (pwpa_ie->vend_hdr.oui_type == wpa_oui[3])) {
                        /* WPA IE found, 'W' for WPA */
                        priv_cap = 'W';
                    } else {
                        pwmm_ie = (IEEEtypes_WmmParameter_t *) pelement_id;
                        if ((memcmp(pwmm_ie->vend_hdr.oui,
                                    wmm_oui,
                                    sizeof(pwmm_ie->vend_hdr.oui)) == 0)
                            && (pwmm_ie->vend_hdr.oui_type == wmm_oui[3])) {
                            /* Check the subtype: 1 == parameter, 0 == info */
                            if ((pwmm_ie->vend_hdr.oui_subtype == 1)
                                && pwmm_ie->ac_params[WMM_AC_VO].aci_aifsn.acm) {
                                /* Call admission on VO; 'C' for CAC */
                                wmm_cap = 'C';
                            } else {
                                /* No CAC; 'M' for uh, WMM */
                                wmm_cap = 'M';
                            }
                        } else {
                            pwps_ie =
                                (IEEEtypes_VendorSpecific_t *) pelement_id;
                            if ((memcmp
                                 (pwps_ie->vend_hdr.oui, wps_oui,
                                  sizeof(pwps_ie->vend_hdr.oui)) == 0)
                                && (pwps_ie->vend_hdr.oui_type == wps_oui[3])) {
                                wps_cap = 'S';
                            }
                        }
                    }
                    break;

                case RSN_IE:
                    /* RSN IE found; '2' for WPA2 (RSN) */
                    priv_cap = '2';
                    break;
                case HT_CAPABILITY:
                    ht_cap = 'N';
                    break;
                default:
                    break;
                }

                pcurrent += *pelement_len;
                bss_info_length -= (2 + *pelement_len);
            }
            /* "A" for Adhoc "I" for Infrastructure, "D" for DFS (Spectrum
               Mgmt) */
            printf(" %c%c%c%c%c%c%c%c | ", cap_info.ibss ? 'A' : 'I', priv_cap, /* P
                                                                                   (WEP),
                                                                                   W
                                                                                   (WPA),
                                                                                   2
                                                                                   (WPA2)
                                                                                 */
                   cap_info.spectrum_mgmt ? 'D' : ' ', wmm_cap, /* M (WMM), C
                                                                   (WMM-Call
                                                                   Admission
                                                                   Control) */
                   dot11k_cap,  /* K */
                   dot11r_cap,  /* R */
                   wps_cap,     /* S */
                   ht_cap);     /* N (11n) or A (11ac) */

            /* Print out the ssid or the hex values if non-printable */
            for (ssid_idx = 0; ssid_idx < ssid_len; ssid_idx++) {
                if (isprint(ssid[ssid_idx])) {
                    printf("%c", ssid[ssid_idx]);
                } else {
                    printf("\\%02x", ssid[ssid_idx]);
                }
            }

            printf("\n");

            if (argc > 3 && strcmp(argv[3], "tsf") == 0) {
                /* TSF is a u64, some formatted printing libs have trouble
                   printing long longs, so cast and dump as bytes */
                pbyte = (t_u8 *) & fixed_fields.network_tsf;
                printf("    TSF=%02x%02x%02x%02x%02x%02x%02x%02x\n",
                       pbyte[7], pbyte[6], pbyte[5], pbyte[4],
                       pbyte[3], pbyte[2], pbyte[1], pbyte[0]);
            }
        }

        scan_start += prsp_info->scan_number;

    } while (prsp_info->scan_number);

    if (displayed_info == TRUE) {
        if (argc > 3 && strcmp(argv[3], "help") == 0) {
            printf("\n\n"
                   "Capability Legend (Not all may be supported)\n"
                   "-----------------\n"
                   " I [ Infrastructure ]\n"
                   " A [ Ad-hoc ]\n"
                   " W [ WPA IE ]\n"
                   " 2 [ WPA2/RSN IE ]\n"
                   " M [ WMM IE ]\n"
                   " C [ Call Admission Control - WMM IE, VO ACM set ]\n"
                   " D [ Spectrum Management - DFS (11h) ]\n"
                   " K [ 11k ]\n"
                   " R [ 11r ]\n" " S [ WPS ]\n" " N [ HT (11n) ]\n" "\n\n");
        }
    } else {
        printf("< No Scan Results >\n");
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Prepare setuserscan command buffer
 *  @param buffer   Command buffer to be filled
 *  @param cmd      Command id
 *  @param num      Number of arguments
 *  @param args     Arguments list
 *  @return         MLAN_STATUS_SUCCESS
 */
static int
prepare_setuserscan_buffer(t_u8 * buffer, t_s8 * cmd, t_u32 num, char *args[])
{
    wlan_ioctl_user_scan_cfg *scan_req = NULL;
    t_u8 *pos = NULL;
    int arg_idx = 0;
    int num_ssid = 0;
    char *parg_tok = NULL;
    char *pchan_tok = NULL;
    char *parg_cookie = NULL;
    char *pchan_cookie = NULL;
    int chan_parse_idx = 0;
    int chan_cmd_idx = 0;
    char chan_scratch[MAX_CHAN_SCRATCH];
    char *pscratch = NULL;
    int tmp_idx = 0;
    int scan_time = 0;
    int is_radio_set = 0;
    unsigned int mac[ETH_ALEN];

    memset(buffer, 0, BUFFER_LENGTH);

    /* Flag it for our use */
    pos = buffer;
    strncpy((char *) pos, CMD_MARVELL, strlen(CMD_MARVELL));
    pos += (strlen(CMD_MARVELL));

    /* Insert command */
    strncpy((char *) pos, (char *) cmd, strlen(cmd));
    pos += (strlen(cmd));

    /* Insert arguments */
    scan_req = (wlan_ioctl_user_scan_cfg *) pos;

    for (arg_idx = 0; arg_idx < num; arg_idx++) {
        if (strncmp(args[arg_idx], "ssid=", strlen("ssid=")) == 0) {
            /* "ssid" token string handler */
            if (num_ssid < MRVDRV_MAX_SSID_LIST_LENGTH) {
                strncpy(scan_req->ssid_list[num_ssid].ssid,
                        args[arg_idx] + strlen("ssid="),
                        sizeof(scan_req->ssid_list[num_ssid].ssid));

                scan_req->ssid_list[num_ssid].max_len = 0;

                num_ssid++;
            }
        } else if (strncmp(args[arg_idx], "bssid=", strlen("bssid=")) == 0) {
            /* "bssid" token string handler */
            sscanf(args[arg_idx] + strlen("bssid="), "%2x:%2x:%2x:%2x:%2x:%2x",
                   mac + 0, mac + 1, mac + 2, mac + 3, mac + 4, mac + 5);

            for (tmp_idx = 0;
                 (unsigned int) tmp_idx < NELEMENTS(mac); tmp_idx++) {
                scan_req->specific_bssid[tmp_idx] = (t_u8) mac[tmp_idx];
            }
        } else if (strncmp(args[arg_idx], "chan=", strlen("chan=")) == 0) {
            /* "chan" token string handler */
            parg_tok = args[arg_idx] + strlen("chan=");

            if (strlen(parg_tok) > MAX_CHAN_SCRATCH) {
                printf("Error: Specified channels exceeds max limit\n");
                return MLAN_STATUS_FAILURE;
            }
            is_radio_set = FALSE;

            while ((parg_tok = strtok_r(parg_tok, ",", &parg_cookie)) != NULL) {

                memset(chan_scratch, 0x00, sizeof(chan_scratch));
                pscratch = chan_scratch;

                for (chan_parse_idx = 0;
                     (unsigned int) chan_parse_idx < strlen(parg_tok);
                     chan_parse_idx++) {
                    if (isalpha(*(parg_tok + chan_parse_idx))) {
                        *pscratch++ = ' ';
                    }

                    *pscratch++ = *(parg_tok + chan_parse_idx);
                }
                *pscratch = 0;
                parg_tok = NULL;

                pchan_tok = chan_scratch;

                while ((pchan_tok = strtok_r(pchan_tok, " ",
                                             &pchan_cookie)) != NULL) {
                    if (isdigit(*pchan_tok)) {
                        scan_req->chan_list[chan_cmd_idx].chan_number
                            = atoi(pchan_tok);
                        if (scan_req->chan_list[chan_cmd_idx].chan_number >
                            MAX_CHAN_BG_BAND)
                            scan_req->chan_list[chan_cmd_idx].radio_type = 1;
                    } else {
                        switch (toupper(*pchan_tok)) {
                        case 'A':
                            scan_req->chan_list[chan_cmd_idx].radio_type = 1;
                            is_radio_set = TRUE;
                            break;
                        case 'B':
                        case 'G':
                            scan_req->chan_list[chan_cmd_idx].radio_type = 0;
                            is_radio_set = TRUE;
                            break;
                        case 'N':
                            break;
                        case 'P':
                            scan_req->chan_list[chan_cmd_idx].scan_type =
                                MLAN_SCAN_TYPE_PASSIVE;
                            break;
                        default:
                            printf("Error: Band type not supported!\n");
                            return -EOPNOTSUPP;
                        }
                        if (!chan_cmd_idx &&
                            !scan_req->chan_list[chan_cmd_idx].chan_number &&
                            is_radio_set)
                            scan_req->chan_list[chan_cmd_idx].radio_type |=
                                BAND_SPECIFIED;
                    }
                    pchan_tok = NULL;
                }
                chan_cmd_idx++;
            }
        } else if (strncmp(args[arg_idx], "keep=", strlen("keep=")) == 0) {
            /* "keep" token string handler */
            scan_req->keep_previous_scan =
                atoi(args[arg_idx] + strlen("keep="));
        } else if (strncmp(args[arg_idx], "dur=", strlen("dur=")) == 0) {
            /* "dur" token string handler */
            scan_time = atoi(args[arg_idx] + strlen("dur="));
            scan_req->chan_list[0].scan_time = scan_time;

        } else if (strncmp(args[arg_idx], "wc=", strlen("wc=")) == 0) {

            if (num_ssid < MRVDRV_MAX_SSID_LIST_LENGTH) {
                /* "wc" token string handler */
                pscratch = strrchr(args[arg_idx], ',');

                if (pscratch) {
                    *pscratch = 0;
                    pscratch++;

                    if (isdigit(*pscratch)) {
                        scan_req->ssid_list[num_ssid].max_len = atoi(pscratch);
                    } else {
                        scan_req->ssid_list[num_ssid].max_len = *pscratch;
                    }
                } else {
                    /* Standard wildcard matching */
                    scan_req->ssid_list[num_ssid].max_len = 0xFF;
                }

                strncpy(scan_req->ssid_list[num_ssid].ssid,
                        args[arg_idx] + strlen("wc="),
                        sizeof(scan_req->ssid_list[num_ssid].ssid));

                num_ssid++;
            }
        } else if (strncmp(args[arg_idx], "probes=", strlen("probes=")) == 0) {
            /* "probes" token string handler */
            scan_req->num_probes = atoi(args[arg_idx] + strlen("probes="));
            if (scan_req->num_probes > MAX_PROBES) {
                fprintf(stderr, "Invalid probes (> %d)\n", MAX_PROBES);
                return -EOPNOTSUPP;
            }
        } else if (strncmp(args[arg_idx], "type=", strlen("type=")) == 0) {
            /* "type" token string handler */
            scan_req->bss_mode = atoi(args[arg_idx] + strlen("type="));
            switch (scan_req->bss_mode) {
            case MLAN_SCAN_MODE_BSS:
            case MLAN_SCAN_MODE_IBSS:
                break;
            case MLAN_SCAN_MODE_ANY:
            default:
                /* Set any unknown types to ANY */
                scan_req->bss_mode = MLAN_SCAN_MODE_ANY;
                break;
            }
        }
    }

    /* Update all the channels to have the same scan time */
    for (tmp_idx = 1; tmp_idx < chan_cmd_idx; tmp_idx++) {
        scan_req->chan_list[tmp_idx].scan_time = scan_time;
    }

    pos += sizeof(wlan_ioctl_user_scan_cfg);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process setuserscan command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_setuserscan(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    int status = 0;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_setuserscan_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: setuserscan fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    do {
        argv[2] = "getscantable";
        status = process_getscantable(0, argv);
    } while (status == -EAGAIN);

    return MLAN_STATUS_SUCCESS;
}
#endif

/**
 *  @brief Process deep sleep configuration
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_deepsleep(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: deepsleep fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    printf("Deepsleep command response: %s\n", cmd->buf);

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process ipaddr command
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_ipaddr(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    /* The argument being a string, this requires special handling */
    prepare_buffer(buffer, argv[2], 0, NULL);
    if (argc >= 4) {
        strcpy((char *) (buffer + strlen(CMD_MARVELL) + strlen(argv[2])),
               argv[3]);
    }

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: ipaddr fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    printf("IP address Configuration: %s\n", (char *) (cmd->buf));

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process countrycode setting
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_countrycode(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct eth_priv_countrycode *countrycode = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    /* The argument being a string, this requires special handling */
    prepare_buffer(buffer, argv[2], 0, NULL);
    if (argc >= 4) {
        strcpy((char *) (buffer + strlen(CMD_MARVELL) + strlen(argv[2])),
               argv[3]);
    }

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: countrycode fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    countrycode = (struct eth_priv_countrycode *) (cmd->buf);
    if (argc == 3) {
        /* GET operation */
        printf("Country code: %s\n", countrycode->country_code);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

#ifdef STA_SUPPORT
/**
 *  @brief Process listen interval configuration
 *  @param argc   number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_listeninterval(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: listen interval fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    if (argc == 3)
        printf("Listen interval command response: %s\n", cmd->buf);

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}
#endif

#ifdef DEBUG_LEVEL1
/**
 *  @brief Process driver debug configuration
 *  @param argc   number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_drvdbg(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    t_u32 drvdbg;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: drvdbg config fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    if (argc == 3) {
        memcpy(&drvdbg, cmd->buf, sizeof(drvdbg));
        printf("drvdbg: 0x%08x\n", drvdbg);
#ifdef DEBUG_LEVEL2
        printf("MINFO  (%08x) %s\n", MINFO, (drvdbg & MINFO) ? "X" : "");
        printf("MWARN  (%08x) %s\n", MWARN, (drvdbg & MWARN) ? "X" : "");
        printf("MENTRY (%08x) %s\n", MENTRY, (drvdbg & MENTRY) ? "X" : "");
#endif
        printf("MIF_D  (%08x) %s\n", MIF_D, (drvdbg & MIF_D) ? "X" : "");
        printf("MFW_D  (%08x) %s\n", MFW_D, (drvdbg & MFW_D) ? "X" : "");
        printf("MEVT_D (%08x) %s\n", MEVT_D, (drvdbg & MEVT_D) ? "X" : "");
        printf("MCMD_D (%08x) %s\n", MCMD_D, (drvdbg & MCMD_D) ? "X" : "");
        printf("MDAT_D (%08x) %s\n", MDAT_D, (drvdbg & MDAT_D) ? "X" : "");
        printf("MIOCTL (%08x) %s\n", MIOCTL, (drvdbg & MIOCTL) ? "X" : "");
        printf("MINTR  (%08x) %s\n", MINTR, (drvdbg & MINTR) ? "X" : "");
        printf("MEVENT (%08x) %s\n", MEVENT, (drvdbg & MEVENT) ? "X" : "");
        printf("MCMND  (%08x) %s\n", MCMND, (drvdbg & MCMND) ? "X" : "");
        printf("MDATA  (%08x) %s\n", MDATA, (drvdbg & MDATA) ? "X" : "");
        printf("MERROR (%08x) %s\n", MERROR, (drvdbg & MERROR) ? "X" : "");
        printf("MFATAL (%08x) %s\n", MFATAL, (drvdbg & MFATAL) ? "X" : "");
        printf("MMSG   (%08x) %s\n", MMSG, (drvdbg & MMSG) ? "X" : "");

    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}
#endif

/**
 *  @brief Process hscfg configuration
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_hscfg(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    struct eth_priv_hs_cfg *hscfg;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: hscfg fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    hscfg = (struct eth_priv_hs_cfg *) (cmd->buf);
    if (argc == 3) {
        /* GET operation */
        printf("HS Configuration:\n");
        printf("  Conditions: %d\n", (int) hscfg->conditions);
        printf("  GPIO: %d\n", (int) hscfg->gpio);
        printf("  GAP: %d\n", (int) hscfg->gap);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process hssetpara configuration
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_hssetpara(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: hssetpara fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process wakeup reason
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_wakeupresaon(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: get wakeup reason fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    printf("Get wakeup reason response: %s\n", cmd->buf);

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process scancfg configuration
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_scancfg(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;
    struct eth_priv_scan_cfg *scancfg;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], (argc - 3), &argv[3]);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if (ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: scancfg fail\n");
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    scancfg = (struct eth_priv_scan_cfg *) (cmd->buf);
    if (argc == 3) {
        /* GET operation */
        printf("Scan Configuration:\n");
        printf("    Scan Type:              %d (%s)\n", scancfg->scan_type,
               (scancfg->scan_type == 1) ? "Active" : (scancfg->scan_type ==
                                                       2) ? "Passive" : "");
        printf("    Scan Mode:              %d (%s)\n", scancfg->scan_mode,
               (scancfg->scan_mode == 1) ? "BSS" : (scancfg->scan_mode ==
                                                    2) ? "IBSS" : (scancfg->
                                                                   scan_mode ==
                                                                   3) ? "Any" :
               "");
        printf("    Scan Probes:            %d (%s)\n", scancfg->scan_probe,
               "per channel");
        printf("    Specific Scan Time:     %d ms\n",
               scancfg->scan_time.specific_scan_time);
        printf("    Active Scan Time:       %d ms\n",
               scancfg->scan_time.active_scan_time);
        printf("    Passive Scan Time:      %d ms\n",
               scancfg->scan_time.passive_scan_time);
        printf("    Extended Scan Support:  %d (%s)\n", scancfg->ext_scan,
               (scancfg->ext_scan == 0) ? "No" : (scancfg->ext_scan ==
                                                  1) ? "Yes" : "");
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief Process generic commands
 *  @param argc   Number of arguments
 *  @param argv   A pointer to arguments array
 *  @return     MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
process_generic(int argc, char *argv[])
{
    t_u8 *buffer = NULL;
    struct eth_priv_cmd *cmd = NULL;
    struct ifreq ifr;

    /* Initialize buffer */
    buffer = (t_u8 *) malloc(BUFFER_LENGTH);
    if (!buffer) {
        printf("ERR:Cannot allocate buffer for command!\n");
        return MLAN_STATUS_FAILURE;
    }

    prepare_buffer(buffer, argv[2], 0, NULL);

    cmd = (struct eth_priv_cmd *) malloc(sizeof(struct eth_priv_cmd));
    if (!cmd) {
        printf("ERR:Cannot allocate buffer for command!\n");
        free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Fill up buffer */
    cmd->buf = buffer;
    cmd->used_len = 0;
    cmd->total_len = BUFFER_LENGTH;

    /* Perform IOCTL */
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_ifrn.ifrn_name, dev_name, strlen(dev_name));
    ifr.ifr_ifru.ifru_data = (void *) cmd;

    if ((ioctl(sockfd, MLAN_ETH_PRIV, &ifr)) < 0) {
        perror("mlanutl");
        fprintf(stderr, "mlanutl: %s fail\n", argv[2]);
        if (cmd)
            free(cmd);
        if (buffer)
            free(buffer);
        return MLAN_STATUS_FAILURE;
    }

    /* Process result */
    if (argc == 3) {
        /* GET operation */
        printf("%s command response received: %s\n", argv[2], buffer);
    }

    if (buffer)
        free(buffer);
    if (cmd)
        free(cmd);

    return MLAN_STATUS_SUCCESS;
}

/********************************************************
        Global Functions
********************************************************/

/**
 *  @brief Entry function for mlanutl
 *  @param argc     Number of arguments
 *  @param argv     A pointer to arguments array
 *  @return         MLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
main(int argc, char *argv[])
{
    int ret = MLAN_STATUS_SUCCESS;

    if ((argc == 2) && (strcmp(argv[1], "-v") == 0)) {
        fprintf(stdout, "Marvell mlanutl version %s\n", MLANUTL_VER);
        exit(0);
    }
    if (argc < 3) {
        fprintf(stderr, "Invalid number of parameters!\n");
        display_usage();
        exit(1);
    }

    strncpy(dev_name, argv[1], IFNAMSIZ - 1);

    /*
     * Create a socket
     */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "mlanutl: Cannot open socket.\n");
        exit(1);
    }

    ret = process_command(argc, argv);

    if (ret == MLAN_STATUS_FAILURE) {
        ret = process_generic(argc, argv);
    }

    if (ret) {
        fprintf(stderr, "Invalid command specified!\n");
        display_usage();
        ret = 1;
    }

    close(sockfd);
    return ret;
}
