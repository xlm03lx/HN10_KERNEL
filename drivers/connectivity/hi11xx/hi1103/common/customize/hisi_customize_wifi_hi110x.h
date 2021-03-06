

#ifndef __HISI_CUSTOMIZE_WIFI_HI110X_H__
#define __HISI_CUSTOMIZE_WIFI_HI110X_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

//#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "mac_vap.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define NVRAM_PARAMS_ARRAY      "nvram_params"

#define MAC_LEN                 6
#define NV_WLAN_NUM             193
#define NV_WLAN_VALID_SIZE      12
#define MAC2STR(a)              (a)[0], "**", "**", "**", (a)[4], (a)[5]
#define MACFMT                  "%02x:%s:%s:%s:%02x:%02x"
#define MAC2STR_ALL(a)          (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACFMT_ALL              "%02x:%02x:%02x:%02x:%02x:%02x"
#define CUS_TAG_INI                     0x11
#define CUS_TAG_DTS                     0x12
#define CUS_TAG_NV                      0x13
#define CUS_TAG_PRIV_INI                0x14
#define CUS_TAG_PRO_LINE_INI            0x15
#define CALI_TXPWR_PA_DC_REF_MIN        (1000)
#define CALI_TXPWR_PA_DC_REF_MAX        (0x8000)
#define CALI_TXPWR_PA_DC_FRE_MIN        (0)
#define CALI_TXPWR_PA_DC_FRE_MAX        (78)
#define CALI_BT_TXPWR_PA_DC_REF_MAX     (15000)
#define RF_LINE_TXRX_GAIN_DB_2G_MIN     (-100)
#define RF_LINE_TXRX_GAIN_DB_5G_MIN     (-48)
#define PSD_THRESHOLD_MIN               (-15)
#define PSD_THRESHOLD_MAX               (-10)
#define LNA_GAIN_DB_MIN                 (-40)
#define LNA_GAIN_DB_MAX                 (80)

#define MAC_NUM_2G_BAND                 3   /* 2g band数 */
#define MAC_NUM_5G_BAND                 7   /* 5g band数 */
#define MAC_2G_CHANNEL_NUM              13
#define NUM_OF_NV_PARAMS                (NUM_OF_NV_MAX_TXPOWER + 4)     /* NVRAM中存储的参数值的总个数，包括4个基准功率 */
#define TX_RATIO_MAX                    (2000)                          /* tx占空比的最大有效值 */
#define TX_PWR_COMP_VAL_MAX             (50)                            /* 发射功率补偿值的最大有效值 */
#define MORE_PWR_MAX                    (50)                            /* 根据温度额外补偿的发射功率的最大有效值 */
#define COUNTRY_CODE_LEN                (3)                             /* 国家码位数 */
#define MAX_COUNTRY_COUNT               (300)                           /* 支持定制的国家的最大个数 */
#define DELTA_CCA_ED_HIGH_TH_RANGE      15                              /* δ调整上限，最大向上或向下调整15dB */
#define CUS_NUM_5G_BW                   4   /* 定制化5g带宽数 */
#define CUS_NUM_FCC_2G_PRO              3   /* 定制化2g FCC 11B+OFDM_20M+OFDM_40M */
#define CUS_NUM_5G_20M_SIDE_BAND        6   /* 定制化5g边带数 */
#define CUS_NUM_5G_40M_SIDE_BAND        4
#define CUS_NUM_5G_80M_SIDE_BAND        4
#define CUS_NUM_5G_160M_SIDE_BAND       2
#define CUS_NUM_OF_SAR_PARAMS           16  /* 定制化降SAR参数 5G_C0/1_BAND1~7 2.4G_C0/1 */
#define CUS_NUM_OF_SAR_LVL              3   /* 定制化降SAR档位数 */
#define CUS_MIN_OF_SAR_VAL              (0x28) /* 定制化降SAR最小值 4dbm */
#define DY_CALI_PARAMS_LEN              (104)   /* 动态校准定制项最大长度 */
#define DY_2G_CALI_PARAMS_NUM           (4)    /* 动态校准参数个数,2.4g */
#define DY_CALI_PARAMS_NUM              (14)   /* 动态校准参数个数,2.4g 4个(ofdm 20/40 11b cw),5g 5*2(high & low)个band */
#define DY_CALI_PARAMS_BASE_NUM         (9)    /* 动态校准参数个数,2.4g 4个(ofdm 20/40 11b cw),5g 5(high)个band */
#define DY_CALI_PARAMS_TIMES            (3)    /* 动态校准参数二次项系数个数 */
#define DY_CALI_FIT_PRECISION_A1        (6)
#define DY_CALI_FIT_PRECISION_A0        (16)

