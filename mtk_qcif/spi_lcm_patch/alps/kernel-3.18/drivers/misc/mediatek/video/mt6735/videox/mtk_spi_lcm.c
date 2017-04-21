#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/spi/spidev.h>
#include <linux/spi/spi.h>
#include <linux/power_supply.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <mt_spi.h>
#include "disp_drv_platform.h"

/*spi device name*/
#define 	SPI_DEVICE_NAME		"mt_spi_lcm"
#define	MT_SPI_CLASS_NAME	"spi_lcd"

#define	MTK_SPI_ALIGN_MASK_NUM  			10
#define 	MTK_SPI_ALIGN_MASK  				((0x1 << MTK_SPI_ALIGN_MASK_NUM) - 1)
#define 	MTK_SPI_LCM_FRAME_BUFFER_MAX  	3

#define 	CONFIG_HAS_EARLYSUSPEND  		0
//#define 	MT_SPI_LCM_UPDATE_COLOR
//#define MT_SPI_PIN_SYS

#ifdef CONFIG_OF
struct pinctrl *mt_spi_ctrl = NULL;
struct pinctrl_state *mt_spi_cs_ctrl_h = NULL;
struct pinctrl_state *mt_spi_cs_ctrl_l = NULL;
struct pinctrl_state *mt_spi_cs_ctrl_m = NULL;
struct pinctrl_state *mt_spi_clk_ctrl_h = NULL;
struct pinctrl_state *mt_spi_clk_ctrl_l = NULL;
struct pinctrl_state *mt_spi_clk_ctrl_m = NULL;
struct pinctrl_state *mt_spi_miso_ctrl_h = NULL;
struct pinctrl_state *mt_spi_miso_ctrl_l = NULL;
struct pinctrl_state *mt_spi_miso_ctrl_m = NULL;
struct pinctrl_state *mt_spi_mosi_ctrl_h = NULL;
struct pinctrl_state *mt_spi_mosi_ctrl_l = NULL;
struct pinctrl_state *mt_spi_mosi_ctrl_m = NULL;
#endif

struct mt_spi_pin_config {
	u32  spi_cs_pin;
	u32  spi_cs_mode;
	u32  spi_clk_pin;
	u32  spi_clk_mode;
	u32  spi_sda_pin;
	u32  spi_sda_mode;
	u32  spi_dc_pin;
	u32  spi_dc_mode;
};

struct mt_spi_lcm_dev {
	struct spi_device	*spi;
	struct mt_spi_pin_config spi_pin_cfg;
	struct work_struct     spi_work;
#ifdef MT_SPI_LCM_UPDATE_COLOR
	struct timer_list   mt_lcm_timer;
#endif
	struct mutex buf_lock;
	struct mutex pin_lock;
	spinlock_t spi_lock;
	unsigned long long tag_time;
	unsigned int spi_clock;
	unsigned int spi_hz;	
	u32  lcm_cmd_mode;
	u32  lcm_port_line;
	u32  lcm_frame_width;
	u32  lcm_frame_height;
	u32  lcm_frame_buffer_size;
	u32  lcm_frame_active_size;
	u32  lcm_frame_transfer_size;
	u32  lcm_frame_buffer_id;
	u8	*lcm_frame_buffer[MTK_SPI_LCM_FRAME_BUFFER_MAX];		
#if CONFIG_HAS_EARLYSUSPEND
	struct early_suspend    early_fp;
#endif
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static DECLARE_WAIT_QUEUE_HEAD(custom_screen_update_wq);

static atomic_t custom_screen_update_flag = ATOMIC_INIT(0);
static struct task_struct *custom_screen_update_wq_task = NULL;
static struct mt_spi_lcm_dev *mt_lcm=NULL;
static struct cdev mtSpiLcmDevice;
static struct class *mtSpiLcmClass;
static struct platform_driver *mt_spi_pdev = NULL;
static dev_t mtSpiLcmDeviceNo;

struct mt_chip_conf lcm_spi_conf={
	.setuptime=2,
	.holdtime=2,
	.high_time=2,
	.low_time=2,
	.cs_idletime=10,
	.ulthgh_thrsh=0,
	.cpol=0,
	.cpha=0,
	.rx_mlsb=1,
	.tx_mlsb=1,
	.tx_endian=1,
	.rx_endian=1,
	.com_mod=1,
	.pause=0,
	.deassert=0,
	.ulthigh=0,
	.tckdly=0,
};

static struct spi_board_info spi_board_devs[] __initdata = {
	[0] = {
    		.modalias=MT_SPI_CLASS_NAME,
		.bus_num = 0,
		.chip_select=0,
		.mode = SPI_MODE_0,
		.controller_data=&lcm_spi_conf,
	},
};

extern int primary_display_get_width(void);
extern int primary_display_get_height(void);
extern void _disp_custom_lcm_vsync(void);
extern void _disp_custom_lcm_check_esd(void);
extern unsigned int _disp_custom_lcm_get_line(void);
extern unsigned int _disp_custom_lcm_get_clock_h(void);
extern unsigned int _disp_custom_lcm_get_clock_l(void);

extern int mt_set_gpio_pull_enable(unsigned long pin, unsigned long enable);
extern int mt_set_gpio_pull_select(unsigned long pin, unsigned long select);
extern int mt_set_gpio_mode(unsigned long pin, unsigned long mode);
extern int mt_get_gpio_mode(unsigned long pin);
extern int mt_set_gpio_dir(unsigned long pin, unsigned long dir);
extern int mt_set_gpio_out(unsigned long pin, unsigned long output);
extern int mt_get_gpio_in(unsigned long pin);

static void mt_lcm_spi_pins_prepare(int mode){
#ifdef MT_SPI_PIN_SYS
	if(mode){
		if(mt_spi_ctrl && mt_spi_cs_ctrl_m)
			pinctrl_select_state(mt_spi_ctrl, mt_spi_cs_ctrl_m);
		if(mt_spi_ctrl && mt_spi_clk_ctrl_m)
			pinctrl_select_state(mt_spi_ctrl, mt_spi_clk_ctrl_m);
		if(mt_spi_ctrl && mt_spi_miso_ctrl_l)
			pinctrl_select_state(mt_spi_ctrl, mt_spi_miso_ctrl_l);
		if(mt_spi_ctrl && mt_spi_mosi_ctrl_m)
			pinctrl_select_state(mt_spi_ctrl, mt_spi_mosi_ctrl_m);
	}
	else{
		if(mt_spi_ctrl && mt_spi_cs_ctrl_l)
			pinctrl_select_state(mt_spi_ctrl, mt_spi_cs_ctrl_l);
		if(mt_spi_ctrl && mt_spi_clk_ctrl_l)
			pinctrl_select_state(mt_spi_ctrl, mt_spi_clk_ctrl_l);
		if(mt_spi_ctrl && mt_spi_miso_ctrl_l)
			pinctrl_select_state(mt_spi_ctrl, mt_spi_miso_ctrl_l);
		if(mt_spi_ctrl && mt_spi_mosi_ctrl_l)
			pinctrl_select_state(mt_spi_ctrl, mt_spi_mosi_ctrl_l);
	}
#else
	struct mt_spi_lcm_dev	*spi_dev = mt_lcm;
	if(spi_dev == NULL){
		return;
	}
	if(mode){
		/*cs*/
		if(mt_get_gpio_mode(spi_dev->spi_pin_cfg.spi_cs_pin) != spi_dev->spi_pin_cfg.spi_cs_mode){
			mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_cs_pin, spi_dev->spi_pin_cfg.spi_cs_mode);
		}
		
		/*sck*/
		if(mt_get_gpio_mode(spi_dev->spi_pin_cfg.spi_clk_pin) != spi_dev->spi_pin_cfg.spi_clk_mode){
			mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_clk_pin, spi_dev->spi_pin_cfg.spi_clk_mode);
		}
		
