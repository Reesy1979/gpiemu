///////////////////////////////////////////////////////////////////////////////
//	Bilinear scaling using the IPU
///////////////////////////////////////////////////////////////////////////////

#include <dingoo/jz4740.h>
#include "ipu.scaler.h"

#define IPU_LUT_LEN	20
#define CFG_EXTAL	12000000

///////////////////////////////////////////////////////////////////////////////
struct Ration2m ipu_ratio_table[IPU_LUT_LEN * IPU_LUT_LEN];
int				ipu_rtable_len;
rsz_lut			h_lut[IPU_LUT_LEN];
rsz_lut			v_lut[IPU_LUT_LEN];
int 			Hsel;
int 			Wsel;
int 			Height_lut_max;
int 			Width_lut_max;

///////////////////////////////////////////////////////////////////////////////
static void		init_ipu_ratio_table();
static int		find_ipu_ratio_factor(float ratio);
static int		resize_out_cal(int insize, int outsize, int srcN, int dstM);
static int		resize_lut_cal(int srcN, int dstM, rsz_lut lut[]);

///////////////////////////////////////////////////////////////////////////////
//	Stop the unit
///////////////////////////////////////////////////////////////////////////////
void ipu_stop()
{
    if(is_enable_ipu())
    {
        while(!polling_end_flag_ipu());
    }
    stop_ipu();
    clear_end_flag_ipu();
    reset_ipu();
    //__cpm_stop_ipu();
}

///////////////////////////////////////////////////////////////////////////////
//	Prepare the unit
///////////////////////////////////////////////////////////////////////////////
void ipu_init(int src_width,  int src_height,
              int dst_width,  int dst_height,
              int dst_stride, int dst_width_bytes, int dst_height_bytes)
{
    int i;
    int srcN;
    int dstM;

    //__cpm_start_ipu();

    init_ipu_ratio_table();

    stop_ipu();
    reset_ipu();
    clear_end_flag_ipu();
    enable_rsize_ipu();
    disable_irq_ipu();

    REG_IPU_CTRL |= H_UP_SCALE | V_UP_SCALE;
    REG_IPU_D_FMT = INFMT_YUV444 | OUTFMT_RGB565;

    Hsel = find_ipu_ratio_factor(((float) src_width ) / (float) dst_width );
    Wsel = find_ipu_ratio_factor(((float) src_height) / (float) dst_height);

    Width_lut_max  = ipu_ratio_table[Wsel].m;
    Height_lut_max = ipu_ratio_table[Hsel].m;

    srcN = ipu_ratio_table[Wsel].n;
    dstM = ipu_ratio_table[Wsel].m;
    resize_out_cal(src_width, dst_width, srcN, dstM);
    resize_lut_cal(srcN, dstM, v_lut);

    srcN = ipu_ratio_table[Hsel].n;
    dstM = ipu_ratio_table[Hsel].m;
    resize_out_cal(src_height, dst_height, srcN, dstM);
    resize_lut_cal(srcN, dstM, h_lut);

    Wsel = ipu_ratio_table[Wsel].m;
    Hsel = ipu_ratio_table[Hsel].m;

    REG_IPU_CSC_C0_COEF = YUV_CSC_C0;
    REG_IPU_CSC_C1_COEF = YUV_CSC_C1;
    REG_IPU_CSC_C2_COEF = YUV_CSC_C2;
    REG_IPU_CSC_C3_COEF = YUV_CSC_C3;
    REG_IPU_CSC_C4_COEF = YUV_CSC_C4;

    for (i = 0; i < Width_lut_max; i++)
    {
        REG32(IPU_VRSZ_LUT_BASE + i * 4) = (v_lut[i].coef << W_COEF_SFT) |
                                           (v_lut[i].in_n << IN_N_SFT) |
                                           OUT_N_MSK;
    }

    for (i = 0; i < Height_lut_max; i++)
    {
        REG32(IPU_HRSZ_LUT_BASE + i * 4) = (h_lut[i].coef << W_COEF_SFT) |
                                           (h_lut[i].in_n << IN_N_SFT) |
                                           OUT_N_MSK;
    }

    REG_IPU_OUT_GS = OUT_FM_W(dst_width_bytes) | OUT_FM_H(dst_height_bytes);
    REG_IPU_OUT_STRIDE = dst_stride;

    REG_IPU_RSZ_COEF_INDEX = ((Wsel - 1) << VE_IDX_SFT) |
                             ((Hsel - 1) << HE_IDX_SFT);

    REG_IPU_IN_FM_GS = IN_FM_W(src_width) | IN_FM_H(src_height);
    REG_IPU_Y_STRIDE = src_width;
    REG_IPU_UV_STRIDE = U_STRIDE(src_width) | V_STRIDE(src_width);
}