/* 根据产线delt power更新系数 */
#define CUS_FLUSH_NV_RATIO_BY_DELT_POW(_s_pow_par2, _s_pow_par1, _s_pow_par0, _s_delt_power) \
do {(_s_pow_par0) = (oal_int16)((((oal_int32)(_s_delt_power)*(_s_delt_power)*(_s_pow_par2))+(((oal_int32)(_s_pow_par1)<<DY_CALI_FIT_PRECISION_A1)*(_s_delt_power)) \
                                                               +((oal_int32)(_s_pow_par0)<<DY_CALI_FIT_PRECISION_A0))>>DY_CALI_FIT_PRECISION_A0);                    \
    (_s_pow_par1) = (oal_int16)((((oal_int32)(_s_delt_power)*(_s_pow_par2)*2)+((oal_int32)(_s_pow_par1)<<DY_CALI_FIT_PRECISION_A1))>>DY_CALI_FIT_PRECISION_A1);      \
    }while(0)


/* 计算绝对值 */
#define CUS_ABS(val)                    ((val) > 0 ? (val) : -(val))

/* 判断CCA能量门限调整值是否超出范围, 最大调整幅度:DELTA_CCA_ED_HIGH_TH_RANGE */
#define CUS_DELTA_CCA_ED_HIGH_TH_OUT_OF_RANGE(val)  (CUS_ABS(val) > DELTA_CCA_ED_HIGH_TH_RANGE ? 1 : 0)

/* 取16位数 */
#define CUS_GET_LOW_16BIT(val)    ((oal_uint16)((oal_uint32)val & 0x0000FFFF))
#define CUS_GET_HIGH_16BIT(val)   ((oal_uint16)(((oal_uint32)val & 0xFFFF0000) >> 16))

#define HWIFI_GET_5G_PRO_LINE_DELT_POW_PER_BAND(_ps_nv_params, _ps_ini_params) \
    (oal_int16)(((oal_int32)(((_ps_nv_params)[1]) - ((_ps_ini_params)[1]))<<DY_CALI_FIT_PRECISION_A1)/(2*((_ps_nv_params)[0])))

#define HWIFI_GET_2P4_MAX_TXPWR_BASE_MASTER(_plat_tag) hwifi_get_max_txpwr_base(_plat_tag, NVRAM_PARAMS_INDEX_19)
#define HWIFI_GET_5G_MAX_TXPWR_BASE_MASTER(_plat_tag)  hwifi_get_max_txpwr_base(_plat_tag, NVRAM_PARAMS_INDEX_20)
#define HWIFI_GET_2P4_MAX_TXPWR_BASE_SLAVE(_plat_tag)  hwifi_get_max_txpwr_base(_plat_tag, NVRAM_PARAMS_INDEX_21)
#define HWIFI_GET_5G_MAX_TXPWR_BASE_SLAVE(_plat_tag)   hwifi_get_max_txpwr_base(_plat_tag, NVRAM_PARAMS_INDEX_22)

