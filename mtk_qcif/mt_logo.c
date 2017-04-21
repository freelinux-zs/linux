#include <debug.h>

#include <platform/mt_typedefs.h>
#include <platform/mt_disp_drv.h>
#include <platform/disp_drv.h>
#include <platform/lcd_drv.h>
#include <platform/mt_logo.h>
#include <platform/disp_drv_platform.h>

#include <target/board.h>
#include "lcm_drv.h"

#include <string.h>

/* show logo header file */
#include <show_logo_common.h>
#include <decompress_common.h>
#include <show_animation_common.h>


LCM_SCREEN_T phical_screen;
void  *logo_addr = NULL;

static LOGO_CUST_IF *logo_cust_if = NULL;

/********** show_animationm_ver:  charging animation version  ************/
/*                                                                       */ 
/* version 0: show 4 recatangle growing animation without battery number */
/* version 1: show wave animation with  battery number                   */
/*                                                                       */ 
/***                                                                   ***/

int show_animationm_ver = 0;


/*
 * Get the defined charging animation version 
 *
 */
void sync_anim_version()
{
    dprintf(INFO, "[lk logo: %s %d]\n",__FUNCTION__,__LINE__);
    
#ifdef ANIMATION_NEW
    show_animationm_ver = 1 ;     
#else
    show_animationm_ver = 0 ;
    dprintf(INFO, "[lk logo %s %d]not define ANIMATION_NEW:show old animation \n",__FUNCTION__,__LINE__); 
#endif

}


#ifdef MTK_SPI_LCM_PATCH
#if 1
#define  GPIO_SPI_CS 	(GPIO5 | 0x80000000)
#define  GPIO_SPI_CK 	(GPIO6 | 0x80000000)
#define  GPIO_SPI_RS 	(GPIO3 | 0x80000000)
#define  GPIO_SPI_SDA 	(GPIO4 | 0x80000000)

#define GPIO_SPI_SDA 		(GPIO4 | 0x80000000)
#define GPIO_SPI_REG_BASE	0x10211100
#define GPIO_SPI_CS_BIT 	0x20
#define GPIO_SPI_CLK_BIT 	0x40
#define GPIO_SPI_DC_BIT 	0x08
#define GPIO_SPI_SDA_BIT 	0x10
#else
#define GPIO_SPI_SDA 		(GPIO68 | 0x80000000)
#define GPIO_SPI_REG_BASE	0x10211120
#define GPIO_SPI_CS_BIT 	0x02
#define GPIO_SPI_CLK_BIT 	0x04
#define GPIO_SPI_DC_BIT 	0x08
#define GPIO_SPI_SDA_BIT 	0x10
#endif

extern int primary_display_get_line();

void mt_spi_write_reg(unsigned char cmd, unsigned char *data, unsigned char len){
	unsigned int i, val;	
	unsigned int val_clr;
	unsigned char da = 0;
	
	if(primary_display_get_line()==2){
		val_clr = ~(GPIO_SPI_CLK_BIT|GPIO_SPI_SDA_BIT);
		val = INREG32(GPIO_SPI_REG_BASE);
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;

		//BIT9
		val = (val&val_clr);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		
		val = (val&val_clr);
		if(cmd&0x80) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x40) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x20) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x10) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x08) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x04) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x02) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x01) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

		//send data
		for(i=0; i<len; i++){
			da = data[i];
			//BIT9
			val = (val&val_clr);
			val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		
			val = (val&val_clr);
			if(da&0x80) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x40) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x20) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x10) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x08) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x04) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x02) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x01) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		} 	
		
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		val = (val|GPIO_SPI_CS_BIT);		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
	}
	else{
		val_clr = ~(GPIO_SPI_CLK_BIT|GPIO_SPI_SDA_BIT);
		val = INREG32(GPIO_SPI_REG_BASE);
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
	
		val = (val&val_clr);
		if(cmd&0x80) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x40) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x20) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x10) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x08) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x04) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x02) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x01) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

		//send data
		val = (val|GPIO_SPI_DC_BIT);		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		for(i=0; i<len; i++){
			da = data[i];
			val = (val&val_clr);
			if(da&0x80) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x40) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x20) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x10) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x08) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x04) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x02) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			val = (val&val_clr);
			if(da&0x01) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		} 	
	
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		val = (val|GPIO_SPI_CS_BIT);		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
	}
}

