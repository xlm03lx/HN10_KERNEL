/*
 * watchdog for preventing the charging work dead
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/mod_devicetable.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/kern_levels.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/hisi/rdr_hisi_ap_ringbuffer.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include "securec.h"
#include "polar_table.h"

#define polar_debug(fmt, args...)do {pr_debug("[polar]" fmt, ## args);} while (0)
#define polar_info(fmt, args...) do {pr_info("[polar]" fmt, ## args);} while (0)
#define polar_err(fmt, args...)  do {pr_err("[polar]" fmt, ## args);} while (0)

static unsigned long polar_sample_interval[POLAR_TIME_ARRAY_NUM] = {
    500,   1500,   3000,   5000,
    10000, 15000,  25000,  35000,
    45000, 65000,  85000,  135000,
    225000,375000, 625000, 1200000
};
static int polar_curr_interval[POLAR_CURR_ARRAY_NUM] = {
    200, 100, 150, 250, 400, 600, 1000, 2000};
static int polar_curr_vector_interval[POLAR_CURR_ARRAY_VECTOR_NUM] = {
    200, 150, 400, 1000};

/*----全极化OCV温度表 ----*/
/*soc(0~100):2.5%一档
55度40度25度10度5度0度-5度-10度-20度-----*/

static int polar_temp_points[POLAR_OCV_PC_TEMP_COLS] = {
    55, 40, 25, 10, 5, 0, -5, -10, -20};
static int polar_pc_points[POLAR_OCV_PC_TEMP_ROWS] = {
    1000,975,950,925,900,
    875,850,825,800,775,
    750,725,700,675,650,
    625,600,575,550,525,
    500,475,450,425,400,
    375,350,325,300,275,
    250,225,200,175,150,
    125,100,75,50,25,0
};

static int polar_resistence_pc_points[POLAR_RES_PC_CURR_ROWS] = {
    950,900,850,800,750,
    700,650,600,550,500,
    450,400,350,300,250,
    200,150,100,50,0
};

static int polar_full_res_temp_points[POLAR_OCV_PC_TEMP_COLS] = {
    55, 40, 25, 10, 5, 0, -5, -10, -20};
static struct hisi_polar_device *g_polar_di = NULL;
static polar_res_tbl polar_res_lut = {
    .rows = POLAR_OCV_PC_TEMP_COLS,
    .cols = POLAR_RES_PC_CURR_ROWS,
    .z_lens = POLAR_CURR_ARRAY_NUM,
    .x_array = polar_full_res_temp_points,
    .y_array = polar_resistence_pc_points,
    .z_array = polar_curr_interval,
};
static polar_x_y_z_tbl polar_vector_lut = {
    .rows = POLAR_RES_PC_CURR_ROWS,
    .cols = POLAR_CURR_ARRAY_NUM,
    .z_lens = POLAR_OCV_PC_TEMP_COLS,
    .x_array = polar_resistence_pc_points,
    .y_array = polar_curr_interval,
    .z_array = polar_temp_points,
};
static polar_ocv_tbl polar_ocv_lut = {
    .rows = POLAR_OCV_PC_TEMP_ROWS,
    .cols = POLAR_OCV_PC_TEMP_COLS,
    .percent = polar_pc_points,
    .temp = polar_temp_points,
};

static struct polar_curr_info polar_avg_curr_info
    [POLAR_TIME_ARRAY_NUM][POLAR_CURR_ARRAY_NUM + 1];
static long polar_err_a = POLAR_ERR_A_DEFAULT;
static long polar_err_b1 = 0;
static long polar_err_b2 = 0;
static long polar_vol_array[POLAR_ARRAY_NUM] = {
    POLAR_VOL_INVALID, POLAR_VOL_INVALID};