/* 开机校准MASK */
#define HI1103_CALI_FIST_POWER_ON_MASK      BIT(2)
/* 读取NVRAM MASK */
#define HI1103_CUST_READ_NVRAM_MASK         BIT(4)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/* NV map idx */
typedef enum
{
    HWIFI_CFG_NV_WINVRAM_NUMBER           = 340,
    HWIFI_CFG_NV_WITXNVCCK_NUMBER         = 367,
    HWIFI_CFG_NV_WITXNVC1_NUMBER          = 368,
    HWIFI_CFG_NV_WITXNVBWC0_NUMBER        = 369,
    HWIFI_CFG_NV_WITXNVBWC1_NUMBER        = 370,
    HWIFI_CFG_NV_WITXL2G5G0_NUMBER        = 384,
    HWIFI_CFG_NV_WITXL2G5G1_NUMBER        = 385,
}wlan_nvram_idx;

typedef enum
{
   REGDOMAIN_FCC        = 0,
   REGDOMAIN_ETSI       = 1,
   REGDOMAIN_JAPAN      = 2,
   REGDOMAIN_COMMON     = 3,

   REGDOMAIN_COUNT
} regdomain_enum;

/* 定制化 DTS CONFIG ID */
typedef enum
{
    /* 校准 */
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN1,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_START = WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN1,     /* 校准 2g TXPWR_REF起始配置ID */
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN2,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN3,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN4,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN5,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN6,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN7,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN8,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN9,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN10,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN11,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN12,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN13,

    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND1,     //13
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_START = WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND1,    /* 校准 5g TXPWR_REF起始配置ID */
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND2,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND3,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND4,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND5,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND6,
    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND7,
    WLAN_CFG_DTS_CALI_TONE_AMP_GRADE,
    /* 动态校准 */
    WLAN_CFG_DTS_DYN_CALI_DSCR_ITERVL,
    /* DPN 40M 20M 11b */
    WLAN_CFG_DTS_2G_CORE0_DPN_CH1,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH2,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH3,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH4,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH5,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH6,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH7,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH8,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH9,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH10,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH11,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH12,
    WLAN_CFG_DTS_2G_CORE0_DPN_CH13,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH1,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH2,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH3,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH4,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH5,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH6,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH7,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH8,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH9,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH10,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH11,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH12,
    WLAN_CFG_DTS_2G_CORE1_DPN_CH13,
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* rf register */
    WLAN_CFG_DTS_RF_REG117,                     //33
    WLAN_CFG_DTS_RF_FIRST = WLAN_CFG_DTS_RF_REG117,
    WLAN_CFG_DTS_RF_REG123,
    WLAN_CFG_DTS_RF_REG124,
    WLAN_CFG_DTS_RF_REG125,
    WLAN_CFG_DTS_RF_REG126,
    WLAN_CFG_DTS_RF_LAST = WLAN_CFG_DTS_RF_REG126,
    /* bt tmp */
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND1,    //37
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND2,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND3,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND4,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND5,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND6,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND7,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND8,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_NUM,          //45
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE1,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE2,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE3,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE4,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE5,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE6,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE7,
    WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE8,         //53
    WLAN_CFG_DTS_BT_CALI_TONE_AMP_GRADE,
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)

    WLAN_CFG_DTS_BUTT,
}WLAN_CFG_DTS;