void mt_spi_read_reg(unsigned char cmd, unsigned char *reg, unsigned char len){
	unsigned int i, val;
	unsigned int val_clr;
	unsigned char data = 0;

	if(primary_display_get_line()==2){
		val_clr = ~(GPIO_SPI_CLK_BIT|GPIO_SPI_SDA_BIT);
		val = INREG32(GPIO_SPI_REG_BASE);
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;

		//BIT9
		val = (val&val_clr);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

		val = (val&val_clr);
		if(cmd&0x80) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x40) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x20) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x10) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x08) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x04) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x02) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x01) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

		//val = (val&val_clr);
		//val = (val|GPIO_SPI_SDA_BIT);
		//(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		//(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		mt_set_gpio_dir(GPIO_SPI_SDA, GPIO_DIR_IN);
		//rx data
		for(i=0; i<len; i++){
			data = 0;
			//BIT0
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			//BIT1
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			data = data <<1;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			//BIT2
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			data = data <<1;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			//BIT3
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			data = data <<1;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			//BIT4
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			data = data <<1;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			//BIT5
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			data = data <<1;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			//BIT6
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			data = data <<1;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			//BIT7
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			data = data <<1;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			reg[i] = data;
		}

		mt_set_gpio_dir(GPIO_SPI_SDA, GPIO_DIR_OUT);
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		val = (val|GPIO_SPI_CS_BIT);		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
	}
	else{
		val_clr = ~(GPIO_SPI_CLK_BIT|GPIO_SPI_SDA_BIT);
		val = INREG32(GPIO_SPI_REG_BASE);
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		
		val = (val&val_clr);
		if(cmd&0x80) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x40) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x20) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x10) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x08) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x04) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x02) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		val = (val&val_clr);
		if(cmd&0x01) val = (val|GPIO_SPI_SDA_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

		
		mt_set_gpio_dir(GPIO_SPI_SDA, GPIO_DIR_IN);
		//rx data
		val = (val&val_clr);	
		val = (val|GPIO_SPI_DC_BIT);
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		//Dummy Clk
		if(len>2){
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		}
		
		for(i=0; i<len; i++){
			data = 0;
			//BIT7
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);	
			data = data <<1;
			
			//BIT6
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);	
			data = data <<1;

			//BIT5
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			data = data <<1;
			
			//BIT4
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			data = data <<1;
			
			//BIT3
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			data = data <<1;
			
			//BIT2
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			data = data <<1;
			
			//BIT1
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			data = data <<1;
			//BIT0
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			data = data | mt_get_gpio_in(GPIO_SPI_SDA);
			reg[i] = data;
		}
		
		mt_set_gpio_dir(GPIO_SPI_SDA, GPIO_DIR_OUT);
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		val = (val|GPIO_SPI_CS_BIT);		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
	}
}

static int mt_spi_set_blocksize(u16 x, u16 y, u16 w, u16 h){
	unsigned char data[32];
	
	w = w-1;
	h = h-1;
	
	data[0] = (x>>8);
	data[1] = (x&0xFF);
	data[2] = (w>>8);
	data[3] = (w&0xFF);
	mt_spi_write_reg(0x2a, data,4);
	
	data[0] = (y>>8);
	data[1] = (y&0xFF);
	data[2] = (h>>8);
	data[3] = (h&0xFF);
	mt_spi_write_reg(0x2b, data,4);
	
	data[0] = 0x00;
	mt_spi_write_reg(0x2c, data,0);

	return 0;
}