		/*sda*/
		if(mt_get_gpio_mode(spi_dev->spi_pin_cfg.spi_sda_pin) != spi_dev->spi_pin_cfg.spi_sda_mode){
			mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_sda_pin, spi_dev->spi_pin_cfg.spi_sda_mode);
		}
		
		/*miso*/
		if(mt_get_gpio_mode(spi_dev->spi_pin_cfg.spi_dc_pin) != spi_dev->spi_pin_cfg.spi_dc_mode){
			mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_dc_pin, spi_dev->spi_pin_cfg.spi_dc_mode);
		}
	}
	else{
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 0);
		mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_cs_pin, 0);
		mt_set_gpio_dir(spi_dev->spi_pin_cfg.spi_cs_pin, 1);

		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
		mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
		mt_set_gpio_dir(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
		
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);
		mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_sda_pin, 0);
		mt_set_gpio_dir(spi_dev->spi_pin_cfg.spi_sda_pin, 1);
		
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_dc_pin, 0);
		mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_dc_pin, 0);
		mt_set_gpio_dir(spi_dev->spi_pin_cfg.spi_dc_pin, 1);
	}
#endif
}

static void mt_lcm_spi_set_dc(int on){
#ifdef MT_SPI_PIN_SYS
	if(mt_spi_ctrl && mt_spi_miso_ctrl_l){
		if(on){
			pinctrl_select_state(mt_spi_ctrl, mt_spi_miso_ctrl_h);
		}
		else{
			pinctrl_select_state(mt_spi_ctrl, mt_spi_miso_ctrl_l);
		}
	}
#else
	struct mt_spi_lcm_dev	*spi_dev = mt_lcm;
	if(spi_dev == NULL){
		return;
	}
	
	if(mt_get_gpio_mode(spi_dev->spi_pin_cfg.spi_dc_pin) != spi_dev->spi_pin_cfg.spi_dc_mode){
		mt_set_gpio_mode(spi_dev->spi_pin_cfg.spi_dc_pin, spi_dev->spi_pin_cfg.spi_dc_mode);
	}
	mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_dc_pin, on);
#endif
}

static void mt_lcm_spi_setup(struct mt_spi_lcm_dev *spi_dev, int bits, int max_speed_hz){
	spi_dev->spi->mode = SPI_MODE_0;
	spi_dev->spi->max_speed_hz = max_speed_hz; 
	spi_dev->spi->bits_per_word = bits;
	spi_dev->spi->controller_data =(void*)&lcm_spi_conf;
	spi_setup(spi_dev->spi);
}