/* 私有定制化 PRIV CONFIG ID */
typedef enum
{
    /* 校准开关 */
    WLAN_CFG_PRIV_CALI_MASK,
    WLAN_CFG_PRIV_CALI_DATA_MASK,
    WLAN_CFG_PRIV_CALI_AUTOCALI_MASK,

    /* TBD:hal_cfg_customize_info_stru/mac_device_capability_stru */
    WLAN_CFG_PRIV_BW_MAX_WITH,
    WLAN_CFG_PRIV_LDPC_CODING,
    WLAN_CFG_PRIV_RX_STBC,
    WLAN_CFG_PRIV_TX_STBC,
    WLAN_CFG_PRIV_SU_BFER,
    WLAN_CFG_PRIV_SU_BFEE,
    WLAN_CFG_PRIV_MU_BFER,
    WLAN_CFG_PRIV_MU_BFEE,
    WLAN_CFG_PRIV_11N_TXBF,

    /* DBDC */
    WLAN_CFG_PRIV_DBDC_RADIO_0,
    WLAN_CFG_PRIV_DBDC_RADIO_1,
    WLAN_CFG_PRIV_FASTSCAN_SWITCH,

    /* 天线切换功能 */
    WLAN_CFG_ANT_SWITCH,

    /* BTCOEX */
    WLAN_CFG_PRIV_BTCOEX_MASK,

    /* 限流 */
#ifdef _PRE_WLAN_DOWNLOAD_PM
    WLAN_CFG_PRIV_DOWNLOAD_RATE_LIMIT_PPS,
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    WLAN_CFG_PRIV_TXOPPS_SWITCH,
#endif
    WLAN_CFG_PRIV_OVER_TEMPER_PROTECT_THRESHOLD,
    WLAN_CFG_PRIV_OVER_TEMP_PRO_ENABLE,
    WLAN_CFG_PRIV_OVER_TEMP_PRO_REDUCE_PWR_ENABLE,
    WLAN_CFG_PRIV_OVER_TEMP_PRO_SAFE_TH,

    WLAN_DSSS2OFDM_DBB_PWR_BO_VAL,
    WLAN_CFG_PRIV_EVM_PLL_REG_FIX,

    /*VOE*/
    WLAN_CFG_PRIV_VOE_SWITCH,
    /*11ax*/
    WLAN_CFG_PRIV_11AX_SWITCH,

    WLAN_CFG_PRIV_BUTT,
}WLAN_CFG_PRIV;
typedef oal_uint8 wlan_cfg_priv_id_uint8;

