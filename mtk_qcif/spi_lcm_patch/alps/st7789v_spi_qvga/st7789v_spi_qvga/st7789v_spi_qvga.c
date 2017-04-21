#ifdef BUILD_LK
#include <stdio.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#else
#include <mt-plat/mt_gpio.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define FRAME_WIDTH  										(240)
#define FRAME_HEIGHT 										(320)
#define LCM_DSI_CMD_MODE 0

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define SET_CHIP_SELECT(v)    (lcm_util.set_chip_select((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define  GPIO_LCM_RST 	(91 | 0x80000000)

#ifdef MT_SPIA
#define  GPIO_SPI_CS 	(65 | 0x80000000)
#define  GPIO_SPI_CK 	(66 | 0x80000000)
#define  GPIO_SPI_DC 	(67 | 0x80000000)
#define  GPIO_SPI_SDA 	(68 | 0x80000000)
#else
#define  GPIO_SPI_CS 	(5 | 0x80000000)
#define  GPIO_SPI_CK 	(6 | 0x80000000)
#define  GPIO_SPI_DC 	(3 | 0x80000000)
#define  GPIO_SPI_SDA 	(4 | 0x80000000)
#endif

extern void mt_spi_write_reg(unsigned char cmd, unsigned char *data, unsigned char len);
extern void mt_spi_read_reg(unsigned char cmd, unsigned char *reg, unsigned char len);

static void lcm_spi_init(void)  {
	mt_set_gpio_mode(GPIO_SPI_CS, 0);
	mt_set_gpio_mode(GPIO_SPI_CK, 0);
	mt_set_gpio_mode(GPIO_SPI_DC, 0);
	mt_set_gpio_mode(GPIO_SPI_SDA, 0);
	mt_set_gpio_mode(GPIO_LCM_RST, 0);

	mt_set_gpio_dir(GPIO_SPI_CS, 1);
	mt_set_gpio_dir(GPIO_SPI_CK, 1);
	mt_set_gpio_dir(GPIO_SPI_DC, 1);
	mt_set_gpio_dir(GPIO_SPI_SDA, 1);
	mt_set_gpio_dir(GPIO_LCM_RST, 1);

	mt_set_gpio_out(GPIO_SPI_CS, 1);
	mt_set_gpio_out(GPIO_SPI_CK, 0);
	mt_set_gpio_out(GPIO_SPI_SDA, 0);
	mt_set_gpio_out(GPIO_SPI_DC, 0);
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util){
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;
	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	params->dsi.LANE_NUM				= LCM_ONE_LANE;
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	params->dsi.packet_size=256;
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 20;
	params->dsi.vertical_frontporch					= 20;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 
	params->dsi.horizontal_sync_active				= 10;
	params->dsi.horizontal_backporch				= 80;
	params->dsi.horizontal_frontporch				= 80;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.customization_esd_check_enable = 1;	
	params->dsi.lcm_esd_check_table[0].cmd	= 0xDC;
	params->dsi.lcm_esd_check_table[0].count	= 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x52;
	params->dsi.pll_div1 = 1;
	params->dsi.pll_div2 = 1;
	params->dsi.PLL_CLOCK = 104;
}

static void lcm_init(void)
{
	unsigned char data[32];
	
	lcm_spi_init();

	mt_set_gpio_out(GPIO_LCM_RST, 1);
	MDELAY(2);
	mt_set_gpio_out(GPIO_LCM_RST, 0);
	MDELAY(10);
	mt_set_gpio_out(GPIO_LCM_RST, 1);
	MDELAY(120);

	mt_spi_write_reg(0x11, data,0);
	MDELAY(120);
	
	data[0] = 0x0c;
	data[1] = 0x0c;
	data[2] = 0x00;
	data[3] = 0x33;
	data[4] = 0x33;
	mt_spi_write_reg(0xb2, data,5);

	data[0] = 0x35;
	mt_spi_write_reg(0xb7, data,1);
	
//---------------------------------ST7789S Power setting--------------------------------------//
	data[0] = 0x2b;
	mt_spi_write_reg(0xbb, data,1);
	
	data[0] = 0x2c;
	mt_spi_write_reg(0xc0, data,1);

	data[0] = 0x01;
	mt_spi_write_reg(0xc2, data,1);

	data[0] = 0x17;
	mt_spi_write_reg(0xc3, data,1);

	data[0] = 0x20;
	mt_spi_write_reg(0xc4, data,1);
///////////
	//data[0] = 0x03;
	//mt_spi_write_reg(0xc5, data,1);
///////////////////
	data[0] = 0x1E;
	mt_spi_write_reg(0xc6, data,1);

	data[0] = 0x0f;
	mt_spi_write_reg(0xca, data,1);

	data[0] = 0x08;
	mt_spi_write_reg(0xc8, data,1);

	data[0] = 0x90;
	mt_spi_write_reg(0x55, data,1);

	data[0] = 0xa4;
	data[1] = 0xa1;
	mt_spi_write_reg(0xd0, data,2);

	data[0] = 0xf0;
	data[1] = 0x00;
	data[2] = 0x0a;
	data[3] = 0x10;
	data[4] = 0x12;
	data[5] = 0x1b;
	data[6] = 0x39;
	data[7] = 0x44;
	data[8] = 0x47;
	data[9] = 0x28;
	data[10] = 0x12;
	data[11] = 0x10;
	data[12] = 0x16;
	data[13] = 0x1b;
	mt_spi_write_reg(0xe0, data,14);

	data[0] = 0xf0;
	data[1] = 0x00;
	data[2] = 0x0a;
	data[3] = 0x10;
	data[4] = 0x11;
	data[5] = 0x1a;
	data[6] = 0x3b;
	data[7] = 0x34;
	data[8] = 0x4e;
	data[9] = 0x3a;
	data[10] = 0x17;
	data[11] = 0x16;
	data[12] = 0x21;
	data[13] = 0x22;
	mt_spi_write_reg(0xe1, data,14);
	
	data[0] = 0x00;
	mt_spi_write_reg(0x35, data,0x01);
	data[0] = 0x00;
	data[1] = 0x60;
	mt_spi_write_reg(0x44, data,0x02);
	
	data[0] = 0x00;
	mt_spi_write_reg(0x36, data,1);
	
	data[0] = 0x55;
	mt_spi_write_reg(0x3a, data,1);
	mt_spi_write_reg(0x29, data,0);
}

static void lcm_suspend(void){
	unsigned char data[32];

	data[0] = 0x00;
	mt_spi_write_reg(0x28, data,0);
	MDELAY(20);	
	data[0] = 0x00;
	mt_spi_write_reg(0x10, data,0);
	MDELAY(20);	
}

static void lcm_resume(void){
	unsigned char data[32];

	data[0] = 0x00;
	mt_spi_write_reg(0x11, data,0);
	MDELAY(20);	
	data[0] = 0x00;
	mt_spi_write_reg(0x29, data,0);
	MDELAY(20);	
}

static unsigned int lcm_esd_check(void){
	unsigned char data[8];
	
	mt_spi_read_reg(0x04, data, 4);
	//dprintf(0, "lcm id 0x04: %x %x %x %x\r\n", data[0], data[1], data[2], data[3]);
	if((data[1] == 0x85) && (data[2] == 0x52)){
		return 0;
	}
	return 1;
}

static unsigned int lcm_esd_recover(void){
	lcm_init();
	return 1;
}

static unsigned int lcm_compare_id(void)
{
	unsigned char data[16];
	
	lcm_spi_init();
	mt_set_gpio_out(GPIO_LCM_RST, 1);
	MDELAY(2);
	mt_set_gpio_out(GPIO_LCM_RST, 0);
	MDELAY(10);
	mt_set_gpio_out(GPIO_LCM_RST, 1);
	MDELAY(100);

	mt_spi_read_reg(0x04, data, 4);
	dprintf(0, "lcm id 0x04: %x %x %x %x\r\n", data[0], data[1], data[2], data[3]);
	if((data[1] == 0x85) && (data[2] == 0x52)){
		return 1;
	}

	return 0;
}

LCM_DRIVER st7789v_spi_qvga_lcm_drv = 
{
	.name = "st7789v_spi_qvga_lcm",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
};