static int mt_spi_write_byte(struct mt_spi_lcm_dev *spi_dev, u8 *tx_buf, u32 len){
	struct spi_message msg;
	struct spi_transfer *xfer;
	u8 *reminder_buf = NULL;
	int ret = 0;

	reminder_buf = kzalloc(len, GFP_KERNEL);
	if(reminder_buf == NULL ) {
		pr_warn("mt_lcm:No memory for exter data.\n");
		return -ENOMEM;
	}
	memcpy(reminder_buf, tx_buf, len);

	xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
	if( xfer == NULL){
		pr_warn("mt_lcm:No memory for command.\n");
		if(reminder_buf != NULL)
			kfree(reminder_buf);
		return -ENOMEM;
	}

	spi_message_init(&msg);
	xfer[0].tx_buf = reminder_buf;
	xfer[0].len = len ;
	spi_message_add_tail(&xfer[0], &msg);

	ret = spi_sync(spi_dev->spi, &msg);
	if(ret == 0) {
		ret = msg.actual_length;
	} else 	{
		printk("mt_lcm:write async failed. ret = %d\n", ret);
	}

	if(xfer != NULL) {
		kfree(xfer);
		xfer = NULL;
	}
	if(reminder_buf != NULL) {
		kfree(reminder_buf);
		reminder_buf = NULL;
	}

	return ret;
}

void mt_spi_read_reg(u8 cmd, u8 *reg, u16 len){
	struct mt_spi_lcm_dev	*spi_dev = mt_lcm;
	int i,j,offset = 0x80;
	u8 data = 0;
	
	if(spi_dev == NULL){
		return;
	}
	
	mutex_lock(&(spi_dev->pin_lock));
	if(spi_dev->lcm_port_line == 2){
		mt_lcm_spi_pins_prepare(0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);	
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
		offset = 0x80;
		for(i=0; i<8; i++){  
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
			if(cmd&offset)
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 1);	
			else
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);	
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
			offset = offset>>1;
		} 

		mt_set_gpio_dir(spi_dev->spi_pin_cfg.spi_sda_pin, 0);
		for(j=0; j<len; j++){
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 1);	
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
			data = 0;
			for (i=0; i<8; i++) {  
				mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
				mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
				data = data | mt_get_gpio_in(spi_dev->spi_pin_cfg.spi_sda_pin);
				data = (data <<1);
			}  
			reg[j] = data;
		}

		mt_set_gpio_dir(spi_dev->spi_pin_cfg.spi_sda_pin, 1);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 1);
	}
	else{
		mt_lcm_spi_pins_prepare(0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_dc_pin, 0);
		offset = 0x80;
		for(i=0; i<8; i++){  
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
			if(cmd&offset) mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 1);	
			else mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);	
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
			offset = offset>>1;
		} 

		mt_set_gpio_dir(spi_dev->spi_pin_cfg.spi_sda_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_dc_pin, 1);
		if(len>2){
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
		}
		for(j=0; j<len; j++){
			data = 0;
			for (i=0; i<8; i++) {  
				mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
				data = (data <<1);
				mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
				data = data | mt_get_gpio_in(spi_dev->spi_pin_cfg.spi_sda_pin);
			}  
			reg[j] = data;
		}
		
		mt_set_gpio_dir(spi_dev->spi_pin_cfg.spi_sda_pin, 1);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 1);
	}		
	mutex_unlock(&(spi_dev->pin_lock));
}

void mt_spi_write_reg(u8 cmd, u8 *reg, u16 len){
	struct mt_spi_lcm_dev	*spi_dev = mt_lcm;
	if(spi_dev == NULL){
		return;
	}
	mutex_lock(&(spi_dev->pin_lock));
	if(spi_dev->lcm_port_line == 2){
		int i,j,offset = 0x80;
		u8 data = 0;
	
		mt_lcm_spi_pins_prepare(0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);	
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
		offset = 0x80;
		for(i=0; i<8; i++){  
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
			if(cmd&offset) mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 1);	
			else mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);	
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
			offset = offset>>1;
		} 

		for(j=0; j<len; j++){
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 1);	
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
			data = reg[j];
			offset = 0x80;
			for (i=0; i<8; i++) {  
				mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
				if(data&offset) mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 1);	
				else mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);	
				mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
				offset = offset>>1;
			}  
		}
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
		mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 1);
	}
	else{
		if(spi_dev->lcm_cmd_mode){
			mt_lcm_spi_pins_prepare(1);
			mt_lcm_spi_set_dc(0);
			mt_spi_write_byte(spi_dev, &cmd, 1);
			mt_lcm_spi_set_dc(1);
			if(len>0){
				mt_spi_write_byte(spi_dev, reg, len);
			}
		}
		else{
			int i,j,offset = 0x80;
			u8 data = 0;
		
			mt_lcm_spi_pins_prepare(0);
			mt_lcm_spi_set_dc(0);
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 0);
			offset = 0x80;
			for(i=0; i<8; i++){  
				mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
				if(cmd&offset) mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 1);	
				else mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);	
				mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
				offset = offset>>1;
			} 

			mt_lcm_spi_set_dc(1);
			for(j=0; j<len; j++){
				offset = 0x80;
				data = reg[j];
				for (i=0; i<8; i++) {  
					mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
					if(data&offset) mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 1);	
					else mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_sda_pin, 0);	
					mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
					offset = offset>>1;
				}  
			}
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_clk_pin, 0);
			mt_set_gpio_out(spi_dev->spi_pin_cfg.spi_cs_pin, 1);
		}
	}
	mutex_unlock(&(spi_dev->pin_lock));
}

void mt_lcm_spi_set_blocksize(u16 x, u16 y, u16 w, u16 h){
	u8 data[32];
	
	w = w-1;
	h = h-1;
	data[0] = x>>8;
	data[1] = x&0xFF;
	data[2] = w>>8;
	data[3] = w&0xFF;
	mt_spi_write_reg(0x2a, data, 4);
	data[0] = y>>8;
	data[1] = y&0xFF;
	data[2] = h>>8;
	data[3] = h&0xFF;
	mt_spi_write_reg(0x2b, data, 4);
	mt_spi_write_reg(0x2c, data, 0);	
}