/* 定制化 INI CONFIG ID */
typedef enum
{
    /* ROAM */
    WLAN_CFG_INIT_ROAM_SWITCH = 0,
    WLAN_CFG_INIT_SCAN_ORTHOGONAL,
    WLAN_CFG_INIT_TRIGGER_B,
    WLAN_CFG_INIT_TRIGGER_A,
    WLAN_CFG_INIT_DELTA_B,
    WLAN_CFG_INIT_DELTA_A,

    /* 性能 */
    WLAN_CFG_INIT_AMPDU_TX_MAX_NUM,       //7
    WLAN_CFG_INIT_USED_MEM_FOR_START,
    WLAN_CFG_INIT_USED_MEM_FOR_STOP,
    WLAN_CFG_INIT_RX_ACK_LIMIT,
    WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT,
    WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT,
    /* LINKLOSS */
    WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_BT,    //13,这里不能直接赋值
    WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_DBAC,
    WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_NORMAL,
    /* 自动调频 */
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0,            //16
    WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_0,
    WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_0,
    WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_1,
    WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_1,
    WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_1,
    WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_2,
    WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_2,
    WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_2,
    WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_3,
    WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_3,
    WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_3,
    WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0,
    WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_1,
    WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_2,
    WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_3,
#endif
    WLAN_CFG_INIT_IRQ_AFFINITY,
    WLAN_CFG_INIT_IRQ_TH_LOW,
    WLAN_CFG_INIT_IRQ_TH_HIGH,
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    WLAN_CFG_INIT_HW_AMPDU,
    WLAN_CFG_INIT_HW_AMPDU_TH_LOW,
    WLAN_CFG_INIT_HW_AMPDU_TH_HIGH,
#endif
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    WLAN_CFG_INIT_AMPDU_AMSDU_SKB,
    WLAN_CFG_INIT_AMSDU_AMPDU_TH_LOW,
    WLAN_CFG_INIT_AMSDU_AMPDU_TH_HIGH,
#endif
    WLAN_CFG_INIT_RX_DDC,
    WLAN_CFG_INIT_RX_DDC_HIGH,
    WLAN_CFG_INIT_RX_DDC_LOW,
    /* 接收ampdu+amsdu */
    WLAN_CFG_INIT_RX_AMPDU_AMSDU_SKB,

    /* 低功耗 */
    WLAN_CFG_INIT_POWERMGMT_SWITCH,                 //31
    WLAN_CFG_INIT_PS_MODE,
    WLAN_CFG_INIT_FAST_CHECK_CNT,               //How many 20ms

    /* 可维可测 */
    WLAN_CFG_INIT_LOGLEVEL,
    /* 2G RF前端 */
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND_START,    //33
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND1 = WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND_START,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND2,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND3,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND_END = WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND3,
    /* 5G RF前端 */
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND_START,     //37
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND1 = WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND_START,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND2,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND3,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND4,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND5,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND6,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND7,
    WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND_END = WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND7,

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    /* 用于定制化计算PWR RF值的偏差 */
    WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C0_MULT4,
    WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C1_MULT4,
    WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C0_MULT4,
    WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C1_MULT4,
#endif

    /* FEM 03考虑2g */
    WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_2G,
    WLAN_CFG_INIT_RF_LNA_GAIN_DB_2G,
    WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_2G,
    WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_2G,
    WLAN_CFG_INIT_RF_PA_GAIN_LVL_2G,
    WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G,
    WLAN_CFG_INIT_EXT_PA_ISEXIST_2G,
    WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G,
    WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G,
    WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G,
    WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_5G,
    WLAN_CFG_INIT_RF_LNA_GAIN_DB_5G,
    WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_5G,
    WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_5G,
    WLAN_CFG_INIT_RF_PA_GAIN_LVL_5G,
    WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G,
    WLAN_CFG_INIT_EXT_PA_ISEXIST_5G,
    WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G,
    WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G,
    WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G,
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* 温度上升导致发射功率下降过多的功率补偿 */
    WLAN_CFG_INIT_TX_RATIO_LEVEL_0,                 /* tx占空比 */                      //48
    WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_0,          /* 发射功率补偿值 */
    WLAN_CFG_INIT_TX_RATIO_LEVEL_1,
    WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_1,
    WLAN_CFG_INIT_TX_RATIO_LEVEL_2,
    WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_2,
    WLAN_CFG_INIT_MORE_PWR,                         /* 根据温度额外补偿的发射功率 */
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* SCAN */
    WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN,
    /* 11AC2G */
    WLAN_CFG_INIT_11AC2G_ENABLE,                    /* 11ac2g开关 */                    //56
    WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40,             /* 2ght40禁止开关 */
    WLAN_CFG_INIT_DUAL_ANTENNA_ENABLE,              /* 双天线开关 */
    /* sta keepalive*/
    WLAN_CFG_INIT_STA_KEEPALIVE_CNT_TH,               /* sta keepalive th*/
    WLAN_CFG_INIT_FAR_DIST_POW_GAIN_SWITCH,
    WLAN_CFG_INIT_FAR_DIST_DSSS_SCALE_PROMOTE_SWITCH, /* 超远距11b 1m 2m dbb scale提升使能开关 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    WLAN_CFG_INIT_CHANN_RADIO_CAP,                    /* 通道0、1 2g 5g频道能力 */

    WLAN_CFG_LTE_GPIO_CHECK_SWITCH,                 /* lte?????? */
    WLAN_ATCMDSRV_ISM_PRIORITY,
    WLAN_ATCMDSRV_LTE_RX,
    WLAN_ATCMDSRV_LTE_TX,
    WLAN_ATCMDSRV_LTE_INACT,
    WLAN_ATCMDSRV_ISM_RX_ACT,
    WLAN_ATCMDSRV_BANT_PRI,
    WLAN_ATCMDSRV_BANT_STATUS,
    WLAN_ATCMDSRV_WANT_PRI,
    WLAN_ATCMDSRV_WANT_STATUS,
    WLAN_TX5G_UPC_MIX_GAIN_CTRL_1,
    WLAN_TX5G_UPC_MIX_GAIN_CTRL_2,
    WLAN_TX5G_UPC_MIX_GAIN_CTRL_3,
    WLAN_TX5G_UPC_MIX_GAIN_CTRL_4,
    WLAN_TX5G_UPC_MIX_GAIN_CTRL_5,
    WLAN_TX5G_UPC_MIX_GAIN_CTRL_6,
    WLAN_TX5G_UPC_MIX_GAIN_CTRL_7,

    WLAN_TX2G_PA_GATE_VCTL_REG236,
    WLAN_TX2G_PA_GATE_VCTL_REG237,
    WLAN_TX2G_PA_GATE_VCTL_REG238,
    WLAN_TX2G_PA_GATE_VCTL_REG239,
    WLAN_TX2G_PA_GATE_VCTL_REG240,
    WLAN_TX2G_PA_GATE_VCTL_REG241,
    WLAN_TX2G_PA_GATE_VCTL_REG242,
    WLAN_TX2G_PA_GATE_VCTL_REG243,
    WLAN_TX2G_PA_GATE_VCTL_REG244,

    WLAN_TX2G_PA_VRECT_GATE_THIN_REG253,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG254,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG255,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG256,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG257,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG258,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG259,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG260,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG261,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG262,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG263,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG264,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG265,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG266,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG267,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG268,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG269,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG270,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG271,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG272,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG273,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG274,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG275,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG276,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG277,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG278,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG279,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND1,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG281,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG282,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG283,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG284,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND2,
    WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND3,
#endif
    WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_2G,
    WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_2G,
    WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_5G,
    WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_5G,
    WLAN_CFG_INIT_VOE_SWITCH,
    WLAN_CFG_INIT_11AX_SWITCH,

    WLAN_CFG_INIT_BUTT,
}WLAN_CFG_INIT;

