/*
 * Shared compile-time wireless capability predicates for userland.
 * Keep board/driver feature checks in one place so rc/httpd/defaults
 * stop carrying divergent copies of the same logic.
 */

#ifndef _WIRELESS_CAPS_H_
#define _WIRELESS_CAPS_H_

#if defined(USE_WID_2G)
#define WL_CAP_2G_WID USE_WID_2G
#else
#define WL_CAP_2G_WID 0
#endif

#if defined(USE_WID_5G)
#define WL_CAP_5G_WID USE_WID_5G
#else
#define WL_CAP_5G_WID 0
#endif

#if defined(USE_WID_2G) && (USE_WID_2G == 7615 || USE_WID_2G == 7915)
#define WL_CAP_2G_MODERN 1
#else
#define WL_CAP_2G_MODERN 0
#endif

#if defined(USE_WID_5G) && (USE_WID_5G == 7615 || USE_WID_5G == 7915)
#define WL_CAP_5G_MODERN 1
#else
#define WL_CAP_5G_MODERN 0
#endif

#if defined(USE_WID_5G) && (USE_WID_5G == 7610 || USE_WID_5G == 7612 || USE_WID_5G == 7615 || USE_WID_5G == 7915)
#define WL_CAP_5G_VHT 1
#else
#define WL_CAP_5G_VHT 0
#endif

#if defined(BOARD_HAS_5G_11AX) && BOARD_HAS_5G_11AX
#define WL_CAP_5G_11AX 1
#elif defined(USE_WID_5G) && (USE_WID_5G == 7915)
#define WL_CAP_5G_11AX 1
#else
#define WL_CAP_5G_11AX 0
#endif

#if defined(BOARD_HAS_2G_11AX) && BOARD_HAS_2G_11AX
#define WL_CAP_2G_11AX 1
#elif defined(USE_WID_2G) && (USE_WID_2G == 7915)
#define WL_CAP_2G_11AX 1
#else
#define WL_CAP_2G_11AX 0
#endif

#if defined(BOARD_MT7615_DBDC) || defined(BOARD_MT7915_DBDC)
#define WL_CAP_IS_DBDC 1
#define WL_CAP_5G_VHT160 0
#define WL_CAP_LAN_AP_ISOLATE 0
#else
#define WL_CAP_IS_DBDC 0
#define WL_CAP_5G_VHT160 WL_CAP_5G_MODERN
#define WL_CAP_LAN_AP_ISOLATE 1
#endif

#define WL_CAP_5G_TXBF WL_CAP_5G_MODERN
#define WL_CAP_5G_MUMIMO WL_CAP_5G_MODERN
#define WL_CAP_5G_STBC WL_CAP_5G_MODERN
#define WL_CAP_2G_TURBO_QAM WL_CAP_2G_MODERN

#ifdef CONFIG_BAND_STEERING
#define WL_CAP_BAND_STEERING 1
#else
#define WL_CAP_BAND_STEERING 0
#endif

#ifdef CONFIG_DOT11K_RRM_SUPPORT
#define WL_CAP_RRM 1
#else
#define WL_CAP_RRM 0
#endif

#ifdef CONFIG_WNM_SUPPORT
#define WL_CAP_WNM 1
#else
#define WL_CAP_WNM 0
#endif

#ifdef CONFIG_DOT11R_FT_SUPPORT
#define WL_CAP_FT 1
#else
#define WL_CAP_FT 0
#endif

#ifdef CONFIG_MBO_SUPPORT
#define WL_CAP_MBO 1
#else
#define WL_CAP_MBO 0
#endif

#ifdef CONFIG_VOW_SUPPORT
#define WL_CAP_VOW 1
#else
#define WL_CAP_VOW 0
#endif

#ifdef CONFIG_WHNAT_SUPPORT
#define WL_CAP_WHNAT 1
#else
#define WL_CAP_WHNAT 0
#endif

#if WL_CAP_RRM && WL_CAP_WNM && WL_CAP_5G_MODERN
#define WL_CAP_5G_KV 1
#else
#define WL_CAP_5G_KV 0
#endif

#if WL_CAP_RRM && WL_CAP_WNM && WL_CAP_2G_MODERN
#define WL_CAP_2G_KV 1
#else
#define WL_CAP_2G_KV 0
#endif

#if WL_CAP_FT && WL_CAP_5G_MODERN
#define WL_CAP_5G_FT 1
#else
#define WL_CAP_5G_FT 0
#endif