static void _disp_update_screen(unsigned char * screen, int x, int y, int w, int h){
	unsigned int *pix_buf;
	unsigned int index;
	unsigned int val, val_clr;
	unsigned int data_max;
	unsigned int pix;
	
	mt_spi_set_blocksize(x,y,w,h);

	if(primary_display_get_line()==2){
		data_max = (w-x)*(h-y)*2;	
		val = INREG32(GPIO_SPI_REG_BASE);
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));	
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
	
		pix_buf = (unsigned int *)screen;
		val_clr = ~(GPIO_SPI_CLK_BIT|GPIO_SPI_SDA_BIT);
		for(index=0; index<data_max/4; index++) {
			pix = pix_buf[index];
			//BIT9
			val = (val&val_clr);
			val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			
			val = (val&val_clr);
			if(pix&0x8000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT6
			val = (val&val_clr);
			if(pix&0x4000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT5
			val = (val&val_clr);
			if(pix&0x2000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT4
			val = (val&val_clr);
			if(pix&0x1000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT3
			val = (val&val_clr);
			if(pix&0x0800) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT2
			val = (val&val_clr);
			if(pix&0x0400) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT1
			val = (val&val_clr);
			if(pix&0x0200) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT0
			val = (val&val_clr);
			if(pix&0x0100) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//////////////////////////////////////////////////////////
			//BIT9
			val = (val&val_clr);
			val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			
			val = (val&val_clr);
			if(pix&0x0080) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT6
			val = (val&val_clr);
			if(pix&0x0040) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT5
			val = (val&val_clr);
			if(pix&0x0020) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT4
			val = (val&val_clr);
			if(pix&0x0010) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT3
			val = (val&val_clr);
			if(pix&0x0008) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT2
			val = (val&val_clr);
			if(pix&0x0004) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT1
			val = (val&val_clr);
			if(pix&0x0002) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT0
			val = (val&val_clr);
			if(pix&0x0001) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT9
			val = (val&val_clr);
			val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			
			val = (val&val_clr);
			if(pix&0x80000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT6
			val = (val&val_clr);
			if(pix&0x40000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT5
			val = (val&val_clr);
			if(pix&0x20000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT4
			val = (val&val_clr);
			if(pix&0x10000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT3
			val = (val&val_clr);
			if(pix&0x08000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT2
			val = (val&val_clr);
			if(pix&0x04000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT1
			val = (val&val_clr);
			if(pix&0x02000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT0
			val = (val&val_clr);
			if(pix&0x01000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//////////////////////////////////////////////////////////
			//BIT9
			val = (val&val_clr);
			val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			
			val = (val&val_clr);
			if(pix&0x00800000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT6
			val = (val&val_clr);
			if(pix&0x00400000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT5
			val = (val&val_clr);
			if(pix&0x00200000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT4
			val = (val&val_clr);
			if(pix&0x00100000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT3
			val = (val&val_clr);
			if(pix&0x00080000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT2
			val = (val&val_clr);
			if(pix&0x00040000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT1
			val = (val&val_clr);
			if(pix&0x00020000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT0
			val = (val&val_clr);
			if(pix&0x00010000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		}

		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		val = (val|GPIO_SPI_CS_BIT);		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
	}
	else{
		val = INREG32(GPIO_SPI_REG_BASE);
		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));	
		val = (val|GPIO_SPI_DC_BIT);		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
		
		data_max = (w-x)*(h-y)*2;
		pix_buf = (unsigned int *)screen;

		val = INREG32(GPIO_SPI_REG_BASE);
		val_clr = ~(GPIO_SPI_CLK_BIT|GPIO_SPI_SDA_BIT);
		for(index=0; index<data_max/4; index++) {
			pix = *pix_buf++;

			val = (val&val_clr);
			if(pix&0x8000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT6
			val = (val&val_clr);
			if(pix&0x4000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT5
			val = (val&val_clr);
			if(pix&0x2000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			//BIT4
			val = (val&val_clr);
			if(pix&0x1000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			//BIT3
			val = (val&val_clr);
			if(pix&0x0800) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			//BIT2
			val = (val&val_clr);
			if(pix&0x0400) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT1
			val = (val&val_clr);
			if(pix&0x0200) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT0
			val = (val&val_clr);
			if(pix&0x0100) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT7
			val = (val&val_clr);
			if(pix&0x0080) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT6
			val = (val&val_clr);
			if(pix&0x0040) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT5
			val = (val&val_clr);
			if(pix&0x0020) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT4
			val = (val&val_clr);
			if(pix&0x0010) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT3
			val = (val&val_clr);
			if(pix&0x0008) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT2
			val = (val&val_clr);
			if(pix&0x0004) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT1
			val = (val&val_clr);
			if(pix&0x0002) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT0
			val = (val&val_clr);
			if(pix&0x0001) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
			
			//BIT7	
			val = (val&val_clr);
			if(pix&0x80000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT6
			val = (val&val_clr);
			if(pix&0x40000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT5
			val = (val&val_clr);
			if(pix&0x20000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT4
			val = (val&val_clr);
			if(pix&0x10000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT3
			val = (val&val_clr);
			if(pix&0x08000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT2
			val = (val&val_clr);
			if(pix&0x04000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT1
			val = (val&val_clr);
			if(pix&0x02000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT0
			val = (val&val_clr);
			if(pix&0x01000000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT7
			val = (val&val_clr);
			if(pix&0x00800000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT6
			val = (val&val_clr);
			if(pix&0x00400000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT5
			val = (val&val_clr);
			if(pix&0x00200000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT4
			val = (val&val_clr);
			if(pix&0x00100000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT3
			val = (val&val_clr);
			if(pix&0x00080000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT2
			val = (val&val_clr);
			if(pix&0x00040000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT1
			val = (val&val_clr);
			if(pix&0x00020000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);

			//BIT0
			val = (val&val_clr);
			if(pix&0x00010000) val = (val|GPIO_SPI_SDA_BIT);
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
			(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = (val|GPIO_SPI_CLK_BIT);
		}

		val = (val&(~(GPIO_SPI_CS_BIT|GPIO_SPI_CLK_BIT|GPIO_SPI_DC_BIT|GPIO_SPI_SDA_BIT)));		
		val = (val|GPIO_SPI_CS_BIT);		
		(*(volatile unsigned int * const)(GPIO_SPI_REG_BASE)) = val;
	}
}
#endif

/*
 * Initliaze charging animation parameters 
 *
 */
void init_fb_screen()
{
    dprintf(INFO, "[lk logo: %s %d]\n",__FUNCTION__,__LINE__);
    unsigned int fb_size = mt_get_fb_size();
    logo_addr = mt_get_logo_db_addr();

    phical_screen.width = CFG_DISPLAY_WIDTH;
    phical_screen.height = CFG_DISPLAY_HEIGHT;
    phical_screen.fb_size = fb_size;   
    phical_screen.fill_dst_bits = CFG_DISPLAY_BPP;   
    phical_screen.bits_per_pixel = CFG_DISPLAY_BPP;
        
    // in JB2.MP need to allign width and height to 32 ,but jb5.mp needn't   
    phical_screen.needAllign = 1;
    phical_screen.allignWidth = ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT);
    
#ifdef MTK_SPI_LCM_PATCH
    phical_screen.fill_dst_bits = 16;
    phical_screen.bits_per_pixel = 16;
    phical_screen.allignWidth = ALIGN_TO(CFG_DISPLAY_WIDTH, 16);
#endif

    /* In GB, no need to adjust 180 showing logo ,for fb driver dealing the change */
    /* but in JB, need adjust it for screen 180 roration           */
    phical_screen.need180Adjust = 0;   // need sync with chip driver 
    
    dprintf(INFO, "[lk logo: %s %d]MTK_LCM_PHYSICAL_ROTATION = %s\n",__FUNCTION__,__LINE__, MTK_LCM_PHYSICAL_ROTATION);

    if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "270", 3))
    { 
        phical_screen.rotation = 270;
    } else if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "90", 2)){ 
        phical_screen.rotation = 90;
    } else if(0 == strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3) && (phical_screen.need180Adjust == 1)){ 
        phical_screen.rotation = 180;   
    } else {
        phical_screen.rotation = 0;   
    }
    
    sync_anim_version();
    if (show_animationm_ver == 1)
    {
        unsigned int logonum;
        unsigned int *db_addr = logo_addr;
    
        unsigned int *pinfo = (unsigned int*)db_addr;
        
        logonum = pinfo[0];
        dprintf(INFO, "[lk logo: %s %d]pinfo[0]=0x%08x, pinfo[1]=0x%08x, pinfo[2]=%d\n", __FUNCTION__,__LINE__,
                    pinfo[0], pinfo[1], pinfo[2]);
    
        dprintf(INFO, "[lk logo: %s %d]define ANIMATION_NEW:show new animation with capacity num\n",__FUNCTION__,__LINE__); 
        dprintf(INFO, "[lk logo: %s %d]CAPACITY_LEFT =%d, CAPACITY_TOP =%d \n",__FUNCTION__,__LINE__,(CAPACITY_LEFT) ,(CAPACITY_TOP) ); 
        dprintf(INFO, "[lk logo: %s %d]LCM_HEIGHT=%d, LCM_WIDTH=%d \n",__FUNCTION__,__LINE__,(CAPACITY_RIGHT),(CAPACITY_BOTTOM)); 
        if(logonum < 6)
        {
            show_animationm_ver = 0 ;
        } else {
            show_animationm_ver = 1 ;   
        }
    }
    
}


/*
 * Custom interface 
 *
 */
void mt_logo_get_custom_if(void)
{
    if(logo_cust_if == NULL)
    {
        logo_cust_if = (LOGO_CUST_IF *)LOGO_GetCustomIF();
    }
}


/*
 * Show first boot logo when phone boot up
 *
 */
void mt_disp_show_boot_logo(void)
{
    dprintf(INFO, "[lk logo: %s %d]\n",__FUNCTION__,__LINE__);    
    mt_logo_get_custom_if();

    if(logo_cust_if->show_boot_logo)
    {
        logo_cust_if->show_boot_logo();
    }
    else
    {
        ///show_logo(0);
        init_fb_screen();
        fill_animation_logo(BOOT_LOGO_INDEX, mt_get_fb_addr(), mt_get_tempfb_addr(), logo_addr, phical_screen);
        mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#ifdef MTK_SPI_LCM_PATCH
        _disp_update_screen(mt_get_fb_addr(), 0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#endif
    }

    return;
}

/*
 * Custom interface : charging state
 *
 */
void mt_disp_enter_charging_state(void)
{
    mt_logo_get_custom_if();

    if(logo_cust_if->enter_charging_state)
    {
        logo_cust_if->enter_charging_state();
    }
    else
    {
        dprintf(INFO, "[lk logo: %s %d]do nothing \n",__FUNCTION__,__LINE__);
    }

    return;
}


/*
 * Show full battery when poweroff charging
 *
 */
void mt_disp_show_battery_full(void)
{
    dprintf(INFO, "[lk logo: %s %d]\n",__FUNCTION__,__LINE__);
    mt_disp_show_battery_capacity(100);
}


/*
 * Show animation when poweroff charging
 *
 */
void mt_disp_show_battery_capacity(UINT32 capacity)
{
    dprintf(INFO, "[lk logo: %s %d]capacity =%d\n",__FUNCTION__,__LINE__, capacity);
    mt_logo_get_custom_if();

    if(logo_cust_if->show_battery_capacity)
    {
        logo_cust_if->show_battery_capacity(capacity);
    }
    else
    {     
        init_fb_screen();
        
        fill_animation_battery_by_ver(capacity, mt_get_fb_addr(), mt_get_tempfb_addr(), logo_addr, phical_screen, show_animationm_ver);            
                  
        mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
    }

}

/*
 * Show charging over logo
 *
 */
void mt_disp_show_charger_ov_logo(void)
{
    dprintf(INFO, "[lk logo: %s %d]\n",__FUNCTION__,__LINE__);
    mt_logo_get_custom_if();

    if(logo_cust_if->show_boot_logo)
    {
        logo_cust_if->show_boot_logo();
    }
    else
    {
        init_fb_screen();
        fill_animation_logo(CHARGER_OV_INDEX, mt_get_fb_addr(), mt_get_tempfb_addr(), logo_addr, phical_screen);
        mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
    }

    return;
}

/*
 * Show low battery logo 
 *
 */
void mt_disp_show_low_battery(void)
{
    dprintf(INFO, "[lk logo: %s %d]\n",__FUNCTION__,__LINE__);
    mt_logo_get_custom_if();

    if(logo_cust_if->show_boot_logo)
    {
        logo_cust_if->show_boot_logo();
    }
    else
    {
        init_fb_screen();
        //show_logo(2);
        fill_animation_logo(LOW_BATTERY_INDEX, mt_get_fb_addr(), mt_get_tempfb_addr(), logo_addr, phical_screen);
        mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#ifdef MTK_SPI_LCM_PATCH
        _disp_update_screen(mt_get_fb_addr(), 0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
#endif
    }

    return;
}


/*
 * Fill rectangle region for with black  or other color  
 *
 */
void mt_disp_fill_rect(UINT32 left, UINT32 top,
                           UINT32 right, UINT32 bottom,
                           UINT32 color)
{
    dprintf(INFO, "[lk logo: %s %d]\n",__FUNCTION__,__LINE__);   
    init_fb_screen();
    RECT_REGION_T rect = {left, top, right, bottom};
    
    fill_rect_with_color(mt_get_fb_addr(), rect, color, phical_screen);     
}