/* 定制化 NVRAM PARAMS INDEX */
typedef enum
{
    NVRAM_PARAMS_INDEX_0,
    NVRAM_PARAMS_INDEX_1,
    NVRAM_PARAMS_INDEX_2,
    NVRAM_PARAMS_INDEX_3,
    NVRAM_PARAMS_INDEX_4,
    NVRAM_PARAMS_INDEX_5,
    NVRAM_PARAMS_INDEX_6,
    NVRAM_PARAMS_INDEX_7,
    NVRAM_PARAMS_INDEX_8,
    NVRAM_PARAMS_INDEX_9,
    NVRAM_PARAMS_INDEX_10,
    NVRAM_PARAMS_INDEX_11,
    /* 2.4g 5g 20M mcs9
      2.4g 5g 20M mcs10 11
      2.4g 5g 40M mcs10 11
      2.4g 5g 80M mcs10 11
      5g 160M mcs0~3
      5g 160M mcs10~11 */
    NVRAM_PARAMS_INDEX_12,
    NVRAM_PARAMS_INDEX_13,
    NVRAM_PARAMS_INDEX_14,
    NVRAM_PARAMS_INDEX_15,
    NVRAM_PARAMS_INDEX_16,
    NVRAM_PARAMS_INDEX_17,
    NVRAM_PARAMS_TXPWR_INDEX_BUTT,
    NVRAM_PARAMS_INDEX_19, /* 2.4g基准发射功率 */
    NVRAM_PARAMS_INDEX_20, /* 5g基准发射功率 */
    NVRAM_PARAMS_INDEX_21, /* 双rf，slave 2.4g基准发射功率 */
    NVRAM_PARAMS_INDEX_22, /* 双rf，slave 5g基准发射功率 */
    NVRAM_PARAMS_BASE_INDEX_BUTT,
    NVRAM_PARAMS_INDEX_23,
    NVRAM_PARAMS_INDEX_24,
    /* FCC最大目标发射功率 */
    NVRAM_PARAMS_INDEX_25,
    NVRAM_PARAMS_INDEX_26,
    NVRAM_PARAMS_INDEX_27,
    NVRAM_PARAMS_INDEX_28,
    NVRAM_PARAMS_INDEX_29,
    NVRAM_PARAMS_INDEX_30,
    NVRAM_PARAMS_INDEX_31,
    NVRAM_PARAMS_INDEX_32,
    NVRAM_PARAMS_INDEX_33,
    NVRAM_PARAMS_INDEX_34,
    NVRAM_PARAMS_INDEX_35,
    NVRAM_PARAMS_INDEX_36,
    NVRAM_PARAMS_INDEX_37,
    NVRAM_PARAMS_INDEX_38,
    NVRAM_PARAMS_INDEX_39,
    NVRAM_PARAMS_INDEX_40,
    NVRAM_PARAMS_INDEX_41,
    NVRAM_PARAMS_INDEX_42,
    /* SAR CONTROL */
    NVRAM_PARAMS_INDEX_43,
    NVRAM_PARAMS_INDEX_44,
    NVRAM_PARAMS_INDEX_45,
    NVRAM_PARAMS_INDEX_46,
    NVRAM_PARAMS_INDEX_47,
    NVRAM_PARAMS_INDEX_48,
    NVRAM_PARAMS_INDEX_49,
    NVRAM_PARAMS_INDEX_50,
    NVRAM_PARAMS_INDEX_51,
    NVRAM_PARAMS_INDEX_52,
    NVRAM_PARAMS_INDEX_53,
    NVRAM_PARAMS_INDEX_54,
    NVRAM_PARAMS_INDEX_55,
    NVRAM_PARAMS_INDEX_56,
    NVRAM_PARAMS_INDEX_57,
    NVRAM_PARAMS_INDEX_58,
    NVRAM_PARAMS_PWR_INDEX_BUTT,
}NVRAM_PARAMS_INDEX;