#if WL_CAP_FT && WL_CAP_2G_MODERN
#define WL_CAP_2G_FT 1
#else
#define WL_CAP_2G_FT 0
#endif

#if WL_CAP_5G_11AX
#define WL_CAP_DEFAULT_5G_GMODE 5
#elif WL_CAP_5G_VHT
#define WL_CAP_DEFAULT_5G_GMODE 4
#else
#define WL_CAP_DEFAULT_5G_GMODE 2
#endif

#if WL_CAP_2G_11AX
#define WL_CAP_DEFAULT_2G_GMODE 6
#else
#define WL_CAP_DEFAULT_2G_GMODE 5
#endif

#if WL_CAP_5G_VHT
#define WL_CAP_DEFAULT_5G_HT_BW 2
#else
#define WL_CAP_DEFAULT_5G_HT_BW 1
#endif

#define WL_CAP_DEFAULT_2G_HT_BW 1

#if WL_CAP_5G_VHT160
#define WL_CAP_MAX_5G_HT_BW 3
#elif WL_CAP_5G_VHT
#define WL_CAP_MAX_5G_HT_BW 2
#else
#define WL_CAP_MAX_5G_HT_BW 1
#endif

#define WL_CAP_MAX_2G_HT_BW 1

static int
wl_cap_get_band_wid(int is_aband)
{
	return (is_aband) ? WL_CAP_5G_WID : WL_CAP_2G_WID;
}

static int
wl_cap_band_is_modern(int is_aband)
{
	return (is_aband) ? WL_CAP_5G_MODERN : WL_CAP_2G_MODERN;
}

static int
wl_cap_has_5g_vht(void)
{
	return WL_CAP_5G_VHT;
}

static int
wl_cap_has_5g_txbf(void)
{
	return WL_CAP_5G_TXBF;
}

static int
wl_cap_has_5g_mumimo(void)
{
	return WL_CAP_5G_MUMIMO;
}

static int
wl_cap_has_5g_vht160(void)
{
	return WL_CAP_5G_VHT160;
}

static int
wl_cap_has_5g_11ax(void)
{
	return WL_CAP_5G_11AX;
}

static int
wl_cap_has_2g_11ax(void)
{
	return WL_CAP_2G_11AX;
}

static int
wl_cap_has_2g_turbo_qam(void)
{
	return WL_CAP_2G_TURBO_QAM;
}

static int
wl_cap_has_lan_ap_isolate(void)
{
	return WL_CAP_LAN_AP_ISOLATE;
}

static int
wl_cap_has_band_steering(void)
{
	return WL_CAP_BAND_STEERING;
}

static int
wl_cap_has_rrm(void)
{
	return WL_CAP_RRM;
}

static int
wl_cap_has_wnm(void)
{
	return WL_CAP_WNM;
}

static int
wl_cap_has_ft(void)
{
	return WL_CAP_FT;
}

static int
wl_cap_has_mbo(void)
{
	return WL_CAP_MBO;
}

static int
wl_cap_has_vow(void)
{
	return WL_CAP_VOW;
}

static int
wl_cap_has_whnat(void)
{
	return WL_CAP_WHNAT;
}

static int
wl_cap_supports_kv(int is_aband)
{
	return (is_aband) ? WL_CAP_5G_KV : WL_CAP_2G_KV;
}

static int
wl_cap_supports_ft(int is_aband)
{
	return (is_aband) ? WL_CAP_5G_FT : WL_CAP_2G_FT;
}

static int
wl_cap_supports_vht160(int is_aband)
{
	return (is_aband) ? WL_CAP_5G_VHT160 : 0;
}

static int
wl_cap_supports_vht_stbc(int is_aband)
{
	return (is_aband) ? WL_CAP_5G_STBC : 0;
}

static int
wl_cap_default_gmode(int is_aband)
{
	return (is_aband) ? WL_CAP_DEFAULT_5G_GMODE : WL_CAP_DEFAULT_2G_GMODE;
}

static int
wl_cap_default_ht_bw(int is_aband)
{
	return (is_aband) ? WL_CAP_DEFAULT_5G_HT_BW : WL_CAP_DEFAULT_2G_HT_BW;
}

static int
wl_cap_max_ht_bw(int is_aband)
{
	return (is_aband) ? WL_CAP_MAX_5G_HT_BW : WL_CAP_MAX_2G_HT_BW;
}

#endif