static int _disp_update_screen(void){
	struct mt_spi_lcm_dev	*spi_dev = mt_lcm;
	struct spi_message msg;
	struct spi_transfer *xfer;
	u32  package_num ;
	u32  reminder ;
	u32 tx_len;
	u8 *reminder_buf = NULL;
	u8 *screen_buf = NULL;
	u8   twice = 0;
	int ret = 0;
	
	if(spi_dev == NULL){
		return -1;
	}

	tx_len = spi_dev->lcm_frame_transfer_size;
	package_num = (tx_len)>>MTK_SPI_ALIGN_MASK_NUM;
	reminder = (tx_len) & MTK_SPI_ALIGN_MASK;
	
	mutex_lock(&(spi_dev->pin_lock));
	mt_lcm_spi_pins_prepare(1);
	if(spi_dev->lcm_port_line != 2){
		mt_lcm_spi_set_dc(1);
	}
	
	screen_buf = spi_dev->lcm_frame_buffer[spi_dev->lcm_frame_buffer_id];
	if((package_num > 0) && (reminder != 0)) {
		twice = 1;
		reminder_buf = screen_buf+tx_len - reminder;
		xfer = kzalloc(sizeof(*xfer)*2, GFP_KERNEL);
	} else {
		twice = 0;
		xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
	}
	if( xfer == NULL){
		if(reminder_buf != NULL)
			kfree(reminder_buf);
		ret =  -ENOMEM;
		goto exit;
	}

	spi_message_init(&msg);
	if(twice == 1) {
		xfer[0].tx_buf = screen_buf;
		xfer[0].len = package_num << MTK_SPI_ALIGN_MASK_NUM;
		spi_message_add_tail(&xfer[0], &msg);
		xfer[1].tx_buf = reminder_buf;
		xfer[1].len = reminder;
		spi_message_add_tail(&xfer[1], &msg);
	} else {
		xfer[0].tx_buf = screen_buf;
		xfer[0].len = tx_len ;
		spi_message_add_tail(&xfer[0], &msg);
	}
	ret = spi_sync(spi_dev->spi, &msg);
	if(ret == 0) {
		if(twice == 1)
			ret = msg.actual_length;
		else
			ret = msg.actual_length;
	} else 	{
		printk("mt_lcm:write async failed. ret = %d\n", ret);
	}

	if(xfer != NULL) {
		kfree(xfer);
		xfer = NULL;
	}

exit:	
	mutex_unlock(&(spi_dev->pin_lock));
	
	return ret;
}

void _display_custom_screen_prepare_mode0(u8 *src, u8 *dst, u32 size){
__asm__ __volatile__(
	"	mov	r3, %2			\n\t"
	"	mvn	r4, #0x0000FF00	\n\t"
	
	"	ldr	r5, [%0],#4		\n\t"
	"	ldr	r6, [%0],#4		\n\t"
	"1:	ldr	r7, [%0],#4		\n\t"
	"	ldr	r8, [%0],#4		\n\t"
	
	"	eor	r9, r5, r5,ror 	#16	\n\t"
	"	and	r9, r4, r9,lsr 	#8	\n\t"
	"	eor  	r5, r9, r5,ror 	#8	\n\t"
	"	ror  	r5, r5,		#16	\n\t"
	"	str	r5, [%1],#4		\n\t"
	
	"	eor  	r9, r6, r6,ror 	#16	\n\t"
	"	and	r9, r4, r9,lsr 	#8	\n\t"
	"	eor	r6, r9, r6,ror 	#8	\n\t"
	"	ror  	r6, r6,		#16	\n\t"
	"	str	r6, [%1],#4		\n\t"

	"	ldr	r5, [%0],#4		\n\t"
	"	eor  	r9, r7, r7,ror 	#16	\n\t"
	"	and  r9, r4, r9,lsr 	#8	\n\t"
	"	eor  	r7, r9, r7,ror 	#8	\n\t"
	"	ror  	r7, r7,		#16	\n\t"
	"	str	r7, [%1],#4		\n\t"

	"	ldr	r6, [%0],#4		\n\t"
	"	eor  r9, r8, r8,ror 	#16	\n\t"
	"	and	r9, r4, r9,lsr 	#8	\n\t"
	"	eor	r8, r9, r8,ror 	#8	\n\t"
	"	ror  	r8, r8,		#16	\n\t"
	"	str	r8, [%1],#4		\n\t"
	
	"	ldr	r7, [%0],#4		\n\t"
	"	eor	r9, r5, r5,ror 	#16	\n\t"
	"	and	r9, r4, r9,lsr 	#8	\n\t"
	"	eor  	r5, r9, r5,ror 	#8	\n\t"
	"	ror  	r5, r5,		#16	\n\t"
	"	str	r5, [%1],#4		\n\t"

	"	ldr	r8, [%0],#4		\n\t"
	"	eor  	r9, r6, r6,ror 	#16	\n\t"
	"	and	r9, r4, r9,lsr 	#8	\n\t"
	"	eor	r6, r9, r6,ror 	#8	\n\t"
	"	ror  	r6, r6,		#16	\n\t"
	"	str	r6, [%1],#4		\n\t"
	
	"	ldr	r5, [%0],#4		\n\t"
	"	eor  	r9, r7, r7,ror 	#16	\n\t"
	"	and  r9, r4, r9,lsr 	#8	\n\t"
	"	eor  	r7, r9, r7,ror 	#8	\n\t"
	"	ror  	r7, r7,		#16	\n\t"
	"	str	r7, [%1],#4		\n\t"

	"	ldr	r6, [%0],#4		\n\t"
	"	eor  r9, r8, r8,ror 	#16	\n\t"
	"	and	r9, r4, r9,lsr 	#8	\n\t"
	"	eor	r8, r9, r8,ror 	#8	\n\t"
	"	ror  	r8, r8,		#16	\n\t"
	"	str	r8, [%1],#4		\n\t"
	
	"	subs	r3, r3, #32		\n\t"
	"	bne	1b"
	
	:"+r"(src),"+r"(dst)
	:"r"(size)
	: "r3", "r4", "r5", "r6", "r7", "r8", "r9");
}