/* 03产侧nvram参数 */
typedef enum
{
    /* DP init */
    WLAN_CFG_NVRAM_DP2G_INIT0 = 0,
    WLAN_CFG_NVRAM_DP2G_INIT1 = 1,
    /* pa */
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0 = 2,
    WLAN_CFG_NVRAM_RATIO_PA2GA0,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A0,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GA1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1,
    /* just for ini file */
    /* 5g Band1& 2g CW */
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GCWA0,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GCWA1,
    /* add 5g low power part*/
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1_LOW,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1_LOW,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW,

    WLAN_CFG_DTS_NVRAM_PARAMS_BUTT,
    /* 新增系数依次往后添加 */
}PRO_LINE_NVRAM_PARAMS_INDEX;


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef unsigned char countrycode_t[COUNTRY_CODE_LEN];
typedef struct
{
   regdomain_enum       en_regdomain;
   countrycode_t        auc_country_code;
} countryinfo_stru;

/**
 *  regdomain <-> plat_tag map structure
 *
 **/
typedef struct regdomain_plat_tag_map
{
    regdomain_enum  en_regdomain;
    int             plat_tag;
} regdomain_plat_tag_map_stru;

typedef struct
{
    char*   name;
    int     case_entry;
} wlan_cfg_cmd;

typedef struct
{
    oal_int32               l_val;
    oal_bool_enum_uint8     en_value_state;
}wlan_customize_private_stru;

typedef struct wlan_cus_pwr_fit_para_stru
{
    oal_int16 s_pow_par2;   /* 二次项系数 */
    oal_int16 s_pow_par1;   /* 一次 */
    oal_int16 s_pow_par0;   /* 常数项 */
}wlan_customize_pwr_fit_para_stru;

typedef struct
{
    oal_uint8*   puc_nv_name;
    oal_uint8*   puc_param_name;
    oal_uint32   ul_nv_map_idx;
    oal_uint8    uc_param_idx;
    oal_uint8    auc_rsv[3];
} wlan_cfg_nv_map_handler;

