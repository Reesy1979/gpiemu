///////////////////////////////////////////////////////////////////////////////
//	Bilinear scaling using the IPU
///////////////////////////////////////////////////////////////////////////////

#ifndef _IPU_H_
#define _IPU_H_

// IPU_REG_BASE
#define IPU_BASE    0xB3080000
#define IPU_P_BASE  0x13080000
#define IPU__OFFSET 0x13080000
#define IPU__SIZE   0x00001000

struct ipu_module
{
    unsigned int reg_ctrl;      	// 0x00
    unsigned int reg_status;    	// 0x04
    unsigned int reg_d_fmt;     	// 0x08
    unsigned int reg_y_addr;    	// 0x0c
    unsigned int reg_u_addr;    	// 0x10
    unsigned int reg_v_addr;    	// 0x14
    unsigned int reg_in_fm_gs;  	// 0x18
    unsigned int reg_y_stride;  	// 0x1c
    unsigned int reg_uv_stride; 	// 0x20
    unsigned int reg_out_addr;  	// 0x24
    unsigned int reg_out_gs;    	// 0x28
    unsigned int reg_out_stride;	// 0x2c
    unsigned int rsz_coef_index;	// 0x30
    unsigned int reg_csc_c0_coef;   // 0x34
    unsigned int reg_csc_c1_coef;   // 0x38
    unsigned int reg_csc_c2_coef;   // 0x3c
    unsigned int reg_csc_c3_coef;   // 0x40
    unsigned int reg_csc_c4_coef;   // 0x44
    unsigned int hrsz_coef_lut[20]; // 0x48
    unsigned int vrsz_coef_lut[20]; // 0x98
};

typedef struct
{
   unsigned int coef;
   unsigned short int in_n;
   unsigned short int out_n;
} rsz_lut;

struct Ration2m
{
    float ratio;
    int n, m;
};


// Register offset
#define  IPU_CTRL           (IPU_BASE+0x0)
#define  IPU_STATUS         (IPU_BASE+0x4)
#define  IPU_D_FMT          (IPU_BASE+0x8)
#define  IPU_Y_ADDR         (IPU_BASE+0xc)
#define  IPU_U_ADDR         (IPU_BASE+0x10)
#define  IPU_V_ADDR         (IPU_BASE+0x14)
#define  IPU_IN_FM_GS       (IPU_BASE+0x18)
#define  IPU_Y_STRIDE       (IPU_BASE+0x1c)
#define  IPU_UV_STRIDE      (IPU_BASE+0x20)
#define  IPU_OUT_ADDR       (IPU_BASE+0x24)
#define  IPU_OUT_GS         (IPU_BASE+0x28)
#define  IPU_OUT_STRIDE     (IPU_BASE+0x2c)
#define  IPU_RSZ_COEF_INDEX (IPU_BASE+0x30)
#define  IPU_CSC_C0_COEF    (IPU_BASE+0x34)
#define  IPU_CSC_C1_COEF    (IPU_BASE+0x38)
#define  IPU_CSC_C2_COEF    (IPU_BASE+0x3c)
#define  IPU_CSC_C3_COEF    (IPU_BASE+0x40)
#define  IPU_CSC_C4_COEF    (IPU_BASE+0x44)
#define  IPU_HRSZ_LUT_BASE  (IPU_BASE+0x48)
#define  IPU_VRSZ_LUT_BASE  (IPU_BASE+0x98)

#define  REG_IPU_CTRL       	REG32(IPU_CTRL)
#define  REG_IPU_STATUS         REG32(IPU_STATUS)
#define  REG_IPU_D_FMT          REG32(IPU_D_FMT)
#define  REG_IPU_Y_ADDR         REG32(IPU_Y_ADDR)
#define  REG_IPU_U_ADDR         REG32(IPU_U_ADDR)
#define  REG_IPU_V_ADDR         REG32(IPU_V_ADDR)
#define  REG_IPU_IN_FM_GS       REG32(IPU_IN_FM_GS)
#define  REG_IPU_Y_STRIDE       REG32(IPU_Y_STRIDE)
#define  REG_IPU_UV_STRIDE      REG32(IPU_UV_STRIDE)
#define  REG_IPU_OUT_ADDR       REG32(IPU_OUT_ADDR)
#define  REG_IPU_OUT_GS         REG32(IPU_OUT_GS)
#define  REG_IPU_OUT_STRIDE     REG32(IPU_OUT_STRIDE)
#define  REG_IPU_RSZ_COEF_INDEX REG32(IPU_RSZ_COEF_INDEX)
#define  REG_IPU_CSC_C0_COEF    REG32(IPU_CSC_C0_COEF)
#define  REG_IPU_CSC_C1_COEF    REG32(IPU_CSC_C1_COEF)
#define  REG_IPU_CSC_C2_COEF    REG32(IPU_CSC_C2_COEF)
#define  REG_IPU_CSC_C3_COEF    REG32(IPU_CSC_C3_COEF)
#define  REG_IPU_CSC_C4_COEF    REG32(IPU_CSC_C4_COEF)
#define  REG_IPU_HRSZ_LUT_BASE  REG32(IPU_HRSZ_LUT_BASE)
#define  REG_IPU_VRSZ_LUT_BASE  REG32(IPU_VRSZ_LUT_BASE)