void _display_custom_screen_prepare_mode1(u8 *src, u8 *dst, u32 size){

}

void _display_custom_screen_update_wakeup(u8 *screen, u16 w, u16 h){
	struct mt_spi_lcm_dev	*spi_dev = mt_lcm;	

	if(spi_dev == NULL){
		return ;
	}

	mutex_lock(&spi_dev->buf_lock);
	spi_dev->lcm_frame_buffer_id++;
       spi_dev->lcm_frame_buffer_id %= MTK_SPI_LCM_FRAME_BUFFER_MAX;
	mutex_unlock(&spi_dev->buf_lock);
	
	if(spi_dev->lcm_port_line == 2){
		_display_custom_screen_prepare_mode1(screen, spi_dev->lcm_frame_buffer[spi_dev->lcm_frame_buffer_id], spi_dev->lcm_frame_active_size);
	}
	else{
		_display_custom_screen_prepare_mode0(screen, spi_dev->lcm_frame_buffer[spi_dev->lcm_frame_buffer_id], spi_dev->lcm_frame_active_size);
	}
	
#ifndef MT_SPI_LCM_UPDATE_COLOR
	_disp_custom_lcm_check_esd();
	mt_lcm_spi_set_blocksize(0,0,spi_dev->lcm_frame_width,spi_dev->lcm_frame_height);
	atomic_set(&custom_screen_update_flag, 1);
	wake_up(&custom_screen_update_wq);
#endif
	_disp_custom_lcm_vsync();
}

static int _disp_primary_screen_update_thread(void* data){
	int ret=0;
	
	while(1){
		ret = wait_event_interruptible(custom_screen_update_wq, (atomic_read(&custom_screen_update_flag) != 0));
		atomic_set(&custom_screen_update_flag, 0);
		_disp_update_screen();
		if (kthread_should_stop()){
			break;
		}
	}

	return ret;
}

#ifdef MT_SPI_LCM_UPDATE_COLOR
static void spi_lcm_debug_timer_work(struct work_struct *work){	
	struct mt_spi_lcm_dev	*spi_dev = mt_lcm;
	static int color;
	
	if(spi_dev == NULL){
		return ;
	}
	
	mutex_lock(&spi_dev->buf_lock);
	spi_dev->lcm_frame_buffer_id++;
	spi_dev->lcm_frame_buffer_id %= MTK_SPI_LCM_FRAME_BUFFER_MAX;
	mutex_unlock(&spi_dev->buf_lock);

	memset( spi_dev->lcm_frame_buffer[spi_dev->lcm_frame_buffer_id], color++, spi_dev->lcm_frame_active_size);
	//mt_lcm_spi_set_blocksize(0,0,spi_dev->lcm_frame_width,spi_dev->lcm_frame_height);
	atomic_set(&custom_screen_update_flag, 1);
	wake_up(&custom_screen_update_wq);
	
	mod_timer(&spi_dev->mt_lcm_timer, jiffies + 1*HZ/50);
}

static void spi_lcm_debug_timer_func(unsigned long arg){
	struct mt_spi_lcm_dev	*spi_dev = mt_lcm;
	
	schedule_work(&spi_dev->spi_work);	
}
#endif

static int spi_lcm_open(struct inode *pInode, struct file *pFile){

	return 0;
}


static int spi_lcm_release(struct inode *pInode, struct file *pFile){

	return 0;
}

static long spi_lcm_ioctl(struct file *pFile, unsigned int code, unsigned long param){

	return 0;
}

#ifdef CONFIG_COMPAT
static long spi_lcm_ioctl_compat(struct file *pFile, unsigned int code, unsigned long param){
	return 0;
}
#endif

static const struct file_operations mtSpiLcmOP = {
	.owner = THIS_MODULE,
	.open = spi_lcm_open,
	.release = spi_lcm_release,
	.unlocked_ioctl = spi_lcm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = spi_lcm_ioctl_compat,
#endif
};

#if CONFIG_HAS_EARLYSUSPEND
static void mt_lcm_early_suspend(struct early_suspend *h){    
	struct mt_spi_lcm_dev *mt_spi_lcm_dev = container_of(h, struct mt_spi_lcm_dev, early_fp);

}

static void mt_lcm_late_resume(struct early_suspend *h){    
	struct mt_spi_lcm_dev *mt_spi_lcm_dev = container_of(h, struct mt_spi_lcm_dev, early_fp);	

}
#endif

