/** @file  mlanscanagent.h
  *
  * @brief This files contains mlanconfig scanagent command handling.
  *
  *  Copyright (C) 2008-2009, Marvell International Ltd.
  *  All Rights Reserved
  */
/************************************************************************
Change log:
     08/11/2009: initial version
************************************************************************/

#ifndef _MLAN_SCANAGENT_H_
#define _MLAN_SCANAGENT_H_

typedef struct
{
    /** Action GET or SET */
    t_u16 action;
    /** scan interval */
    t_u16 scan_interval;
} __ATTRIB_PACK__ HostCmd_DS_SCANAGENT_CONFIG_PROFILE_SCAN;

#ifdef BIG_ENDIAN
typedef struct
{
    /** Reserved Not used for now */
    t_u16 reserved2:6;
    /** Channel width freq. 10 MHZ enabled */
    t_u16 chanwidth_10_MHZ_enabled:1;
    /** Channel width freq. 20 MHZ enabled */
    t_u16 chanwidth_20_MHZ_enabled:1;
    /** Reserved Not used for now */
    t_u16 reserved1:5;
    /** Band freq. 4 GHZ enabled */
    t_u16 band_4_GHZ_enabled:1;
    /** Band freq. 5 GHZ enabled */
    t_u16 band_5_GHZ_enabled:1;
    /** Band freq. 2.4 GHZ enabled */
    t_u16 band_2_4_GHZ_enabled:1;
} __ATTRIB_PACK__ CONFIG_CHAN_FILTER;
#else
typedef struct
{
    /** Band freq. 2.4 GHZ enabled */
    t_u16 band_2_4_GHZ_enabled:1;
    /** Band freq. 5 GHZ enabled */
    t_u16 band_5_GHZ_enabled:1;
    /** Band freq. 4 GHZ enabled */
    t_u16 band_4_GHZ_enabled:1;
    /** Reserved Not used for now */
    t_u16 reserved1:5;
    /** Channel width freq. 20 MHZ enabled */
    t_u16 chanwidth_20_MHZ_enabled:1;
    /** Channel width freq. 10 MHZ enabled */
    t_u16 chanwidth_10_MHZ_enabled:1;
    /** Reserved Not used for now */
    t_u16 reserved2:6;
} __ATTRIB_PACK__ CONFIG_CHAN_FILTER;
#endif

typedef struct
{
    t_u32 scan_req_id_out;              /**< Scan request id */
    t_u32 delay;                        /**< Delay */
    t_u16 table;                        /**< Reg channel table */
    t_u16 scan_type;                    /**< Scan type */
    CONFIG_CHAN_FILTER filter;          /**< Configuration channel filter */
    t_u16 reserved;                     /**< Reserved */

    t_u8 tlv_buffer[1];                 /**< Rest is TLV buffer */

    /* MrvlIEtypes_SsIdParamSet_t MrvlIEtypes_Bssid_List_t
       MrvlIEtypes_ConfigScanTiming_t */
} __ATTRIB_PACK__ HostCmd_DS_SCANAGENT_SCAN_EXEC;

typedef struct
{
    /** Action Set or get */
    t_u16 action;
    /** Reserved */
    t_u16 reserved;
    /** Table age limit */
    t_u16 table_age_limit;
    /** Table hold limit */
    t_u16 table_hold_limit;
} __ATTRIB_PACK__ HostCmd_DS_SCANAGENT_SCAN_TABLE_LIMITS;

typedef struct
{
    /** Action Set or get */
    t_u16 action;
    /** TLV buffer starts here */
    t_u8 tlv_buffer[1];
    /* MrvlIEtypes_ConfigScanTiming_t */
} __ATTRIB_PACK__ HostCmd_DS_SCANAGENT_CONFIG_TIMING;

typedef struct
{
    /** Action Set or get */
    t_u16 action;
    /** Reserved */
    t_u16 reserved;
    /** Request Id, 0 to disable */
    t_u32 scan_request_id;
    /** Age, 0 to disable */
    t_u32 age;
    /** TLV Buffer follows */
    t_u8 tlv_buffer[1];

    /* MrvlIEtypes_SsIdParamSet_t MrvlIEtypes_Bssid_List_t */
} __ATTRIB_PACK__ HostCmd_DS_SCANAGENT_TABLE_MAINTENANCE;

/** ENUM definition: Scanagent Table Maintenance */
typedef enum
{
    SCAN_TABLE_OP_INVALID = 0,

    SCAN_TABLE_OP_LOCK = 1,
    SCAN_TABLE_OP_UNLOCK = 2,
    SCAN_TABLE_OP_PURGE = 3,

} __ATTRIB_PACK__ HostCmd_DS_ScanagentTableMaintenance_e;

/** MrvlIEtypes_SsIdParamSet_t */
typedef struct _MrvlIEtypes_SsIdParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t header;
    /** SSID */
    t_u8 ssid[1];
} __ATTRIB_PACK__ MrvlIEtypes_SsIdParamSet_t;

/** _MrvlIEtypes_Bssid_List_t */
typedef struct _MrvlIEtypes_Bssid_List_t
{
    /** Header */
    MrvlIEtypesHeader_t header;
    /** BSSID */
    t_u8 bssid[ETH_ALEN];
} __ATTRIB_PACK__ MrvlIEtypes_Bssid_List_t;

typedef struct
{
    /** Header */
    MrvlIEtypesHeader_t header;

    t_u32 mode;    /**< Mode */
    t_u32 dwell;   /**< Dwell */
    t_u32 max_off;  /**< Max. off */
    t_u32 min_link; /**< Minimum Link */

} __ATTRIB_PACK__ MrvlIEtypes_ConfigScanTiming_t;

/** ENUM definition: ScanAgentScanType  */
typedef enum
{
    CONFIG_SITE_SURVEY = 0,
    CONFIG_NEIGHBOR = 1,
    CONFIG_PROFILE = 2,
    CONFIG_ARBITRARY_CHANNEL = 3,

} __ATTRIB_PACK__ HostCmd_DS_ScanagentScanType_e;

/** ENUM definition: ScanAgentScanTimingMode  */
typedef enum
{
    TIMING_MODE_INVALID = 0,

    TIMING_MODE_DISCONNECTED = 1,
    TIMING_MODE_ADHOC = 2,
    TIMING_MODE_FULL_POWER = 3,
    TIMING_MODE_IEEE_PS = 4,
    TIMING_MODE_PERIODIC_PS = 5,

} __ATTRIB_PACK__HostCmd_DS_ScanagentTimingMode_e;

#endif /* _MLAN_SCANAGENT_H_ */