//	Init IPU ratios table	///////////////////////////////////////////////////
static void init_ipu_ratio_table()
{
    int i;
    int j;
    int cnt;
    
    
    float diff;
    
    // orig table, first calculate
    for (i = 1; i <= IPU_LUT_LEN; i++)
    {
        for (j = 1; j <= IPU_LUT_LEN; j++)
        {
            ipu_ratio_table [(i - 1) * IPU_LUT_LEN + j - 1].ratio = i / (float)j;
            ipu_ratio_table [(i - 1) * IPU_LUT_LEN + j - 1].n = i;
            ipu_ratio_table [(i - 1) * IPU_LUT_LEN + j - 1].m = j;
        }
    }
    
    // Eliminate the ratio greater than 1:2
    for (i = 0; i < (IPU_LUT_LEN) * (IPU_LUT_LEN); i++)
    {
        if (ipu_ratio_table[i].ratio < 0.4999f)
        {
            ipu_ratio_table[i].n = ipu_ratio_table[i].m = -1;
        }
    }

    // eliminate the same ratio
    for (i = 0; i < (IPU_LUT_LEN) * (IPU_LUT_LEN); i++)
    {
        for (j = i + 1; j < (IPU_LUT_LEN) * (IPU_LUT_LEN); j++)
        {
            diff = ipu_ratio_table[i].ratio - ipu_ratio_table[j].ratio;
            if (diff > -0.001f && diff < 0.001f)
            {
                ipu_ratio_table[j].n = -1;
                ipu_ratio_table[j].m = -1;
            }
        }
    }

    // reorder ipu_ratio_table
    cnt = 0;
    for (i = 0; i < (IPU_LUT_LEN) * (IPU_LUT_LEN); i++)
    {
        if (ipu_ratio_table[i].n != -1)
        {
            if (cnt != i)
            {
                ipu_ratio_table[cnt] = ipu_ratio_table[i];
            }
            cnt++;
        }
    }
    ipu_rtable_len = cnt;
}

//	Find a ratio factor in the ratios table	///////////////////////////////////
static int find_ipu_ratio_factor(float ratio)
{
    int i, sel; 
    float diff, min = 0;
    sel = ipu_rtable_len;
    
    for (i = 0; i < ipu_rtable_len; i++)
    {
        if (ratio > ipu_ratio_table[i].ratio)
        {
            diff = ratio - ipu_ratio_table[i].ratio;
        }
        else
        {
            diff = ipu_ratio_table[i].ratio - ratio;
        }
        if (i == 0 || diff < min)
        {
            min = diff;
            sel = i;
        }
    }
    return sel;
}

///////////////////////////////////////////////////////////////////////////////
static int resize_out_cal(int insize, int outsize, int srcN, int dstM)
{
    int calsize, delta;
    float tmp, tmp2;
    delta = 1;
    
    do
    {
        tmp = (insize - delta) * dstM / (float)srcN;
        tmp2 = tmp  * srcN / dstM;
        if (tmp2 == insize - delta)
        {
            calsize = (int) (tmp + 1);
        }
        else
        {
            calsize = (int) (tmp + 2);
        }
        delta++;
    } while (calsize > outsize);
    
    return calsize;
}

///////////////////////////////////////////////////////////////////////////////
static int resize_lut_cal(int srcN, int dstM, rsz_lut lut[])
{
    int i, t;
    float w_coef;
    float factor;
    float factor2;

    for (i = 0, t = 0; i < dstM; i++)
    {
        factor = (float) (i * srcN) / (float) dstM;
        factor2 = factor - (int) factor;
        w_coef = 1.0f - factor2;
        lut[i].coef = (unsigned int) (128.0f * w_coef);
        lut[i].out_n = 1;
        if (t <= factor)
        {
            lut[i].in_n = 1;
            t++;
        }
        else
        {
            lut[i].in_n = 0;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//	Blit a scaled surface onto the screen
///////////////////////////////////////////////////////////////////////////////
void ipu_blit(uint8_t *source_y, uint8_t *source_u, uint8_t *source_v, uint16_t *dest)
{
    stop_ipu();
    clear_end_flag_ipu();
    REG_IPU_OUT_ADDR = (uint32_t) dest     & 0x1fffffff;
    REG_IPU_Y_ADDR   = (uint32_t) source_y & 0x1fffffff;
    REG_IPU_U_ADDR   = (uint32_t) source_u & 0x1fffffff;
    REG_IPU_V_ADDR   = (uint32_t) source_v & 0x1fffffff;
    run_ipu();
}