static int  mt_lcm_probe(struct spi_device *spi){
	struct mt_spi_lcm_dev	*spi_dev;
	int status, i;

	spi_dev = kzalloc(sizeof(struct mt_spi_lcm_dev), GFP_KERNEL);
	if (!spi_dev){
		pr_warn("Failed to alloc memory for mt_lcm device.\n");
		return -ENOMEM;
	}
	memset(spi_dev, 0, sizeof(struct mt_spi_lcm_dev));
	
	spi_dev->lcm_cmd_mode = 1;
	spi_dev->spi = spi;

	//CONFIG
	spi_dev->spi_pin_cfg.spi_cs_pin = 0x80000005;
	spi_dev->spi_pin_cfg.spi_cs_mode = 3;
	spi_dev->spi_pin_cfg.spi_clk_pin = 0x80000006;
	spi_dev->spi_pin_cfg.spi_clk_mode = 3;
	spi_dev->spi_pin_cfg.spi_sda_pin = 0x80000004;
	spi_dev->spi_pin_cfg.spi_sda_mode = 3;_cfg.spi_sda_pin, 1);
//	mt_set_gpio_pull_select(spi_dev->spi_pin_cfg.spi_sda_pin, 1);
	//mt_set_gpio_pull_enable(spi_dev->spi_pin_cfg.spi_dc_pin, 1);
	//mt_set_gpio_pull_select(spi_dev->spi_pin_cfg.spi_dc_pin, 1);
		
	lcm_spi_conf.high_time = _disp_cus
	spi_dev->spi_pin_cfg.spi_dc_pin = 0x80000003;
	spi_dev->spi_pin_cfg.spi_dc_mode = 0;

	mt_set_gpio_pull_enable(spi_dev->spi_pin_cfg.spi_cs_pin, 1);
	mt_set_gpio_pull_select(spi_dev->spi_pin_cfg.spi_cs_pin, 1);
	mt_set_gpio_pull_enable(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
	mt_set_gpio_pull_select(spi_dev->spi_pin_cfg.spi_clk_pin, 1);
//	mt_set_gpio_pull_enable(spi_dev->spi_pintom_lcm_get_clock_h();
	lcm_spi_conf.low_time = _disp_custom_lcm_get_clock_l();
	lcm_spi_conf.setuptime = 1;
	lcm_spi_conf.holdtime = 4;
	lcm_spi_conf.cs_idletime = 2;
	lcm_spi_conf.ulthgh_thrsh = 0;
	lcm_spi_conf.cpol = 0;
	lcm_spi_conf.cpha = 0;
	lcm_spi_conf.rx_mlsb = 1;
	lcm_spi_conf.tx_mlsb = 1;
	lcm_spi_conf.tx_endian = 0;
	lcm_spi_conf.rx_endian = 0;
	lcm_spi_conf.com_mod = 1;
	lcm_spi_conf.pause = 0;
	lcm_spi_conf.finish_intr = 1;
	lcm_spi_conf.deassert = 0;
	lcm_spi_conf.ulthigh = 0;
	lcm_spi_conf.tckdly = 0;
	
	if(lcm_spi_conf.high_time<1){
		lcm_spi_conf.high_time = 1;
	}
	if(lcm_spi_conf.low_time<1){
		lcm_spi_conf.low_time = 1;
	}
	
	spin_lock_init(&spi_dev->spi_lock);
	mutex_init(&spi_dev->buf_lock);
	mutex_init(&spi_dev->pin_lock);

	spi_dev->lcm_frame_width = (u32)primary_display_get_width();
	spi_dev->lcm_frame_height = (u32)primary_display_get_height();
	spi_dev->lcm_frame_buffer_size = ALIGN_TO(spi_dev->lcm_frame_width,32)*ALIGN_TO(spi_dev->lcm_frame_height,32)*2;
	spi_dev->lcm_frame_active_size = spi_dev->lcm_frame_width*spi_dev->lcm_frame_height*2;
	spi_dev->lcm_frame_transfer_size = spi_dev->lcm_frame_active_size;
	
	spi_dev->lcm_port_line = _disp_custom_lcm_get_line();
	if(spi_dev->lcm_port_line == 2){
		spi_dev->lcm_frame_buffer_size = spi_dev->lcm_frame_buffer_size * 2;
		spi_dev->lcm_frame_transfer_size= spi_dev->lcm_frame_width*spi_dev->lcm_frame_height*2*9/8;
		spi_dev->lcm_frame_transfer_size = (spi_dev->lcm_frame_transfer_size/(1 << MTK_SPI_ALIGN_MASK_NUM) +1)*(1 << MTK_SPI_ALIGN_MASK_NUM);
	}
	spi_dev->lcm_frame_buffer_id = 0;
	for(i=0; i<MTK_SPI_LCM_FRAME_BUFFER_MAX; i++){
		spi_dev->lcm_frame_buffer[i] = kzalloc(spi_dev->lcm_frame_buffer_size, GFP_KERNEL);
		if(spi_dev->lcm_frame_buffer[i] == NULL) {
			kfree(spi_dev);
			goto err;
		}
		memset(spi_dev->lcm_frame_buffer[i], 0, spi_dev->lcm_frame_buffer_size);
	}
	
	//mt_lcm_spi_pins_prepare(1);
	spi_set_drvdata(spi, spi_dev);
	mt_lcm_spi_setup(spi_dev, 8, spi_dev->spi_hz);

	mt_lcm = spi_dev;

#if CONFIG_HAS_EARLYSUSPEND		
	spi_dev->early_fp.level		= EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	spi_dev->early_fp.suspend	= mt_lcm_early_suspend,		
	spi_dev->early_fp.resume		= mt_lcm_late_resume,    	
	register_early_suspend(&spi_dev->early_fp);
#endif
	
	init_waitqueue_head(&custom_screen_update_wq);
	custom_screen_update_wq_task = kthread_create(_disp_primary_screen_update_thread,NULL,"spi_screen_update");
	wake_up_process(custom_screen_update_wq_task);
	
#ifdef MT_SPI_LCM_UPDATE_COLOR
	INIT_WORK(&spi_dev->spi_work, spi_lcm_debug_timer_work);
	init_timer(&spi_dev->mt_lcm_timer); 
	spi_dev->mt_lcm_timer.function = spi_lcm_debug_timer_func;
	spi_dev->mt_lcm_timer.expires = jiffies + 1*HZ;
	add_timer(&spi_dev->mt_lcm_timer);  
#endif

	return 0;
err:
        cdev_del(&mtSpiLcmDevice);
        unregister_chrdev_region(mtSpiLcmDeviceNo, 1);
	
	return status;
}