static long polar_err_a_array[POLAR_VALID_A_NUM] = {0};
static int polar_err_a_coe[POLAR_VALID_A_NUM + 1] = {25,25,25,25};

 /*******************************************************
  Function:       polar_linear_interpolate
  Description:   get y according to : y = ax +b
  Input:           (x0,y0) (x1,y1) x
  Output:         NULL
  Return:         y conresponding x
  Remark:       a = (y1 - y0) / (x1 - x0)
********************************************************/
static int polar_linear_interpolate(int y0, int x0, int y1, int x1, int x)
{
    if ((y0 == y1) || (x == x0))
        return y0;
    if ((x1 == x0) || (x == x1))
        return y1;

    return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

/*******************************************************
  Function:        interpolate_find_pos
  Description:     找到数组中最接近x的值
  Input:           数组int *x_array(从大到小排)
                    数组大小int rows
                    插值int x
  Output:           插值后数组中最接近x的row1和row2
  Return:          NA
********************************************************/
static void interpolate_find_pos(int *x_array, int rows, int x,
                                    int *row1, int *row2)
{
    int i;

    if (!x_array || !row1 || !row2)
        return;

    if (rows < 1)
        return;
    *row1 = 0;
    *row2 = 0;
    if (x > x_array[0]) {
        x = x_array[0];
    }
    if (x < x_array[rows - 1]) {
        x = x_array[rows - 1];
    }
    for (i = 0; i < rows; i++) {
        if (x == x_array[i]) {
            *row1 = i;
            *row2 = i;
            return;
        }
        if (x > x_array[i]) {
            if (0 == i)
                return;
            else
                *row1 = i - 1;
            *row2 = i;
            break;
        }
    }
}

/*******************************************************
  Function:        interpolate_find_pos_reverse
  Description:     找到数组中最接近x的值
  Input:           数组int *x_array(从小到大排)
                    数组大小int rows
                    插值int x
  Output:           插值后数组中最接近x的row1和row2
  Return:          NA
********************************************************/
static void interpolate_find_pos_reverse(int *x_array, int rows, int x,
                                    int *row1, int *row2)
{
    int i;

    if (!x_array || !row1 || !row2)
        return;

    if (rows < 1)
        return;
    *row1 = 0;
    *row2 = 0;
    if (x < x_array[0]) {
        x = x_array[0];
    }
    if (x > x_array[rows - 1]) {
        x = x_array[rows - 1];
    }
    for (i = 0; i < rows; i++) {
        if (x == x_array[i]) {
            *row1 = i;
            *row2 = i;
            return;
        }
        if (x < x_array[i]) {
            if (0 == i)
                return;
            else
                *row1 = i - 1;
            *row2 = i;
            return;
        }
    }
}

/*******************************************************
  Function:        interpolate_linear_x
  Description:     找到数组中最接近x的值
  Input:           数组int *x_array(从小到大排)
                    数组大小int rows
                    插值int x
  Output:           插值后数组中最接近x的index
  Return:          NA
********************************************************/
static int interpolate_linear_x(int *x_array, int *y_array, int rows, int x)
{
    int row1 = 0;
    int row2 = 0;
    int result = 0;

    if (!x_array || !y_array || 0 >= x)
        return 0;
    if (rows < 1)
        return 0;

    interpolate_find_pos_reverse(x_array, rows, x, &row1, &row2);
    result = polar_linear_interpolate(y_array[row1], x_array[row1],
        y_array[row2], x_array[row2], x);
    return result;
}

/*******************************************************
  Function:       interpolate_two_dimension
  Description:    get y according to : y = ax +b in three dimension
  Input:          struct polar_x_y_tbl *polar_lut ---- lookup table
                  x                    ---- row entry
                  y                    ---- col entry
  Output:         NULL
  Return:         value of two dimension interpolated
********************************************************/
static int interpolate_two_dimension(polar_res_tbl* lut,
                int x, int y, int z)
{
    int tfactor1 = 0, tfactor2 = 0, socfactor1 = 0, socfactor2 = 0;
    int row1 = 0, row2 = 0, col1 = 0, col2 = 0, scalefactor = 0;
    int rows, cols, zcols, z_index;
    int z_res[POLAR_CURR_ARRAY_NUM] = {0};

    if (!lut || (lut->rows < 1) || (lut->cols < 1))
        return 0;

    rows = lut->rows;
    cols = lut->cols;
    zcols = lut->z_lens;
    interpolate_find_pos(lut->x_array, rows, x, &row1, &row2);
    interpolate_find_pos(lut->y_array, cols, y, &col1, &col2);

    if (lut->x_array[row1] != lut->x_array[row2]) {
        tfactor1 = POLAR_INT_COE * (lut->x_array[row1] - x)
            /(lut->x_array[row1] - lut->x_array[row2]);
        tfactor2 = POLAR_INT_COE * (x - lut->x_array[row2])
            /(lut->x_array[row1] - lut->x_array[row2]);
        if (lut->y_array[col1] != lut->y_array[col2]) {
            socfactor1 = POLAR_INT_COE * (lut->y_array[col1] - y)
                /(lut->y_array[col1] - lut->y_array[col2]);
            socfactor2 = POLAR_INT_COE * (y - lut->y_array[col2])
                /(lut->y_array[col1] - lut->y_array[col2]);
        }
    }else if (lut->y_array[col1] != lut->y_array[col2]) {
            socfactor1 = POLAR_INT_COE * (lut->y_array[col1] - y)
                /(lut->y_array[col1] - lut->y_array[col2]);
            socfactor2 = POLAR_INT_COE * (y - lut->y_array[col2])
                /(lut->y_array[col1] - lut->y_array[col2]);
    }

    for (z_index = 0; z_index < zcols; z_index++) {
            if (row1 != row2 && col1 != col2) {
                scalefactor = lut->value[row2][col2][z_index]
                        * tfactor1 * socfactor1
                    + lut->value[row2][col1][z_index]
                        * tfactor1 * socfactor2
                    + lut->value[row1][col2][z_index]
                        * tfactor2 * socfactor1
                    + lut->value[row1][col1][z_index]
                        * tfactor2 * socfactor2;
                z_res[z_index] = scalefactor / (POLAR_INT_COE * POLAR_INT_COE);
                continue;
            }else if (row1 == row2 && col1 != col2) {
                scalefactor = lut->value[row1][col2][z_index]
                        *socfactor1 + lut->value[row1][col1][z_index]
                        * socfactor2;
                z_res[z_index] = scalefactor / POLAR_INT_COE;
                continue;
            }else if (row1 != row2 && col1 == col2) {
                scalefactor = lut->value[row2][col2][z_index] * tfactor1
                    + lut->value[row1][col2][z_index] * tfactor2;
                z_res[z_index] = scalefactor / POLAR_INT_COE;
                continue;
            }else {
                scalefactor = lut->value[row2][col2][z_index];
                z_res[z_index] = scalefactor;
                continue;
            }
        }

    scalefactor = interpolate_linear_x(lut->z_array, z_res, zcols, z);
    return scalefactor;
}

/*******************************************************
  Function:        interpolate_nearest_x
  Description:     找到数组中最接近x的值
  Input:           数组int *x_array(从大到小排)
                    数组大小int rows
                    插值int x
  Output:           插值后数组中最接近x的index
  Return:          NA
********************************************************/
static int interpolate_nearest_x(int *x_array, int rows, int x)
{
    int row1 = 0;
    int row2 = 0;
    int index = 0;

    if (!x_array)
        return 0;

    if (rows < 1)
        return 0;
    interpolate_find_pos(x_array, rows, x, &row1, &row2);
    if (x > (x_array[row1] + x_array[row2])/2)
        index = row1;
    else
        index = row2;
    return index;
}

/*******************************************************
  Function:        interpolate_curr_vector
  Description:     找到数组中最接近x的值
  Input:           数组int *curr_array
                    数组大小int rows
                    插值int x
  Output:           插值后对应矢量表中的电流向量index
  Return:          NA
********************************************************/
static int interpolate_curr_vector(int *x_array, int rows, int x)
{
    int row1 = 0;
    int row2 = 0;
    int index = 0;

    if (!x_array)
        return 0;

    if (rows < 1)
        return 0;

    interpolate_find_pos_reverse(x_array, rows, x, &row1, &row2);
    if (x > TWO_AVG(x_array[row1], x_array[row2]))
        index = row2;
    else
        index = row1;

    return index;
}

/*******************************************************
  Function:       interpolate_polar_ocv
  Description:    look for ocv according to temp, lookup table and pc
  Input:
                  struct pc_temp_ocv_lut *lut      ---- lookup table
                  int batt_temp_degc               ---- battery temperature
                  int pc                           ---- percent of uah
  Output:         NULL
  Return:         percent of uah
********************************************************/
static int interpolate_polar_ocv(polar_ocv_tbl *lut,
                int batt_temp_degc, int pc)
{
    int i, ocvrow1, ocvrow2, ocv;
    int rows, cols;
    int row1 = 0;
    int row2 = 0;
    if( NULL == lut ) {
        polar_err("NULL point in [%s]\n", __func__);
        return -1;
    }

    if (0 >= lut->rows || 0 >= lut->cols) {
        polar_err("lut mismatch [%s]\n", __func__);
        return -1;
    }
    rows = lut->rows;
    cols = lut->cols;
    interpolate_find_pos(lut->percent, rows, pc, &row1, &row2);

    if (batt_temp_degc > lut->temp[0])
        batt_temp_degc = lut->temp[0];
    if (batt_temp_degc < lut->temp[cols - 1])
        batt_temp_degc = lut->temp[cols - 1];

    for (i = 0; i < cols; i++)
        if (batt_temp_degc >= lut->temp[i])
            break;
    if ((batt_temp_degc == lut->temp[i]) || (0 == i)) {
        ocv = polar_linear_interpolate(
        lut->ocv[row1][i],
        lut->percent[row1],
        lut->ocv[row2][i],
        lut->percent[row2],
        pc);
        return ocv;
    }

    ocvrow1 = polar_linear_interpolate(
        lut->ocv[row1][i - 1],
        lut->temp[i - 1],
        lut->ocv[row1][i],
        lut->temp[i],
        batt_temp_degc);

    ocvrow2 = polar_linear_interpolate(
        lut->ocv[row2][i - 1],
        lut->temp[i - 1],
        lut->ocv[row2][i],
        lut->temp[i],
        batt_temp_degc);

    ocv = polar_linear_interpolate(
        ocvrow1,
        lut->percent[row1],
        ocvrow2,
        lut->percent[row2],
        pc);

    return ocv;
}

/*******************************************************
  Function:        get_polar_vector
  Description:     获取极化矢量数据
  Input:           极化矢量表polar_x_y_z_tbl* polar_vector_lut
                   电量中心值int soc
                   当前电池温度temp
                   极化电流int curr
  Output:          极化矢量值
  Return:          NA
********************************************************/
static int get_polar_vector_value(polar_x_y_z_tbl* lut,
    int batt_temp_degc, int soc, int curr, int t_index)
{
    int x_soc, y_curr, z_temp;
    if (NULL == lut || t_index > POLAR_TIME_ARRAY_NUM
        ||t_index < 0 || curr <= 0)
        return 0;
    z_temp =  interpolate_nearest_x(lut->z_array, lut->z_lens, batt_temp_degc);
    x_soc = interpolate_nearest_x(lut->x_array, lut->rows, soc);
    y_curr = interpolate_curr_vector(polar_curr_vector_interval,
        POLAR_CURR_ARRAY_VECTOR_NUM, curr);
    return lut->value[z_temp][x_soc][y_curr][t_index];
}

/*******************************************************
  Function:        get_polar_vector_res
  Description:     获取极化矢量数据
  Input:           极化矢量表polar_x_y_z_tbl* polar_vector_lut
                   电量中心值int soc
                   当前电池温度temp
                   极化电流int curr
  Output:          极化矢量内阻
  Return:          NA
********************************************************/
static int get_polar_vector_res(polar_res_tbl* lut,
    int batt_temp_degc, int soc, int curr)
{
    int soc_index, curr_index, batt_temp_index, res;
    if (NULL == lut || soc <= 0 || curr <= 0)
        return 0;
    batt_temp_index = interpolate_nearest_x(lut->x_array, lut->rows,
                        batt_temp_degc);
    soc_index = interpolate_nearest_x(lut->y_array, lut->cols, soc);
    curr_index = interpolate_curr_vector(polar_curr_vector_interval,
        POLAR_CURR_ARRAY_VECTOR_NUM, curr);
    res = interpolate_two_dimension(lut, lut->x_array[batt_temp_index],
            lut->y_array[soc_index], polar_curr_vector_interval[curr_index]);
    return res;
}

/*******************************************************
  Function:        update_polar_error_b
  Description:     更新极化电压b值
  Input:           电量查表OCV ocv_soc_mv
                   当前电压 vol_now_mv
                   当前极化电压 polar_vol_uv
  Output:          更新后的极化电压b值
  Return:          NA
********************************************************/
static void update_polar_error_b(int ocv_soc_mv, int vol_now_mv,
                                    long polar_vol_uv)
{
    struct hisi_polar_device *di = g_polar_di;
    int i;

    if (NULL == di)
        return;
    /*judge if can update polar_b with current polar voltage */
    if (VPERT_NOW_LOW_B > polar_vol_uv || VPERT_NOW_HIGH_B < polar_vol_uv)
        return;

    /*judge if can update polar_b with former two polar voltages */
    for (i = 0; i < POLAR_ARRAY_NUM; i++) {
        if (POLAR_VOL_INVALID == polar_vol_array[i])
            return;
        if (VPERT_PAST_LOW_B > polar_vol_array[i]
            ||VPERT_PAST_HIGH_B < polar_vol_array[i])
            return;
    }

    /*calculate polar b*/
    polar_err_b1 = (long)(ocv_soc_mv - vol_now_mv) * UVOLT_PER_MVOLT;
    polar_err_b2 = polar_vol_uv;
    #ifdef POLAR_INFO_DEBUG
    polar_info("update polar b1,b2:%ld:%ld\n", polar_err_b1, polar_err_b2);
    #endif
    return;
}

/*******************************************************
  Function:        update_polar_error_a
  Description:     更新极化电压a值
  Input:           电量查表OCV ocv_soc_mv
                   当前电压 vol_now_mv
                   当前极化电压 polar_vol_uv
  Output:          更新后的极化电压a值
  Return:          NA
********************************************************/
static void update_polar_error_a(int ocv_soc_mv, int vol_now_mv,
                                    long polar_vol_uv)
{
    int i = 0;
    long temp_err_a = 0;
    long temp_err_a_wavg = 0;
    static int polar_valid_a_index = 0;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return;

    polar_vol_uv = polar_vol_uv - polar_err_b2;
    if ((VPERT_NOW_LOW_A <= polar_vol_uv)
        &&(VPERT_NOW_HIGH_A >= polar_vol_uv)) {
        return;
    }
    temp_err_a = ((long)(ocv_soc_mv  - vol_now_mv) * UVOLT_PER_MVOLT
                - polar_err_b1) * POLAR_ERR_COE_MUL / (-polar_vol_uv);
    /*if polar_a was negative,we use last max average current instead*/
    if (temp_err_a < 0) {
        if (0 != di->last_max_avg_curr)
            polar_err_a = temp_err_a;
        else
            polar_err_a = POLAR_ERR_A_DEFAULT;
        return;
    }
    /*clamp a in range[0.9~3]*/
    temp_err_a = clamp_val(temp_err_a, POLAR_ERR_A_MIN, POLAR_ERR_A_MAX);
    #ifdef POLAR_INFO_DEBUG
    polar_info("%s:polar a before weighted average:%ld\n", __func__, temp_err_a);
    #endif
    /*calculate weighted average of A with current A and former three A values*/
    for (i = 0; i < POLAR_VALID_A_NUM; i++) {
        #ifdef POLAR_INFO_DEBUG
        polar_info("%s:a_array[%d]:%ld,coe:%d\n", __func__,
            i, polar_err_a_array[i], polar_err_a_coe[i]);
        #endif
        if (0 == polar_err_a_array[i])
            temp_err_a_wavg += (temp_err_a * polar_err_a_coe[i]);
        else
            temp_err_a_wavg += (polar_err_a_array[i] * polar_err_a_coe[i]);
    }
    polar_err_a = (temp_err_a * polar_err_a_coe[POLAR_VALID_A_NUM]
        +temp_err_a_wavg) / POLAR_A_COE_MUL;

    polar_err_a_array [polar_valid_a_index % POLAR_VALID_A_NUM] =
        temp_err_a;
    polar_valid_a_index++;
    polar_valid_a_index = polar_valid_a_index % POLAR_VALID_A_NUM;
#ifdef POLAR_INFO_DEBUG
    polar_info("%s:update real polar a:%ld, weighted average a:%ld\n",
        __func__, temp_err_a, polar_err_a);
#endif
    return;
}
/*******************************************************
  Function:        get_estimate_max_avg_curr
  Description:     计算最大负载电流
  Input:           全局设备信息指针di
                   soc当前电量
                   polar_vol当前极化电压
                   polar_past过去5s-20min内的极化电压
                   电池温度batt_temp_degc
  Output:
  Return:         最大负载电流(mA)
********************************************************/
static int get_estimate_max_avg_curr(int ocv_soc_mv, int vol_now,
                int soc, long polar_vol, long polar_past,
                int batt_temp_degc, int v_cutoff, int r_pcb)
{
    int curr_index = 0, t_index = 0;
    long polar_vol_future = 0;
    int polar_res_future = 0;
    int curr_future = 0;
    int curr_tmp = 0;
    int curr_thresh = 0;
    int res, res_zero, res_vector, polar_vector;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return 0;

    if ((0 != di->last_max_avg_curr) && (polar_err_a < 0)) {
        return di->last_max_avg_curr;
    }
    /*calculate polar_vol_future in uV*/
    polar_vol_future = (long)(ocv_soc_mv - v_cutoff) * UVOLT_PER_MVOLT
            -(polar_err_b1 + polar_err_a * polar_err_b2 / POLAR_ERR_COE_MUL)
            +(polar_past * polar_err_a/POLAR_ERR_COE_MUL);
#ifdef POLAR_INFO_DEBUG
        polar_info("%s:polar_vol_future:%ld,ocv_soc_mv:%d,\
v_cutoff:%d,polar_err_b1:%ld,polar_err_b2:%ld,polar_err_a:%ld,\
polar_past:%ld\n", __func__, polar_vol_future, ocv_soc_mv, v_cutoff,
            polar_err_b1, polar_err_b2, polar_err_a, polar_past);
#endif
    for (curr_index = POLAR_CURR_ARRAY_VECTOR_NUM -1; curr_index >= 0;
        curr_index--) {
        curr_future = polar_curr_vector_interval[curr_index];
        polar_vector = 0;
        res = interpolate_two_dimension(&polar_res_lut,
            batt_temp_degc, soc, curr_future);
        res_vector = get_polar_vector_res(&polar_res_lut,
            batt_temp_degc, soc, curr_future);
        for (t_index = 0; t_index < POLAR_VECTOR_5S; t_index++){
            polar_vector += get_polar_vector_value(&polar_vector_lut,
                batt_temp_degc, soc, curr_future, t_index + 1);
        }
        res_zero = get_polar_vector_value(&polar_vector_lut,
            batt_temp_degc, soc, curr_future, 0);
        if (0 != res_vector)
            polar_res_future = ((res_zero + polar_vector) * res) / res_vector;
#ifdef POLAR_INFO_DEBUG
        polar_info("%s:res_zero:%d, polar_vector:%d, polar_res_future:%d,\
res_vector:%d, res:%d\n", __func__, res_zero, polar_vector,
            polar_res_future, res_vector, res);
#endif
        /*calculate polar_res_future in mΩ*/
        polar_res_future = (polar_res_future * polar_err_a)
            /(POLAR_ERR_COE_MUL * POLAR_RES_MHOM_MUL) + r_pcb / UOHM_PER_MOHM;
        if (0 != polar_res_future)
            curr_tmp = (int)(polar_vol_future / polar_res_future);
#ifdef POLAR_INFO_DEBUG
        polar_info("%s:polar_vol_future:%ld,polar_res_future:%d,\n",
            __func__, polar_vol_future, polar_res_future);
#endif
        if (curr_index > 0)
            curr_thresh = TWO_AVG(polar_curr_vector_interval[curr_index],
                                polar_curr_vector_interval[curr_index - 1]);
        else
            curr_thresh = polar_curr_vector_interval[0];
        if (curr_thresh <= curr_tmp) {
                curr_future = curr_tmp;
                polar_vector = 0;
                res = interpolate_two_dimension(&polar_res_lut,
                    batt_temp_degc, soc, curr_future);
                res_vector = get_polar_vector_res(&polar_res_lut,
                    batt_temp_degc, soc, curr_future);
                for (t_index = 0; t_index < POLAR_VECTOR_5S; t_index++){
                    polar_vector += get_polar_vector_value(&polar_vector_lut,
                        batt_temp_degc, soc, curr_future, t_index + 1);
                }
                res_zero = get_polar_vector_value(&polar_vector_lut,
                    batt_temp_degc, soc, curr_future, 0);
                if (0 != res_vector)
                    polar_res_future = ((res_zero + polar_vector) * res)
                        /res_vector;
#ifdef POLAR_INFO_DEBUG
                polar_info("%s:predict:curr_future:%d,res_zero:%d,\
polar_res_future:%d\n", __func__, curr_future,
                    res_zero, polar_res_future);
#endif
                /*calculate polar_res_future in mΩ*/
                polar_res_future = (polar_res_future * polar_err_a)
                    /(POLAR_ERR_COE_MUL * POLAR_RES_MHOM_MUL)
                        + r_pcb / UOHM_PER_MOHM;
                if (0 != polar_res_future)
                    curr_tmp = (int)(polar_vol_future / polar_res_future);
                di->polar_res_future = polar_res_future;
                di->last_max_avg_curr = curr_tmp;
#ifdef POLAR_INFO_DEBUG
                polar_info("%s:predict:curr_future:%d,polar_res_future:%d\n",
                    __func__, curr_tmp,polar_res_future);
#endif
                return curr_tmp;
            }
    }
    di->polar_res_future = polar_res_future;
    di->last_max_avg_curr = curr_tmp;
    return curr_tmp;
}

/*******************************************************
  Function:        get_estimate_peak_curr
  Description:     计算最大峰值电流
  Input:           全局设备信息指针di
                   soc当前电量
                   电池温度batt_temp_degc
  Output:
  Return:         最大峰值电流(mA)
********************************************************/
static int get_estimate_peak_curr(int ocv_soc_mv, long polar_vol, int soc,
                        int batt_temp_degc, int v_cutoff, int r_pcb)
{
    int curr_index, res_zero, curr_future, curr_temp;
    int curr_thresh, res, res_vector;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return 0;

    if ((0 != di->last_max_avg_curr) && (polar_err_a < 0)) {
        return di->last_max_avg_curr;
    }

    for (curr_index = POLAR_CURR_ARRAY_VECTOR_NUM -1; curr_index >= 0;
            curr_index--) {
        curr_future = polar_curr_vector_interval[curr_index];
        res = interpolate_two_dimension(&polar_res_lut,
            batt_temp_degc, soc, curr_future);
        res_vector = get_polar_vector_res(&polar_res_lut,
            batt_temp_degc, soc, curr_future);
        res_zero = get_polar_vector_value(&polar_vector_lut,
            batt_temp_degc, soc, curr_future, 0);
        if (0 != res_vector)
            res_zero = (res_zero * res) / res_vector;
        /*voltage in uv,resistence in mΩ,curr in mA*/
        curr_temp = ((long)(ocv_soc_mv - v_cutoff) * UVOLT_PER_MVOLT
              -(polar_err_b1 + polar_err_a * polar_err_b2 / POLAR_ERR_COE_MUL))
                    /(r_pcb / UOHM_PER_MOHM + polar_err_a * res_zero
                        / (POLAR_RES_MHOM_MUL * POLAR_ERR_COE_MUL));
#ifdef POLAR_INFO_DEBUG
        polar_info("%s:curr_future:%d, res_zero:%d, curr_temp:%d,\
ocv_soc_mv:%d, v_cutoff:%d\n", __func__, curr_future, res_zero,
                curr_temp, ocv_soc_mv, v_cutoff);
#endif
        if (curr_index > 0)
            curr_thresh = TWO_AVG(polar_curr_vector_interval[curr_index],
                                polar_curr_vector_interval[curr_index - 1]);
        else
            curr_thresh = polar_curr_vector_interval[0];
        if (curr_thresh <= curr_temp) {
            curr_future = curr_temp;
            res = interpolate_two_dimension(&polar_res_lut,
                batt_temp_degc, soc, curr_future);
            res_vector = get_polar_vector_res(&polar_res_lut,
                batt_temp_degc, soc, curr_future);
            res_zero = get_polar_vector_value(&polar_vector_lut,
                batt_temp_degc, soc, curr_future, 0);
            if (0 != res_vector)
                res_zero = (res_zero * res) / res_vector;
            /*voltage in uv,resistence in mΩ,curr in mA*/
            curr_temp = ((long)(ocv_soc_mv - v_cutoff) * UVOLT_PER_MVOLT
               -(polar_err_b1 + polar_err_a * polar_err_b2 / POLAR_ERR_COE_MUL))
                        /(r_pcb / UOHM_PER_MOHM + polar_err_a * res_zero
                            /(POLAR_RES_MHOM_MUL * POLAR_ERR_COE_MUL));
#ifdef POLAR_INFO_DEBUG
            polar_info("%s:predict:curr_future:%d, res_zero:%d, curr_temp:%d,\
ocv_soc_mv:%d, v_cutoff:%d\n", __func__, curr_future, res_zero,
                curr_temp, ocv_soc_mv, v_cutoff);
#endif
            di->last_max_peak_curr = curr_temp;
            return curr_temp;
        }
    }
    di->last_max_peak_curr = curr_temp;
    return curr_temp;
}

/*******************************************************
  Function:        calculate_polar_volatge
  Description:     计算极化电压
  Input:           全局设备信息指针di
                   极化矢量表polar_x_y_z_tbl* polar_vector_lut
                   极化阻抗表polar_res_tbl* polar_res_lut
                   当前电量int soc
                   当前电池温度temp
                   当前电流curr
  Output:
  Return:          极化电压(uV)
********************************************************/
static long calculate_polar_volatge(int soc, int temp, int curr)
{
    int t_index;
    int curr_index;
    int soc_avg, curr_avg, ratio;
    int res, res_vector, polar_vector;
    long vol_sum = 0;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return 0;
    for (t_index = 0; t_index < POLAR_TIME_ARRAY_NUM; t_index++) {
        for (curr_index = 0; curr_index < (POLAR_CURR_ARRAY_NUM + 1);
                curr_index++) {
            soc_avg = polar_avg_curr_info[t_index][0].soc_avg;
            curr_avg = polar_avg_curr_info[t_index][curr_index].current_avg;
            ratio = polar_avg_curr_info[t_index][curr_index].duration;
            if (0 == ratio || 0 == soc_avg || 0 == curr_avg)
                continue;
            res = interpolate_two_dimension(&polar_res_lut,
                    temp, soc_avg, abs(curr_avg));
            res_vector = get_polar_vector_res(&polar_res_lut,
                    temp, soc_avg, abs(curr_avg));
            polar_vector = get_polar_vector_value(&polar_vector_lut,
                    temp, soc_avg, abs(curr_avg), t_index + 1);
            /*curr_avg is in ma, ratio is in percentage, res is in 0.1mΩ*/
            if (0 != res_vector)
                vol_sum += (long)curr_avg * ((((long)ratio
                    *(res / POLAR_RES_MHOM_MUL))/POLAR_RATIO_PERCENTAGE)
                        *polar_vector) / res_vector;
        }
    }
    return vol_sum;
}

/*******************************************************
  Function:        calculate_polar_vol_r0
  Description:     计算极化电压
  Input:           全局设备信息指针di
                   极化矢量表polar_x_y_z_tbl* polar_vector_lut
                   极化阻抗表polar_res_tbl* polar_res_lut
                   当前电量int soc
                   当前电池温度temp
                   当前电流curr
  Output:
  Return:          R0对应的极化电压(uV)
********************************************************/
static long calculate_polar_vol_r0(int soc, int temp, int curr)
{
    int res, res_zero, res_vector, polar_vector;
    long polar_vol = 0;

    res = interpolate_two_dimension(&polar_res_lut, temp, soc, abs(curr));
    res_vector = get_polar_vector_res(&polar_res_lut, temp, soc, abs(curr));
    polar_vector = get_polar_vector_value(&polar_vector_lut, temp, soc,
                    abs(curr), 0);
    if (0 != res_vector)
        res_zero = (polar_vector * res) / res_vector;
    else
        res_zero = polar_vector;
    polar_vol =  (long)curr * res_zero / POLAR_RES_MHOM_MUL;
    return polar_vol;
}

/*******************************************************
  Function:        add_polar_info
  Description:     极化数据求和
  Input:          struct ploarized_info *ppolar, int t_index
  Output:         t_index某个时间区间内的极化数据求和
  Return:          NA
********************************************************/
static void add_polar_info(int current_ma, int duration, int t_index)
{
    int curr_index;

    if (0 > t_index || POLAR_TIME_ARRAY_NUM <= t_index || 0 >= duration)
        return;
    /*找到对应的电流区间*/
    for (curr_index = 0; curr_index < POLAR_CURR_ARRAY_NUM; curr_index++){
        if (abs(current_ma) <= polar_curr_interval[curr_index])
            break;
    }
    /*反极化是否需要考虑，方案中没有标明，电流用绝对值代替*/
    /*电流大于2c的情况也要计算到POLAR_CURR_ARRAY_NUM*/
    polar_avg_curr_info[t_index][curr_index].current_avg +=
                                    ((long)current_ma * duration);
    polar_avg_curr_info[t_index][curr_index].cnt++;
    polar_avg_curr_info[t_index][curr_index].duration += duration;
}

static void update_polar_avg_curr(void)
{
    int time_interval = 0;
    int t_index = 0;
    int curr_index = 0;
    /*求Tn区间内的各电流区间平均电流*/
    for (t_index = 0; t_index < POLAR_TIME_ARRAY_NUM; t_index++) {
        for (curr_index = 0; curr_index < (POLAR_CURR_ARRAY_NUM + 1);
                    curr_index++){
            /*平均电流计算cc/t*/
            if (0 != polar_avg_curr_info[t_index][curr_index].duration) {
                polar_avg_curr_info[t_index][curr_index].current_avg /=
                polar_avg_curr_info[t_index][curr_index].duration;
            /*电流对应时间比例计算*/
                if (0 != t_index)
                    time_interval = (int)(polar_sample_interval[t_index]
                                    - polar_sample_interval[t_index - 1]);
                else
                    time_interval = (int)polar_sample_interval[t_index];
                polar_avg_curr_info[t_index][curr_index].duration =
                    (polar_avg_curr_info[t_index][curr_index].duration
                        * POLAR_RATIO_PERCENTAGE) / time_interval;
                if (polar_avg_curr_info[t_index][curr_index].duration >
                        POLAR_RATIO_PERCENTAGE)
                    polar_avg_curr_info[t_index][curr_index].duration =
                        POLAR_RATIO_PERCENTAGE;
            }
#ifdef POLAR_INFO_DEBUG
            polar_info("polar_avg_curr_info[%d][%d]|duration_ratio|soc_avg:\
%lld ma,%ld%%,%d\n", t_index, curr_index,
                polar_avg_curr_info[t_index][curr_index].current_avg,
                polar_avg_curr_info[t_index][curr_index].duration,
                polar_avg_curr_info[t_index][0].soc_avg);
#endif
        }
    }
}

/*******************************************************
  Function:        polar_info_calc
  Description:     极化数据计算
  Input:          struct smartstar_coul_device *di
  Output:         分Tn区间极化电流和ratio、电量中心值
  Return:          NA
********************************************************/
static void polar_info_calc(struct hisi_polar_device *di, int predict_msec)
{
    int t_index = 0, last_soc_avg;
    struct ploarized_info *ppolar, *ppolar_head;
    struct list_head *pos;
    unsigned long node_sample_time = 0, node_duration_time = 0;
    unsigned long temp_duration_time = 0, head_sample_time = 0;

    if (NULL == di || NULL == di->polar_buffer) {
        polar_err("[polar] %s di is null.\n", __func__);
        return;
    }
    if (list_empty(&(di->polar_head.list)))
        return;
    memset_s(polar_avg_curr_info, sizeof(polar_avg_curr_info),
            0, sizeof(polar_avg_curr_info));
    ppolar_head = list_first_entry(&(di->polar_head.list),
                        struct ploarized_info, list);
    last_soc_avg = ppolar_head->soc_now;
    head_sample_time = ppolar_head->sample_time + predict_msec;
    /*遍历循环链表，求Tn时间内各电流区间的平均电流*/
    list_for_each(pos, &(di->polar_head.list)){
        ppolar = list_entry(pos, struct ploarized_info, list);
        node_sample_time = ppolar->sample_time;
        node_duration_time = ppolar->duration;
        /*--big data divide start--*/
        while (POLAR_TIME_ARRAY_NUM > t_index) {
             /*sample end time of each node is in Tn*/
            if (time_after_eq(node_sample_time + polar_sample_interval[t_index],
                            head_sample_time)) {
                /*sample start time of each node is in Tn*/
                if (time_after_eq(node_sample_time - node_duration_time
                        + polar_sample_interval[t_index], head_sample_time)) {
                    add_polar_info(ppolar->current_ma, (int)node_duration_time,
                            t_index);
                    break;
                } else{
                /*sample start time of each node is not in Tn, divide node*/
                    temp_duration_time = node_sample_time
                        -(head_sample_time - polar_sample_interval[t_index]);
                    add_polar_info(ppolar->current_ma, (int)temp_duration_time,
                            t_index);
                    node_duration_time = node_duration_time
                        -temp_duration_time;
                    node_sample_time = head_sample_time
                        -polar_sample_interval[t_index];
                    /*求Tn区间内的电量中心值*/
                    polar_avg_curr_info[t_index][0].soc_avg =
                        TWO_AVG(ppolar->soc_now, last_soc_avg);
                    last_soc_avg = ppolar->soc_now;
                    /*节点的时间相对头结点超过Tn，进入下一个Tn区间*/
                    t_index++;
                }
        } else {
        /*sample end time of each node is in Tn*/
            /*求Tn区间内的电量中心值*/
                polar_avg_curr_info[t_index][0].soc_avg =
                        TWO_AVG(ppolar->soc_now, last_soc_avg);
                last_soc_avg = ppolar->soc_now;
                /*节点的时间相对头结点超过Tn，进入下一个Tn区间*/
                t_index++;
            }
        }
    }
    update_polar_avg_curr();
}

/*******************************************************
  Function:        fill_up_polar_fifo
  Description:     极化数据放入fifo中
  Input:           极化数据struct ploarized_info *ppolar
                   head:极化数据索引的链表头
                   rbuffer:极化数据存放的循环buffer
                   total_sample_time:该buffer存放数据的总采样时间
  Output:       NA
  Return:       NA
********************************************************/
static void fill_up_polar_fifo(struct hisi_polar_device *di,
                                struct ploarized_info* ppolar,
                                struct list_head* head,
                                struct hisiap_ringbuffer_s *rbuffer,
                                unsigned long total_sample_time)
{
    struct ploarized_info *ppolar_head;
    struct ploarized_info *ppolar_tail;
    struct ploarized_info* ppolar_buff;
    u32 buff_pos = 0;

    if (NULL == ppolar || NULL == rbuffer || NULL == head || NULL == di)
        return;

    if (!list_empty(head)){
        ppolar_tail = list_last_entry(head, struct ploarized_info, list);
        ppolar_head = list_first_entry(head, struct ploarized_info, list);
        /*judge if the node is before the head sample time*/
        if (time_before(ppolar->sample_time,
                ppolar_head->sample_time + di->fifo_interval)) {
            return;
        }
        /*judge if we need to del node after total_sample_time*/
        if (time_after(ppolar->sample_time,
            (ppolar_tail->sample_time + total_sample_time))) {
            list_del(&ppolar_tail->list);
        }
    }
    hisiap_ringbuffer_write(rbuffer, (u8*)ppolar);
    /*this is to get the buff_pos after write*/
    if (0 == rbuffer->rear) {
        if (rbuffer->is_full)
            buff_pos = rbuffer->max_num;
        else {
            polar_err("[%s]:ringbuffer write failed\n",__func__);
            return;
        }
    }else{
        buff_pos = rbuffer->rear - 1;
    }
    ppolar_buff = (struct ploarized_info*)
        &rbuffer->data[(unsigned long)buff_pos * rbuffer->field_count];
    list_add(&(ppolar_buff->list), head);
}

static unsigned long polar_get_head_time(struct hisi_polar_device *di,
                                              struct list_head* head)
{
    struct ploarized_info *ppolar_head;
    unsigned long sample_time;

    if (NULL == di || NULL == head)
        return 0;
    if (list_empty(head)){
        return 0;
    }
    ppolar_head = list_first_entry(head, struct ploarized_info, list);
    sample_time = ppolar_head->sample_time;
    return sample_time;
}

/*******************************************************
  Function:        sample_timer_func
  Description:     coul fifo sample time callback
  Input:           struct hrtimer *timer
  Output:          NA
  Return:          NA
********************************************************/
static enum hrtimer_restart sample_timer_func(struct hrtimer *timer)
{
    struct hisi_polar_device *di = g_polar_di;
    struct ploarized_info node;
    unsigned long sample_time, flags, fifo_time_ms;
    int fifo_depth, current_ua, i;
    ktime_t kt;

    if (NULL == di)
        return HRTIMER_NORESTART;
    spin_lock_irqsave(&di->coul_fifo_lock, flags);
    /*get coul fifo according to the fifo depth*/
    fifo_depth = hisi_coul_battery_fifo_depth();
    sample_time = hisi_getcurtime();
    sample_time = sample_time / NSEC_PER_MSEC;
    for (i = fifo_depth - 1; i >= 0; i--)
    {
        node.sample_time = sample_time - ((unsigned long)i*di->fifo_interval);//lint !e571
        current_ua = hisi_coul_battery_fifo_curr(i);
        node.current_ma = - (current_ua / UA_PER_MA);
        node.duration = di->fifo_interval;
        node.temperature = hisi_battery_temperature();
        node.soc_now = hisi_coul_battery_ufcapacity_tenth();
        node.list.next = NULL;
        node.list.prev = NULL;
#ifdef POLAR_INFO_DEBUG
        polar_info("%s:time:%llu,curr:%d,duration:%llu,temp:%d:soc:%d\n", __func__,
        node.sample_time, node.current_ma, node.duration,
        node.temperature, node.soc_now);
#endif
        /*here we put the fifo info to 30S ringbuffer*/
        fill_up_polar_fifo(di, &node, &di->coul_fifo_head.list, di->fifo_buffer,
                           COUL_FIFO_SAMPLE_TIME);
    }
    fifo_time_ms = (unsigned long)fifo_depth * di->fifo_interval;//lint !e571
    kt = ktime_set(fifo_time_ms / MSEC_PER_SEC,
                    (fifo_time_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
    hrtimer_forward_now(timer, kt);
    spin_unlock_irqrestore(&di->coul_fifo_lock, flags);
    return HRTIMER_RESTART;
}

/*******************************************************
  Function:        get_polar_vol
  Description:     获取极化电压
  Input:          soc_now:0.1% percent
                  batt_temp:℃
                  curr_now:ma
  Output:
  Return:          极化电压(uV)
********************************************************/
static long get_polar_vol(struct hisi_polar_device *di, int soc_now,
                            int batt_temp, int curr_now)
{
    long vol = 0;

    if (NULL == di)
        return 0;
    vol = calculate_polar_volatge(soc_now, batt_temp, curr_now);
    vol += calculate_polar_vol_r0(soc_now, batt_temp, curr_now);
    return vol;
}

/*******************************************************
  Function:       polar_ocv_could_update
  Description:    judge if ocv update condition meets
  Input:
                  int soc_now ---- soc percentage in 0.1%
                  int curr_now ---- current in ma
                  int temp_now ---- battery temperature:℃
                  int batt_vol_now ---- battery voltage in mv
  Output:         NULL
  Return:         TRUE/FALSE
********************************************************/
static bool polar_ocv_could_update(int soc_now, int curr_now,
                                        int temp_now, int batt_vol_now)
{
#ifdef POLAR_INFO_DEBUG
    polar_info("%s:soc_now:%d,curr_now:%d,temp_now:%d\n", __func__,
                soc_now, curr_now, temp_now);
#endif
    if (abs(curr_now) >= POLAR_CURR_OCV_UPDATE)
        return FALSE;
        else if (temp_now < POLAR_TEMP_OCV_UPDATE)
            return FALSE;
            else if (soc_now <= POLAR_SOC_OCV_UPDATE)
                return FALSE;
                else if (batt_vol_now < 0)
                    return FALSE;
    return TRUE;
}

/*******************************************************
  Function:       polar_ocv_calc
  Description:    predict polar ocv
  Input:
                  int soc_now ---- soc percentage in 0.1%
                  int curr_now ---- current in ma
                  int temp_now ---- battery temperature:℃
                  int batt_vol_now ---- battery voltage in mv
  Output:         NULL
  Return:         TRUE/FALSE
********************************************************/
static int polar_ocv_calc(int soc_now, int curr_now,
                             int temp_now, int batt_vol_now)
{
    long polar_vol_uv;
    int ocv_estimate_mv;

    polar_vol_uv = calculate_polar_volatge(soc_now, temp_now, curr_now);
    polar_vol_uv += calculate_polar_vol_r0(soc_now, temp_now, curr_now);
    ocv_estimate_mv = batt_vol_now - (int)(polar_vol_uv / UVOLT_PER_MVOLT);
    return ocv_estimate_mv;
}

/*******************************************************
  Function:       get_polar_ocv
  Description:    look for ocv according to temp, lookup table and pc
  Input:
                  int soc_now ---- soc percentage in 0.1%
                  int curr_now ---- current in ma
                  int temp_now ---- battery temperature:℃
                  int batt_vol_now ---- battery voltage in mv
  Output:         NULL
  Return:         ocv estimated
********************************************************/
static int get_polar_ocv(int soc_now, int curr_now,
                            int temp_now, int batt_vol_now)
{
    int soc_next, last_soc, soc_ocv_mv, min_ocv_gap, min_ocv_gap_index;
    int last_ocv_estimate, last_soc_ocv, i;
    int ocv_estimate_mv = 0, ocv_approach = 0;

    if (FALSE == polar_ocv_could_update(soc_now, curr_now,
                        temp_now, batt_vol_now))
        return 0;

    soc_next = soc_now - OCV_STEP_START * POLAR_SOC_STEP;
    soc_ocv_mv = interpolate_polar_ocv(&polar_ocv_lut, temp_now, soc_next);
    ocv_estimate_mv = polar_ocv_calc(soc_next, curr_now,
                        temp_now, batt_vol_now);
    min_ocv_gap = abs(ocv_estimate_mv - soc_ocv_mv);
    min_ocv_gap_index = OCV_STEP_START;

    /*calc and compare 9 points of soc vary from -10% to +10% based on soc_now*/
    for (i = OCV_STEP_START + 1; i <= OCV_STEP_END; i++) {
        last_soc = soc_next;
        last_ocv_estimate = ocv_estimate_mv;
        last_soc_ocv = soc_ocv_mv;
        soc_next =  soc_now + (i * POLAR_SOC_STEP);
        soc_ocv_mv = interpolate_polar_ocv(&polar_ocv_lut, temp_now, soc_next);
        ocv_estimate_mv = polar_ocv_calc(soc_next, curr_now,
                            temp_now, batt_vol_now);
        if (0 > ((ocv_estimate_mv - soc_ocv_mv)
                *(last_ocv_estimate - last_soc_ocv))) {
            /*socs estimated and socs from ocv table cross*/
            ocv_approach = last_ocv_estimate
                +(last_ocv_estimate - ocv_estimate_mv)
                    *(last_ocv_estimate - last_soc_ocv)
                        /(last_ocv_estimate - last_soc_ocv
                            +soc_ocv_mv - ocv_estimate_mv);
            return ocv_approach;
        } else if (min_ocv_gap > abs(ocv_estimate_mv - soc_ocv_mv)) {
   /*socs estimated and socs from ocv table do not cross, find the minimu gap*/
            min_ocv_gap = abs(ocv_estimate_mv - soc_ocv_mv);
            min_ocv_gap_index = i;
        }
    }
#ifdef POLAR_INFO_DEBUG
    polar_info("batt_vol_now:%d,min_ocv_gap:%d, min_ocv_gap_index:%d,\
soc_now:%d\n", batt_vol_now, min_ocv_gap, min_ocv_gap_index, soc_now);
#endif
    soc_next =  soc_now + (min_ocv_gap_index * POLAR_SOC_STEP);
    ocv_estimate_mv = polar_ocv_calc(soc_next, curr_now,
                        temp_now, batt_vol_now);
    return ocv_estimate_mv;
}

/*******************************************************
  Function:        copy_fifo_buffer
  Description:     将fifo中的35s数据拷贝到20min的buffer
  Input:           fifo_head:fifo的链表头
                   polar_head:极化buffer的链表头
                   fifo_rbuffer:fifo的循环buffer
                   polar_rbuffer:极化信息的循环buffer
  Output:          NA
  Return:          NA
********************************************************/
void copy_fifo_buffer(struct list_head* fifo_head,
                           struct list_head* polar_head,
                           struct hisiap_ringbuffer_s *fifo_rbuffer,
                           struct hisiap_ringbuffer_s *polar_rbuffer)
{
    struct hisi_polar_device *di = g_polar_di;
    struct ploarized_info* ppolar_temp;
    struct ploarized_info* n;

    if (NULL == fifo_head || NULL == polar_head || NULL == di ||
        NULL == fifo_rbuffer || NULL == polar_rbuffer) {
        return;
    }
    if (list_empty(fifo_head))
        return;
    list_for_each_entry_safe_reverse(ppolar_temp, n, fifo_head, list) {
            fill_up_polar_fifo(di, ppolar_temp, polar_head, polar_rbuffer,
                               COUL_POLAR_SAMPLE_TIME);
    }
}
/*******************************************************
  Function:        sync_sample_info
  Description:     极化数据同步
  Input:           NA
  Output:          NA
  Return:          NA
********************************************************/
void sync_sample_info(void)
{
    struct hisi_polar_device *di = g_polar_di;
    unsigned long sample_time, flags, fifo_time_ms, sync_time;
    int fifo_depth, current_ua, i;
    int sync_num = 0;
    struct ploarized_info node;
    ktime_t kt;

    if (NULL == di)
        return;
#ifdef POLAR_INFO_DEBUG
    polar_info("[polar] %s ++ \n", __func__);
#endif
    spin_lock_irqsave(&di->coul_fifo_lock, flags);
    /*read polar info from the fifo*/
    fifo_depth = hisi_coul_battery_fifo_depth();
    sync_time = hisi_getcurtime();
    sync_time = sync_time / NSEC_PER_MSEC;
    sample_time = polar_get_head_time(di, &di->coul_fifo_head.list);
    if (time_after_eq(sync_time, sample_time + di->fifo_interval)) {
        sync_num = (int)((sync_time - sample_time) / di->fifo_interval);
        sync_num = clamp_val(sync_num, 0, fifo_depth);
        for (i = sync_num - 1; i >= 0; i--)
        {
            node.sample_time = sync_time - ((unsigned long)i*di->fifo_interval);//lint !e571
            current_ua = hisi_coul_battery_fifo_curr(i);
            node.current_ma = - (current_ua / UA_PER_MA);
            node.duration = di->fifo_interval;
            node.temperature =  hisi_battery_temperature();
            node.soc_now = hisi_coul_battery_ufcapacity_tenth();
            node.list.next = NULL;
            node.list.prev = NULL;
#ifdef POLAR_INFO_DEBUG
            polar_info("%s:time:%llu,curr:%d,duration:%llu,temp:%d:soc:%d\n",
                __func__, node.sample_time, node.current_ma, node.duration,
                node.temperature, node.soc_now);
#endif
            /*here we put the fifo info to 35S ringbuffer*/
            fill_up_polar_fifo(di, &node, &di->coul_fifo_head.list,
                                di->fifo_buffer, COUL_FIFO_SAMPLE_TIME);
        }
        /*modify the sample timer when we have synchronized the polar info*/
        fifo_time_ms = (unsigned long)fifo_depth * di->fifo_interval;//lint !e571
        kt = ktime_set(fifo_time_ms / MSEC_PER_SEC,
                        (fifo_time_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
        hrtimer_forward_now(&di->coul_sample_timer, kt);
    }
    /*copy 35s fifo buffer to 20min polar buffer*/
    copy_fifo_buffer(&di->coul_fifo_head.list, &di->polar_head.list,
                     di->fifo_buffer, di->polar_buffer);
    spin_unlock_irqrestore(&di->coul_fifo_lock, flags);
#ifdef POLAR_INFO_DEBUG
    polar_info("[polar] %s sync_num:%d--\n", __func__, sync_num);
#endif
}
EXPORT_SYMBOL(sync_sample_info);

void get_resume_polar_info(int sleep_cc, int sleep_time)
{
    struct hisi_polar_device *di = g_polar_di;
    struct ploarized_info node;
    unsigned long sample_time, flags;

    if (NULL == di || 0 >= sleep_time)
        return;
    spin_lock_irqsave(&di->coul_fifo_lock, flags);
    sample_time = hisi_getcurtime();
    node.sample_time = sample_time / NSEC_PER_MSEC;
    node.current_ma = CC_UAS2MA(sleep_cc, sleep_time);
    node.duration = sleep_time * MSEC_PER_SEC;
    node.temperature =  hisi_battery_temperature();
    node.soc_now = hisi_coul_battery_ufcapacity_tenth();
    node.list.next = NULL;
    node.list.prev = NULL;
#ifdef POLAR_INFO_DEBUG
    polar_info("%s:time:%llu,curr:%d,duration:%llu,temp:%d:soc:%d\n",
        __func__, node.sample_time, node.current_ma, node.duration, 
        node.temperature, node.soc_now);
#endif
    fill_up_polar_fifo(di, &node, &di->coul_fifo_head.list,
                       di->fifo_buffer, COUL_FIFO_SAMPLE_TIME);
    spin_unlock_irqrestore(&di->coul_fifo_lock, flags);
}
EXPORT_SYMBOL(get_resume_polar_info);

void start_polar_sample(void)
{
    struct hisi_polar_device *di = g_polar_di;
    ktime_t kt;
    unsigned long fifo_time_ms;
    int fifo_depth;

    if (NULL == di)
        return;
    fifo_depth = hisi_coul_battery_fifo_depth();
    fifo_time_ms = (unsigned long)fifo_depth * di->fifo_interval;//lint !e571
    kt = ktime_set(fifo_time_ms / MSEC_PER_SEC,
                    (fifo_time_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
    hrtimer_start(&di->coul_sample_timer, kt, HRTIMER_MODE_REL);
}
EXPORT_SYMBOL(start_polar_sample);

void stop_polar_sample(void)
{
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return;
    hrtimer_cancel(&di->coul_sample_timer);
}
EXPORT_SYMBOL(stop_polar_sample);

int polar_params_calculate(struct polar_calc_info* polar,
                                int ocv_soc_mv, int vol_now, int cur)
{
    int curr_future_5s = 0;
    int curr_future_peak = 0;
    int batt_soc_real, temp;
    long polar_future_5s = 0;
    static int polar_vol_index = 0;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di || NULL == polar)
        return -1;
    batt_soc_real =  hisi_coul_battery_ufcapacity_tenth();
    temp = hisi_battery_temperature();
    polar_info_calc(di, 0);
     /*get current polar vol*/
    polar->vol = get_polar_vol(di, batt_soc_real, temp, cur);
    /*pull 5s ahead to calculate future polar vol*/
    polar_info_calc(di, POLAR_CURR_PREDICT_MSECS);
    polar_future_5s = calculate_polar_volatge(batt_soc_real, temp, cur);
    update_polar_error_a(ocv_soc_mv, vol_now, polar->vol);
    /*calculate future max avg current*/
    curr_future_5s = get_estimate_max_avg_curr(ocv_soc_mv, vol_now,
                            batt_soc_real, polar->vol, polar_future_5s, temp,
                            di->v_cutoff, di->r_pcb);
    /*calculate future max peak current*/
    curr_future_peak = get_estimate_peak_curr(ocv_soc_mv, polar->vol,
                            batt_soc_real, temp, di->v_cutoff, di->r_pcb);
    if (curr_future_peak < curr_future_5s)
        curr_future_peak = curr_future_5s;
    polar->curr_5s = curr_future_5s;
    polar->curr_peak = curr_future_peak;
    polar->ocv = get_polar_ocv(batt_soc_real, cur, temp, vol_now);
    polar->ocv_old = ocv_soc_mv;
    polar->ori_vol = vol_now;
    polar->ori_cur = cur;
    polar->err_a = polar_err_a;
    update_polar_error_b(ocv_soc_mv, vol_now, polar->vol);
    /*store polar vol for next b value update*/
    polar_vol_array[polar_vol_index] = polar->vol;
    polar_vol_index++;
    polar_vol_index = polar_vol_index % POLAR_ARRAY_NUM;
    return 0;
}
EXPORT_SYMBOL(polar_params_calculate);

#ifdef POLAR_INFO_DEBUG
int test_polar_ocv_tbl_lookup(int soc, int batt_temp_degc)
{
    int ocv = 0;
    ocv = interpolate_polar_ocv(&polar_ocv_lut, batt_temp_degc, soc);
    return ocv;
}

int test_res_tbl_lookup(int soc, int batt_temp_degc, int curr)
{
    int res = 0;
    res = interpolate_two_dimension(&polar_res_lut,
            batt_temp_degc, soc, curr);
    return res;
}

int test_vector_res_tbl_lookup(int soc, int batt_temp_degc, int curr)
{
    int res_vector = 0;
    res_vector = get_polar_vector_res(&polar_res_lut,
            batt_temp_degc, soc, curr);
    return res_vector;
}

int test_vector_value_tbl_lookup(int soc, int batt_temp_degc, int curr)
{
    int polar_vector = 0;
    polar_vector = get_polar_vector_value(&polar_vector_lut,
            batt_temp_degc, soc, curr, 0);
    return polar_vector;
}

int test_vector_curr_lookup(int curr)
{
    return interpolate_curr_vector(polar_curr_vector_interval,
            POLAR_CURR_ARRAY_VECTOR_NUM, curr);
}
int test_nearest_lookup(int soc)
{
    return interpolate_nearest_x(polar_resistence_pc_points,
            POLAR_RES_PC_CURR_ROWS, soc);
}
#endif
/*******************************************************
  Function:       get_batt_phandle
  Description:    look for batt_phandle with id_voltage
  Input:
                  struct device *dev      ---- device pointer
                  int id_voltage          ---- battery id
                  const char * prop       ---- battery prop
  Output:         struct device_node*
  Return:         phandle of the battery
********************************************************/
static struct device_node *get_batt_phandle(struct device_node *np,
                    const char * prop, int p_num, u32 id_voltage)
{
    int i, ret;
    u32 id_identify_min = 0,id_identify_max = 0;
    const char *batt_brand;
    struct device_node *temp_node = NULL;

    if (NULL == np)
        return NULL;

    if (p_num < 0) {
        polar_info("[%s]get phandle list count failed", __func__);
        return NULL;
    }
    /*iter the phandle list*/
    for (i = 0; i < p_num; i++) {
        temp_node = of_parse_phandle(np, prop, i);
        ret = of_property_read_u32(temp_node, "id_voltage_min",
                &id_identify_min);
        if (ret) {
            polar_err("get id_identify_min failed\n");
            return NULL;
        }
        ret = of_property_read_u32(temp_node, "id_voltage_max",
                &id_identify_max);
        if (ret) {
            polar_err("get id_identify_max failed\n");
            return NULL;
        }
        polar_info("id_identify_min:%d,id_identify_max:%d\n",
                id_identify_min, id_identify_max);
        if ((id_voltage >= id_identify_min) && (id_voltage <= id_identify_max))
            break;
    }
    if (p_num == i) {
        polar_err("no battery modle matched\n");
        return NULL;
    }
    /*print the battery brand*/
    ret = of_property_read_string(temp_node, "batt_brand", &batt_brand);
    if (ret) {
        polar_err("get batt_brand failed\n");
        return NULL;
    }
    polar_info("batt_name:%s matched\n", batt_brand);
    return temp_node;
}

/*******************************************************
  Function:       get_polar_table_info
  Description:    look for batt_phandle with id_voltage
  Input:
  @bat_node:device node from which the property value is to be read
  @propname:name of the property to be searched
  @outvalues:pointer to return value, modified only if return value is 0
  @tbl_size:number of array elements to read
  Output:  outvalues
  Return:  Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough
********************************************************/
static int get_polar_table_info(struct device_node *bat_node,
                const char* propname, u32* outvalues, int tbl_size)
{
    int ele_count;
    int ret;
    /*get polar_ocv_table from dts*/
    ele_count = of_property_count_u32_elems(bat_node, propname);
    polar_info("%s:ele_cnt:%d\n", propname, ele_count);
    /*check if ele_count match with polar_ocv_table*/
    if (ele_count != tbl_size) {
        polar_err("ele_count:%d mismatch with %s\n", ele_count, propname);
        return -EINVAL;
    }
    ret = of_property_read_u32_array(bat_node, propname,
                    outvalues, ele_count);
    if (ret) {
            polar_err("get polar_ocv_table failed\n");
            return ret;
    }
    return 0;
}

/*******************************************************
  Function:       get_polar_dts_info
  Description:    look for dts info
  Input:
  @dev:device pointer
  Output:  NA
  Return:  Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough
********************************************************/
static int get_polar_dts_info(struct hisi_polar_device *di)
{
    int id_voltage = 0;
    int ret = 0;
    int batt_count = 0;
    struct device_node *bat_node;
    struct device_node *coul_node;
    if (NULL == di)
        return -EINVAL;
    coul_node = of_find_compatible_node(NULL, NULL, "hisi,coul_core");
    if (coul_node) {
        ret = of_property_read_u32(coul_node, "normal_cutoff_vol_mv",
                                   &di->v_cutoff);
        ret |= of_property_read_u32(coul_node, "r_pcb",&di->r_pcb);
    }
    if (!coul_node || ret) {
        di->v_cutoff = BATTERY_NORMAL_CUTOFF_VOL;
        di->r_pcb = DEFAULT_RPCB;
        polar_err("get coul info failed\n");
        return -1;
    }

    ret = of_property_read_u32(di->np, "fifo_interval", &di->fifo_interval);
    if (ret) {
        di->fifo_interval = COUL_DEFAULT_SAMPLE_INTERVAL;
        polar_err("get fifo_interval failed\n");
        return ret;
    }
    polar_info("fifo_interval:%d\n", di->fifo_interval);
    ret = of_property_read_u32(di->np, "fifo_depth", 
            &di->fifo_depth);
    if (ret) {
        polar_err("get fifo_depth failed\n");
        return ret;
    }
    polar_info("fifo_depth:%d\n", di->fifo_depth);
    ret = of_property_read_u32(di->np, "polar_batt_cnt", &batt_count);
    if (ret) {
        polar_err("get fifo_depth failed\n");
        return ret;
    }
    polar_info("polar_batt_cnt:%d\n", batt_count);
    id_voltage = hisi_battery_id_voltage();
    bat_node = get_batt_phandle(di->np, "polar_batt_name",
                    batt_count, (u32)id_voltage);
    if (NULL == bat_node) {
        polar_err("get polar_phandle failed\n");
        ret = -EINVAL;
        goto out;
    }
    polar_err("polar_bat_node:%s\n", bat_node->name);
    ret = get_polar_table_info(bat_node,
                    "polar_ocv_table", (u32 *)polar_ocv_lut.ocv,
                    (POLAR_OCV_PC_TEMP_ROWS * POLAR_OCV_PC_TEMP_COLS));
    if (ret)
        goto out;
    ret = get_polar_table_info(bat_node,
                    "polar_res_table", (u32 *)polar_res_lut.value,
                    (POLAR_OCV_PC_TEMP_COLS * POLAR_RES_PC_CURR_ROWS
                        * POLAR_CURR_ARRAY_NUM));
    if (ret)
        goto clr;
    ret = get_polar_table_info(bat_node,
                    "polar_vector_table", (u32 *)polar_vector_lut.value,
                    (POLAR_OCV_PC_TEMP_COLS * POLAR_RES_PC_CURR_ROWS
                        * POLAR_CURR_ARRAY_VECTOR_NUM * POLAR_VECTOR_SIZE));
    if (ret)
        goto clr;
    polar_info("%s:get polar dts info success\n", __func__);
    return 0;
clr:
    memset_s(polar_ocv_lut.ocv, sizeof(polar_ocv_lut.ocv),
            0, sizeof(polar_ocv_lut.ocv));
    memset_s(polar_res_lut.value, sizeof(polar_res_lut.value),
            0, sizeof(polar_res_lut.value));
    memset_s(polar_vector_lut.value, sizeof(polar_vector_lut.value),
            0, sizeof(polar_vector_lut.value));
out:
    return ret;
}

/*******************************************************
  Function:        polar_info_init
  Description:     极化相关数据初始化(根据电池容量初始化电流档位)
  Input:          struct smartstar_coul_device *di
  Output:           初始化后的极化档位数据
  Return:          NA
********************************************************/
static void polar_info_init(struct hisi_polar_device *di)
{
    int i, batt_fcc;
    int batt_present;
    int ret;
    batt_fcc = hisi_battery_fcc();
    batt_present = is_hisi_battery_exist();
    for (i = 1; i < POLAR_CURR_ARRAY_NUM; i++){
        polar_curr_interval[i] = polar_curr_interval[i]
                        *  batt_fcc / UA_PER_MA;
    }
    for (i = 1; i < POLAR_CURR_ARRAY_VECTOR_NUM; i++){
        polar_curr_vector_interval[i] = polar_curr_vector_interval[i]
                        * batt_fcc / UA_PER_MA;
    }
    for (i = 0; i < POLAR_CURR_ARRAY_NUM; i++){
        polar_info("%s:polar_curr_interval[%d],%d;vector_interval[%d],%d\n",
                        __func__, i, polar_curr_interval[i], i/2,
                        polar_curr_vector_interval[i/2]);
    }
    memset_s(polar_ocv_lut.ocv, sizeof(polar_ocv_lut.ocv),
            0, sizeof(polar_ocv_lut.ocv));
    memset_s(polar_res_lut.value, sizeof(polar_res_lut.value),
            0, sizeof(polar_res_lut.value));
    memset_s(polar_vector_lut.value, sizeof(polar_vector_lut.value),
            0, sizeof(polar_vector_lut.value));
    memset_s(polar_avg_curr_info, sizeof(polar_avg_curr_info),
            0, sizeof(polar_avg_curr_info));

    ret = get_polar_dts_info(di);
    if (ret)
        polar_err("get dts info failed\n");
}
//lint -esym(429, di)
static int hisi_coul_polar_probe(struct platform_device *pdev)
{
    struct device_node *node = pdev->dev.of_node;
    int retval = 0, fifo_depth;
    struct hisi_polar_device *di = NULL;
    ktime_t kt;
    unsigned long fifo_time_ms;

    di = devm_kzalloc(&pdev->dev, sizeof(struct hisi_polar_device),
                        GFP_KERNEL);
    if (!di)
        return -ENOMEM;
    di->dev =&pdev->dev;
    di->np = node;
    di->polar_buffer =
        (struct hisiap_ringbuffer_s*) devm_kzalloc(&pdev->dev,
                                        POLAR_BUFFER_SIZE,
                                        GFP_KERNEL);
    di->fifo_buffer =
        (struct hisiap_ringbuffer_s*) devm_kzalloc(&pdev->dev,
                                        FIFO_BUFFER_SIZE,
                                        GFP_KERNEL);
    if (!di->polar_buffer || !di->fifo_buffer) {
        polar_err("failed to alloc polar_buffer struct\n");
        return -ENOMEM;
    } else
        polar_info("polar_buffer alloc ok:%pK", di->polar_buffer);
    retval = hisiap_ringbuffer_init(di->polar_buffer,
                POLAR_BUFFER_SIZE, sizeof(struct ploarized_info),
                "coul_polar");
    retval |= hisiap_ringbuffer_init(di->fifo_buffer,
                FIFO_BUFFER_SIZE, sizeof(struct ploarized_info),
                "coul_fifo");
    if (retval) {
        polar_err("%s failed to init polar ring buffer!!!\n", __FUNCTION__);
        goto out;
    }
    INIT_LIST_HEAD(&di->polar_head.list);
    INIT_LIST_HEAD(&di->coul_fifo_head.list);
    spin_lock_init(&di->coul_fifo_lock);
    polar_info_init(di);
    if ((di->polar_buffer->max_num * di->fifo_interval)
            <= COUL_POLAR_SAMPLE_TIME){
        polar_err("buffer is not enough for sample:max_node:%u,fifo_time:%d",
            di->polar_buffer->max_num, di->fifo_interval);
        retval = -ENOMEM;
        goto out;
    }
    if ((di->fifo_buffer->max_num * di->fifo_interval)
            <= COUL_FIFO_SAMPLE_TIME){
        polar_err("buffer is not enough for sample:max_node:%u,fifo_time:%d",
            di->fifo_buffer->max_num, di->fifo_interval);
        retval = -ENOMEM;
        goto out;
    }
    fifo_depth = hisi_coul_battery_fifo_depth();
    fifo_time_ms = (unsigned long)fifo_depth * di->fifo_interval;//lint !e571
    kt = ktime_set(fifo_time_ms / MSEC_PER_SEC,
                    (fifo_time_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
    hrtimer_init(&di->coul_sample_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    di->coul_sample_timer.function = sample_timer_func;
    hrtimer_start(&di->coul_sample_timer, kt, HRTIMER_MODE_REL);
    platform_set_drvdata(pdev, di);
    g_polar_di = di;
    polar_info("Hisi coul polar ready\n");
    return 0;
out:
    g_polar_di = NULL;
    return retval;//lint !e593
}
//lint +esym(429, di)
static int hisi_coul_polar_remove(struct platform_device *pdev)
{
    struct hisi_polar_device *di = platform_get_drvdata(pdev);

    hrtimer_cancel(&di->coul_sample_timer);
    di = NULL;
    g_polar_di = NULL;
    platform_set_drvdata(pdev, NULL);
    return 0;
}

static const struct of_device_id hisi_coul_polar_of_match[] = {
    {
    .compatible = "hisi,coul_polar",
    .data = NULL
    },
    {},
};

MODULE_DEVICE_TABLE(of, hisi_coul_polar_of_match);

static struct platform_driver hisi_coul_polar_driver = {
    .probe = hisi_coul_polar_probe,
    .driver = {
               .name = "coul_polar",
               .owner = THIS_MODULE,
               .of_match_table = of_match_ptr(hisi_coul_polar_of_match),
        },
     .remove = hisi_coul_polar_remove,
};

static int __init hisi_coul_polar_init(void)
{
    platform_driver_register(&hisi_coul_polar_driver);
    return 0;
}

fs_initcall(hisi_coul_polar_init);

static void __exit hisi_coul_polar_exit(void)
{
    platform_driver_unregister(&hisi_coul_polar_driver);
}

module_exit(hisi_coul_polar_exit);

MODULE_DESCRIPTION("COUL POLARIZATION driver");
MODULE_LICENSE("GPL V2");