// REG_CTRL field define
#define IPU_EN          (1 << 0)
#define RSZ_EN          (1 << 1)
#define FM_IRQ_EN       (1 << 2)
#define IPU_RESET       (1 << 3)
#define H_UP_SCALE      (1 << 8)
#define V_UP_SCALE      (1 << 9)
#define H_SCALE_SHIFT   (8)
#define V_SCALE_SHIFT   (9)

// REG_IPU_STATUS field define
#define OUT_END         (1 << 0)

// REG_D_FMT field define
#define INFMT_YUV420    (0 << 0)
#define INFMT_YUV422    (1 << 0)
#define INFMT_YUV444    (2 << 0)
#define INFMT_YUV411    (3 << 0)
#define INFMT_YCbCr420  (4 << 0)
#define INFMT_YCbCr422  (5 << 0)
#define INFMT_YCbCr444  (6 << 0)
#define INFMT_YCbCr411  (7 << 0)

#define OUTFMT_RGB555   (0 << 16)
#define OUTFMT_RGB565   (1 << 16)
#define OUTFMT_RGB888   (2 << 16)

// REG_IN_FM_GS field define
#define IN_FM_W(val)    ((val) << 16)
#define IN_FM_H(val)    ((val) << 0)

// REG_IN_FM_GS field define
#define OUT_FM_W(val)   ((val) << 16)
#define OUT_FM_H(val)   ((val) << 0)

// REG_UV_STRIDE field define
#define U_STRIDE(val)   ((val) << 16)
#define V_STRIDE(val)   ((val) << 0)


#define VE_IDX_SFT		0
#define HE_IDX_SFT      16

// RSZ_LUT_FIELD
#define OUT_N_SFT       0
#define OUT_N_MSK       0x1
#define IN_N_SFT        1
#define IN_N_MSK        0x1
#define W_COEF_SFT      2
#define W_COEF_MSK      0xFF

// function about REG_IPU_CTRL
#define stop_ipu() \
REG_IPU_CTRL &= ~IPU_EN;

#define run_ipu() \
REG_IPU_CTRL |= IPU_EN;

#define reset_ipu() \
REG_IPU_CTRL |= IPU_RESET;

#define disable_irq_ipu() \
REG_IPU_CTRL &= ~FM_IRQ_EN;

#define disable_rsize_ipu() \
REG_IPU_CTRL &= ~RSZ_EN;

#define enable_rsize_ipu() \
REG_IPU_CTRL |= RSZ_EN;

#define is_enable_ipu() \
  (REG_IPU_CTRL & IPU_EN)

// function about REG_IPU_STATUS
#define clear_end_flag_ipu() \
REG_IPU_STATUS &= ~OUT_END;

#define polling_end_flag_ipu() \
 (REG_IPU_STATUS & OUT_END)

// parameter
// R = 1.164 * (Y - 16) + 1.596 * (cr - 128)    {C0, C1}
// G = 1.164 * (Y - 16) - 0.392 * (cb -128) - 0.813 * (cr - 128)  {C0, C2, C3}
// B = 1.164 * (Y - 16) + 2.017 * (cb - 128)    {C0, C4}

//#if 1
#define YCbCr_CSC_C0 0x4A8        /* 1.164 * 1024 */
#define YCbCr_CSC_C1 0x662        /* 1.596 * 1024 */
#define YCbCr_CSC_C2 0x191        /* 0.392 * 1024 */
#define YCbCr_CSC_C3 0x341        /* 0.813 * 1024 */
#define YCbCr_CSC_C4 0x811        /* 2.017 * 1024 */
//#else
#define YUV_CSC_C0 0x400
#define YUV_CSC_C1 0x59C
#define YUV_CSC_C2 0x161
#define YUV_CSC_C3 0x2DC
#define YUV_CSC_C4 0x718
//#endif

void ipu_init(int src_width,  int src_height,
              int dst_width,  int dst_height, 
			  int dst_stride, int dst_width_bytes, int dst_height_bytes);
			  
void ipu_blit(uint8_t *source_y, uint8_t *source_u, uint8_t *source_v, uint16_t *dest);

#endif