static int  mt_lcm_remove(struct spi_device *spi){
    struct mt_spi_lcm_dev	*spi_dev = spi_get_drvdata(spi);

    spin_lock_irq(&spi_dev->spi_lock);
    spi_dev->spi = NULL;
    spi_set_drvdata(spi, NULL);
    spin_unlock_irq(&spi_dev->spi_lock);

    mutex_lock(&device_list_lock);
    device_destroy(mtSpiLcmClass, mtSpiLcmDeviceNo);
    mutex_unlock(&device_list_lock);

    return 0;
}

static struct spi_driver spi_lcm_driver = {
	.probe =	mt_lcm_probe,
	.remove =	mt_lcm_remove,
	.driver = {
		.name = MT_SPI_CLASS_NAME,
		.owner =	THIS_MODULE,
	},
};

#ifdef CONFIG_OF
static int mt_spi_gpio_init(struct platform_device *pdev){
	static int init=1;	
	int ret = 0;

	if(init){
		mt_spi_ctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR(mt_spi_ctrl)) {
			printk("Cannot find mt_spi_ctrl pinctrl!");
			ret = PTR_ERR(mt_spi_ctrl);
		}

		mt_spi_cs_ctrl_h = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_cs_ctrl_h");
		if (IS_ERR(mt_spi_cs_ctrl_h)) {
			ret = PTR_ERR(mt_spi_cs_ctrl_h);
			printk("%s : pinctrl err, mt_spi_cs_ctrl_h\n", __func__);
			mt_spi_cs_ctrl_h = NULL;
		}
		else{
			printk("%s : pinctrl ok, mt_spi_cs_ctrl_h\n", __func__);
		}
		
		mt_spi_cs_ctrl_l = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_cs_ctrl_l");
		if (IS_ERR(mt_spi_cs_ctrl_l)) {
			ret = PTR_ERR(mt_spi_cs_ctrl_l);
			printk("%s : pinctrl err, mt_spi_cs_ctrl_l\n", __func__);		
			mt_spi_cs_ctrl_l = NULL;
		}
		else{
			printk("%s : pinctrl ok, mt_spi_cs_ctrl_l\n", __func__);
		}
		mt_spi_cs_ctrl_m = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_cs_ctrl_m");
		if (IS_ERR(mt_spi_cs_ctrl_m)) {
			ret = PTR_ERR(mt_spi_cs_ctrl_m);
			printk("%s : pinctrl err, mt_spi_cs_ctrl_m\n", __func__);
			mt_spi_cs_ctrl_m = NULL;
		}
		else{
			printk("%s : pinctrl ok, mt_spi_cs_ctrl_m\n", __func__);
		}
		
		mt_spi_clk_ctrl_h = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_clk_ctrl_h");
		if (IS_ERR(mt_spi_clk_ctrl_h)) {
			ret = PTR_ERR(mt_spi_clk_ctrl_h);
			printk("%s : pinctrl err, mt_spi_clk_ctrl_h\n", __func__);
			mt_spi_clk_ctrl_h = NULL;
		}
		else{
			printk("%s : pinctrl ok, mt_spi_clk_ctrl_h\n", __func__);
		}
		mt_spi_clk_ctrl_l = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_clk_ctrl_l");
		if (IS_ERR(mt_spi_clk_ctrl_l)) {
			ret = PTR_ERR(mt_spi_clk_ctrl_l);
			printk("%s : pinctrl err, mt_spi_clk_ctrl_l\n", __func__);
			mt_spi_clk_ctrl_l = NULL;
		}
		else{
			printk("%s : pinctrl ok, mt_spi_clk_ctrl_l\n", __func__);
		}
		mt_spi_clk_ctrl_m = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_clk_ctrl_m");
		if (IS_ERR(mt_spi_clk_ctrl_m)) {
			ret = PTR_ERR(mt_spi_clk_ctrl_m);
			printk("%s : pinctrl err, mt_spi_clk_ctrl_m\n", __func__);
			mt_spi_clk_ctrl_m = NULL;
		}
		else{
			printk("%s : pinctrl ok, mt_spi_clk_ctrl_m\n", __func__);
		}

		mt_spi_miso_ctrl_h = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_miso_ctrl_h");
		if (IS_ERR(mt_spi_miso_ctrl_h)) {
			ret = PTR_ERR(mt_spi_miso_ctrl_h);
			mt_spi_miso_ctrl_h = NULL;
			printk("%s : pinctrl err, mt_spi_miso_ctrl_h\n", __func__);
		}
		else{
			printk("%s : pinctrl ok, mt_spi_miso_ctrl_h\n", __func__);
		}
		mt_spi_miso_ctrl_l = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_miso_ctrl_l");
		if (IS_ERR(mt_spi_miso_ctrl_l)) {
			ret = PTR_ERR(mt_spi_miso_ctrl_l);
			mt_spi_miso_ctrl_l = NULL;
			printk("%s : pinctrl err, mt_spi_miso_ctrl_l\n", __func__);
		}
		else{
			printk("%s : pinctrl ok, mt_spi_miso_ctrl_l\n", __func__);
		}
		mt_spi_miso_ctrl_m = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_miso_ctrl_m");
		if (IS_ERR(mt_spi_miso_ctrl_m)) {
			ret = PTR_ERR(mt_spi_miso_ctrl_m);
			mt_spi_miso_ctrl_m = NULL;
			printk("%s : pinctrl err, mt_spi_miso_ctrl_m\n", __func__);
		}
		else{
			printk("%s : pinctrl ok, mt_spi_miso_ctrl_m\n", __func__);
		}

		mt_spi_mosi_ctrl_h = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_mosi_ctrl_h");
		if (IS_ERR(mt_spi_mosi_ctrl_h)) {
			ret = PTR_ERR(mt_spi_mosi_ctrl_h);
			mt_spi_mosi_ctrl_h = NULL;
			printk("%s : pinctrl err, mt_spi_mosi_ctrl_h\n", __func__);
		}
		else{
			printk("%s : pinctrl ok, mt_spi_mosi_ctrl_h\n", __func__);
		}
		mt_spi_mosi_ctrl_l = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_mosi_ctrl_l");
		if (IS_ERR(mt_spi_mosi_ctrl_l)) {
			ret = PTR_ERR(mt_spi_mosi_ctrl_l);
			mt_spi_mosi_ctrl_l = NULL;
			printk("%s : pinctrl err, mt_spi_mosi_ctrl_l\n", __func__);
		}
		else{
			printk("%s : pinctrl ok, mt_spi_mosi_ctrl_l\n", __func__);
		}
		mt_spi_mosi_ctrl_m = pinctrl_lookup_state(mt_spi_ctrl, "mt_spi_mosi_ctrl_m");
		if (IS_ERR(mt_spi_mosi_ctrl_m)) {
			ret = PTR_ERR(mt_spi_mosi_ctrl_m);
			mt_spi_mosi_ctrl_m = NULL;
			printk("%s : pinctrl err, mt_spi_mosi_ctrl_m\n", __func__);
		}
		else{
			printk("%s : pinctrl ok, mt_spi_mosi_ctrl_m\n", __func__);
		}
	}
	
	return 0;
}
#endif