/* 定制化HOST全局变量结构体 */
typedef struct
{
    /* ba tx 聚合数 */
    unsigned int    ul_ampdu_tx_max_num;
    /* 漫游 */
    unsigned char   uc_roam_switch;
    unsigned char   uc_roam_scan_orthogonal;
    signed char     c_roam_trigger_b;
    signed char     c_roam_trigger_a;
    signed char     c_roam_delta_b;
    signed char     c_roam_delta_a;
    /* scan */
    unsigned char   uc_random_mac_addr_scan;
    /* capab */
    unsigned char   uc_disable_capab_2ght40;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    unsigned int   ul_lte_gpio_check_switch;
    unsigned int   ul_ism_priority;
    unsigned int   ul_lte_rx;
    unsigned int   ul_lte_tx;
    unsigned int   ul_lte_inact;
    unsigned int   ul_ism_rx_act;
    unsigned int   ul_bant_pri;
    unsigned int   ul_bant_status;
    unsigned int   ul_want_pri;
    unsigned int   ul_want_status;
#endif
} wlan_customize_stru;

/* 不接受cust下发的切换国家码的命令
 * 1、ini文件中国家码被配置成99
 * 2、维测需要:使用hipriv命令修改过国家码
*/
typedef struct
{
    oal_bool_enum_uint8     en_country_code_ingore_ini_flag;
    oal_bool_enum_uint8     en_country_code_ingore_hipriv_flag;
}wlan_cust_country_code_ingore_flag_stru;

typedef struct
{
    oal_int8  ac_delt_txpwr_params[NUM_OF_NV_MAX_TXPOWER];
    oal_uint8 auc_txpwr_base_params[WLAN_RF_CHANNEL_NUMS][WLAN_BAND_BUTT];
    /* FCC边带功率定制项 */
    oal_uint8 auc_5g_fcc_txpwr_limit_params_20M[CUS_NUM_5G_20M_SIDE_BAND];
    oal_uint8 auc_5g_fcc_txpwr_limit_params_40M[CUS_NUM_5G_40M_SIDE_BAND];
    oal_uint8 auc_5g_fcc_txpwr_limit_params_80M[CUS_NUM_5G_80M_SIDE_BAND];
    oal_uint8 auc_5g_fcc_txpwr_limit_params_160M[CUS_NUM_5G_160M_SIDE_BAND];
    oal_uint8 auc_2g_fcc_txpwr_limit_params[MAC_2G_CHANNEL_NUM][CUS_NUM_FCC_2G_PRO];
    /* SAR CTRL */
    oal_uint8 auc_sar_ctrl_params[CUS_NUM_OF_SAR_LVL][CUS_NUM_OF_SAR_PARAMS];
}wlan_cust_nvram_params;


extern wlan_customize_pwr_fit_para_stru g_as_pro_line_params[WLAN_RF_CHANNEL_NUMS][DY_CALI_PARAMS_NUM];
extern wlan_cust_country_code_ingore_flag_stru g_st_cust_country_code_ignore_flag;
extern wlan_customize_stru g_st_wlan_customize;
extern oal_bool_enum_uint8 g_en_nv_dp_init_is_null;      /* NVRAM中dp init置空标志 */

extern int hwifi_config_init(int);
extern int hwifi_get_init_value(int, int);
extern char *hwifi_get_country_code(void);
extern void hwifi_set_country_code(char*, const unsigned int);
extern int hwifi_get_mac_addr(unsigned char *);
extern void *hwifi_get_nvram_params(void);
extern int hwifi_is_regdomain_changed(const countrycode_t, const countrycode_t);
extern int hwifi_get_cfg_params(void);
extern int hwifi_hcc_customize_h2d_data_cfg(void);
extern int hwifi_custom_host_read_cfg_init(void);
extern uint32 hwifi_get_max_txpwr_base(int32 l_plat_tag, oal_uint8 uc_nvram_param_idx);
extern int32 hwifi_get_init_priv_value(oal_int32 l_cfg_id, oal_int32 *pl_priv_value);
extern regdomain_enum hwifi_get_regdomain_from_country_code(const countrycode_t country_code);
extern oal_uint8 *hwifi_get_nvram_param(oal_uint32 ul_nvram_param_idx);
//#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#endif //#if defined(_PRE_PRODUCT_ID_HI110X_HOST)


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif //hisi_customize_wifi.h