static int mt_spi_dev_probe(struct platform_device *pdev){
	struct device *dev = NULL;
	int status;

	status = alloc_chrdev_region(&mtSpiLcmDeviceNo, 0, 1, SPI_DEVICE_NAME);
	cdev_init(&mtSpiLcmDevice, &mtSpiLcmOP);
	mtSpiLcmDevice.owner = THIS_MODULE;
	status = cdev_add(&mtSpiLcmDevice, mtSpiLcmDeviceNo, 1);
	mtSpiLcmClass = class_create(THIS_MODULE, SPI_DEVICE_NAME);
	if (IS_ERR(mtSpiLcmClass)) {
		printk("Failed to create class.\n");
		return PTR_ERR(mtSpiLcmClass);
	}

	dev = device_create(mtSpiLcmClass, NULL, mtSpiLcmDeviceNo, NULL, SPI_DEVICE_NAME);
	if (IS_ERR(dev)) {
		status = PTR_ERR(dev);
		printk(" device_create fail ret=%d\n", status);
		return PTR_ERR(mtSpiLcmClass);
	}
	
#ifdef CONFIG_OF
	mt_spi_gpio_init(pdev);
#endif

	spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
	status = spi_register_driver(&spi_lcm_driver);
	if (status < 0) {
		printk("Failed to register SPI driver.\n");
		return status;
	}

	return status;
}

static int mt_spi_dev_remove(struct platform_device *pdev){
	spi_unregister_driver(&spi_lcm_driver);
	class_destroy(mtSpiLcmClass);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_spi_dev_of_match[] = {
	{.compatible = "mediatek,mt_spi_lcm",},
	{},
};
#endif

static struct platform_driver mt_spi_dev_platform_driver = {
	.probe	  = mt_spi_dev_probe,
	.remove	 = mt_spi_dev_remove,
	.driver = {
		.name  = SPI_DEVICE_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt_spi_dev_of_match,
#endif
	}
};

static struct platform_device mt_spi_dev_platform_device = {
    .name = SPI_DEVICE_NAME,
    .id = 0,
    .dev = {}
};

static int __init mt_spi_display_init(void){
	mt_spi_pdev = &mt_spi_dev_platform_driver;
	
#ifdef CONFIG_OF	
	if(platform_device_register(&mt_spi_dev_platform_device)){
		printk("failed to register mt_spi_dev_platform_device \n");
		return -ENODEV;
	}
#endif

	if (platform_driver_register(&mt_spi_dev_platform_driver)){
		printk("failed to register mt_spi_dev_platform_driver  already exist\n");
		return -ENODEV;
	}
	
	return 0;
}
module_init(mt_spi_display_init);

static void __exit mt_spi_display_exit(void){
	platform_driver_unregister(&mt_spi_dev_platform_driver);
}
module_exit(mt_spi_display_exit);

MODULE_AUTHOR("EM, spi_lcm");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:mt_lcm_spi");


