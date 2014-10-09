/*
 * ov772x Camera Driver
 *
 * Based on ov772x and hisi i2c driver,
 *
 * Copyright (C) 2014, chenwen <crushonme@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
     
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
     //#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
     //#include <asm/semaphore.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
     //#include <asm/hardware.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>

#include "ov7725.h"

#define HI_GPIO_I2C
#ifdef HI_GPIO_I2C
#include "gpio_i2c.h"
#else
#include "i2c.h"
#endif



//Referrence Count
static unsigned int  open_cnt = 0;

void ov7725_write(unsigned char chip_addr,unsigned char reg_addr,unsigned char value)
{
#ifdef HI_GPIO_I2C
    gpio_i2c_write(chip_addr,reg_addr,value);
#else
    i2c_write(chip_addr,(unsigned int)reg_addr,(unsigned int)value);
#endif
}
int ov7725_read(unsigned char chip_addr,unsigned char reg_addr)
{
#ifdef HI_GPIO_I2C
    return gpio_sccb_read(chip_addr,reg_addr);
#else    
    return i2c_read(chip_addr,(unsigned int)reg_addr);
#endif
}

#define SCCB_OV7725_ADDR 0x42

/*
* size = 0: VGA 640x480
* size = 1: QVGA 320x240
* size = 2: QQVGA 160x120
* size = 3: CIF 352x288
* size = 4: QCIF 176x144
*/
static int size = 0;

/* set output drive capability
 * param:multi = 0/1/2/3
 * return 0:set success, return 1:set error
 */
static unsigned int set_output_drivecap(unsigned char multi )
{
	switch(multi)
	{
	case 0:
		ov7725_write(SCCB_OV7725_ADDR, COM2, 0x00);
		break;
	case 1:
		ov7725_write(SCCB_OV7725_ADDR, COM2, 0x01);
		break;
	case 2:
		ov7725_write(SCCB_OV7725_ADDR, COM2, 0x02);
		break;
	case 3:
		ov7725_write(SCCB_OV7725_ADDR, COM2, 0x03);
		break;
	default:
		return 1;
	}
	return 0;
}

/* set output video data format
 * param: mode = 0/1/2/3/4
 * return 0:set success, return 1:set error
 */
static unsigned int set_output_format(enum output_format form)
{
    unsigned char regval;
    switch(form)
    {
        case YUV:
            ov7725_write(SCCB_OV7725_ADDR, COM3, 0x10);//swap Y/UV output sequence
            ov7725_write(SCCB_OV7725_ADDR, 0x66, 0x00);        //U0Y0,V1Y1,U2Y2,V3Y3,....
            regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
            ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x20);//BT.656 protocol on,YUV output
            break;	
        case GBR422:
            regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
            ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x02);
            break;
        case RGB565:
            regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
            ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x06);
            break;
        case RGB555:
            regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
            ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x0a);
            break;
        case RGB444:
            regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
            ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x0e);
            break;
        case PROBRAW:
            ov7725_write(SCCB_OV7725_ADDR, 0x67, 0x4a);//DSP output RAW8
            regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
            ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x01);
            break;
        case BRAW:
            ov7725_write(SCCB_OV7725_ADDR, 0x67, 0x4a);
            regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
            ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x03);
            break;
        default:
            return 1;
    }
    return 0;
}

static void set_vga(void)
{
    unsigned char regval;
    ov7725_write(SCCB_OV7725_ADDR, HSTART, 0x22);
    ov7725_write(SCCB_OV7725_ADDR, HSIZE, 0xa4);//sensor size is larger than VGA
    ov7725_write(SCCB_OV7725_ADDR, VSTART, 0x07);
    ov7725_write(SCCB_OV7725_ADDR, VSIZE, 0xf0);
    ov7725_write(SCCB_OV7725_ADDR, HREF, 0x00);
    regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
    ov7725_write(SCCB_OV7725_ADDR, COM7, regval&(0x40));
    ov7725_write(SCCB_OV7725_ADDR, HOUTSIZE, 0xa0);
    ov7725_write(SCCB_OV7725_ADDR, VOUTSIZE, 0xf0);
    ov7725_write(SCCB_OV7725_ADDR, EXHCH, 0x00);
//    ov7725_write(SCCB_OV7725_ADDR, CLKRC, 0x01);//00/01/03/07 for 60/30/15/7.5fps
}

static void set_qvga(void)
{
    unsigned char regval;
    ov7725_write(SCCB_OV7725_ADDR, HSTART, 0x3f);
    ov7725_write(SCCB_OV7725_ADDR, HSIZE, 0x50);
    ov7725_write(SCCB_OV7725_ADDR, VSTART, 0x03);
    ov7725_write(SCCB_OV7725_ADDR, VSIZE, 0x78);
    ov7725_write(SCCB_OV7725_ADDR, HREF, 0x00);
    regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
    ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x40);
    ov7725_write(SCCB_OV7725_ADDR, HOUTSIZE, 0x50);
    ov7725_write(SCCB_OV7725_ADDR, VOUTSIZE, 0x78);
    ov7725_write(SCCB_OV7725_ADDR, EXHCH, 0x00);
//    ov7725_write(SCCB_OV7725_ADDR, CLKRC, 0x01);//00/01/03/07 for 60/30/15/7.5fps
}

static void set_qqvga(void)
{
    unsigned char regval;
    ov7725_write(SCCB_OV7725_ADDR, HSTART, 0x3f);
    ov7725_write(SCCB_OV7725_ADDR, HSIZE, 0x50);
    ov7725_write(SCCB_OV7725_ADDR, VSTART, 0x03);
    ov7725_write(SCCB_OV7725_ADDR, VSIZE, 0x78);
    ov7725_write(SCCB_OV7725_ADDR, HREF, 0x00);
    regval = ov7725_read(SCCB_OV7725_ADDR, COM7);
    ov7725_write(SCCB_OV7725_ADDR, COM7, regval|0x40);
   // 1/2 down sampling
    ov7725_write(SCCB_OV7725_ADDR, SCAL0, 0x05); 
    ov7725_write(SCCB_OV7725_ADDR, SCAL1, 0x80);
    ov7725_write(SCCB_OV7725_ADDR, SCAL2, 0x80);
    ov7725_write(SCCB_OV7725_ADDR, HOUTSIZE, 0x50);
    ov7725_write(SCCB_OV7725_ADDR, VOUTSIZE, 0x78);
    ov7725_write(SCCB_OV7725_ADDR, EXHCH, 0x00);
    ov7725_write(SCCB_OV7725_ADDR, 0x65, 0x2f);//vertical and horizontal zoom out enable
//    ov7725_write(SCCB_OV7725_ADDR, CLKRC, 0x01);//00/01/03/07 for 60/30/15/7.5fps
}

static void set_cif(void)
{
//    unsigned char regval;
    ov7725_write(SCCB_OV7725_ADDR, HSTART, 0x22);
    ov7725_write(SCCB_OV7725_ADDR, HSIZE, 0xa4);
    ov7725_write(SCCB_OV7725_ADDR, VSTART, 0x07);
    ov7725_write(SCCB_OV7725_ADDR, VSIZE, 0xf0);
    ov7725_write(SCCB_OV7725_ADDR, HREF, 0x00);	//sensor size is VGA	
		
    ov7725_write(SCCB_OV7725_ADDR, HOUTSIZE, 0x5a);
    ov7725_write(SCCB_OV7725_ADDR, VOUTSIZE, 0x90);
    ov7725_write(SCCB_OV7725_ADDR, EXHCH, 0x00);
    ov7725_write(SCCB_OV7725_ADDR, 0x65, 0x2f);//vertical and horizontal zoom out enable
//    ov7725_write(SCCB_OV7725_ADDR, CLKRC, 0x01);//00/01/03/07 for 60/30/15/7.5fps
}

static void set_qcif(void)
{
//    unsigned char regval;
    ov7725_write(SCCB_OV7725_ADDR, HSTART, 0x22);
    ov7725_write(SCCB_OV7725_ADDR, HSIZE, 0xa4);
    ov7725_write(SCCB_OV7725_ADDR, VSTART, 0x07);
    ov7725_write(SCCB_OV7725_ADDR, VSIZE, 0xf0);
    ov7725_write(SCCB_OV7725_ADDR, HREF, 0x00);
//    ov7725_write(SCCB_OV7725_ADDR, SCAL0, HALF);
		
    ov7725_write(SCCB_OV7725_ADDR, HOUTSIZE, 0x5a);
    ov7725_write(SCCB_OV7725_ADDR, VOUTSIZE, 0x90);
    ov7725_write(SCCB_OV7725_ADDR, EXHCH, 0x00);
    ov7725_write(SCCB_OV7725_ADDR, 0x65, 0x2f);//vertical and horizontal zoom out enable
//   ov7725_write(SCCB_OV7725_ADDR, SCAL1, 0x80);
//    ov7725_write(SCCB_OV7725_ADDR, SCAL2, 0x80);
//    ov7725_write(SCCB_OV7725_ADDR, CLKRC, 0x01);//00/01/03/07 for 60/30/15/7.5fps
}
/* set luma 
* param: bri: 0x00 ~ 0xff
*/ 
static void set_bright(unsigned char bri)
{
    ov7725_write(SCCB_OV7725_ADDR, SDE, 0x04); //contrast/bright enable
    ov7725_write(SCCB_OV7725_ADDR, 0x9B, bri);
}

/* set contrast
* param: con > 0x20, enhance Y
              con < 0x20, weaken Y
*/
static void set_contrast(unsigned char con)
{
    ov7725_write(SCCB_OV7725_ADDR, SDE, 0x04); //contrast/bright enable
    ov7725_write(SCCB_OV7725_ADDR, 0x9c, con);
}

/* set saturation
* param: sat > 0x40, U component saturation enhance 
*             sat < 0x40, U component saturation weaken
*/
static void set_saturation(unsigned char sat)
{
    ov7725_write(SCCB_OV7725_ADDR, SDE, 0x02); //saturation enable
    ov7725_write(SCCB_OV7725_ADDR, 0xa7, sat);
    ov7725_write(SCCB_OV7725_ADDR, 0xa8, sat);
}

/* set hue
* param: 0x00 <= hue <= 0xff
*/
static void set_hue(unsigned char hue)
{
    unsigned char reghue;
    reghue = (hue + 1)/2;//range of value to 0x00~0x80
    ov7725_write(SCCB_OV7725_ADDR, SDE, 0x01); //hue enable
    ov7725_write(SCCB_OV7725_ADDR, 0xa9, reghue);
    ov7725_write(SCCB_OV7725_ADDR, 0xaa, reghue);
}

/*night mode auto frame rate max rate control
* return 0:success, return 1: error
*/
static unsigned int set_night_autoframerate(enum nightmode nm)
{
    unsigned char regval;
	switch(nm)
	{
	case AUTOFRAMERATE_OFF:
		ov7725_write(SCCB_OV7725_ADDR, COM5, 0x01);
		break;
	case AUTOFRAMERATE_ON:
		regval = ov7725_read(SCCB_OV7725_ADDR, COM5);
		//SET_BIT(regval, 0x80);
		ov7725_write(SCCB_OV7725_ADDR, COM5, regval|0x80);
		break;
	case AUTOFRAMERATE_NORMAL:
		ov7725_write(SCCB_OV7725_ADDR, COM5, 0x85);
		break;
	case AUTOFRAMERATE_HALF:
		ov7725_write(SCCB_OV7725_ADDR, COM5, 0x95);
		break;
	case AUTOFRAMERATE_QUAT:
		ov7725_write(SCCB_OV7725_ADDR, COM5, 0xa5);
		break;
	case AUTOFRAMERATE_EITH:
		ov7725_write(SCCB_OV7725_ADDR, COM5, 0xb5);
		break;
	default:
		return 1;
	}
	return 0;
}

static void ov7725_reg_init(void)
{
    ov7725_write(SCCB_OV7725_ADDR, 0x3d, 0x03); //DC offset for analog process
    //VGA
    ov7725_write(SCCB_OV7725_ADDR, 0x17, 0x22); //HStart
    ov7725_write(SCCB_OV7725_ADDR, 0x18, 0xa4); //HSize
    ov7725_write(SCCB_OV7725_ADDR, 0x19, 0x07); //VStart
    ov7725_write(SCCB_OV7725_ADDR, 0x1a, 0xf0); //VSize
    ov7725_write(SCCB_OV7725_ADDR, 0x32, 0x00); //HREF
    ov7725_write(SCCB_OV7725_ADDR, 0x29, 0xa0); //Houtsize 8MSB
    ov7725_write(SCCB_OV7725_ADDR, 0x2c, 0xf0); //Voutsize 8MSB
    ov7725_write(SCCB_OV7725_ADDR, 0x2a, 0x00); //Houtsize 2MSB,Voutsize 1MSB
    ov7725_write(SCCB_OV7725_ADDR, 0x11, 0x01); //internal clock
    //DSP control
    ov7725_write(SCCB_OV7725_ADDR, 0x42, 0x7f);
    ov7725_write(SCCB_OV7725_ADDR, 0x4d, 0x09);
    ov7725_write(SCCB_OV7725_ADDR, 0x63, 0xe0);
    ov7725_write(SCCB_OV7725_ADDR, 0x64, 0xff);
    ov7725_write(SCCB_OV7725_ADDR, 0x65, 0x20);//vertical/horizontal zoom disable
    ov7725_write(SCCB_OV7725_ADDR, 0x66, 0x00);
    ov7725_write(SCCB_OV7725_ADDR, 0x67, 0x48);
    //AGC AEC AWB
    ov7725_write(SCCB_OV7725_ADDR, 0x13, 0xf0);
    ov7725_write(SCCB_OV7725_ADDR, 0x0d, 0x41);
    ov7725_write(SCCB_OV7725_ADDR, 0x0f, 0xc5);
    ov7725_write(SCCB_OV7725_ADDR, 0x14, 0x11);
    ov7725_write(SCCB_OV7725_ADDR, 0x22, 0x7f);
    ov7725_write(SCCB_OV7725_ADDR, 0x23, 0x03);
    ov7725_write(SCCB_OV7725_ADDR, 0x24, 0x40);
    ov7725_write(SCCB_OV7725_ADDR, 0x25, 0x30);
    ov7725_write(SCCB_OV7725_ADDR, 0x26, 0xa1);
    ov7725_write(SCCB_OV7725_ADDR, 0x2b, 0x00);//50Hz
    ov7725_write(SCCB_OV7725_ADDR, 0x6b, 0xaa);
    ov7725_write(SCCB_OV7725_ADDR, 0x13, 0xff);
    //matrix sharpness brightness contrast UV
    ov7725_write(SCCB_OV7725_ADDR, 0x90, 0x05);
    ov7725_write(SCCB_OV7725_ADDR, 0x91, 0x01);
    ov7725_write(SCCB_OV7725_ADDR, 0x92, 0x03);
    ov7725_write(SCCB_OV7725_ADDR, 0x93, 0x00);
	
    ov7725_write(SCCB_OV7725_ADDR, 0x94, 0xb0);
    ov7725_write(SCCB_OV7725_ADDR, 0x95, 0x9d);
    ov7725_write(SCCB_OV7725_ADDR, 0x96, 0x13);
    ov7725_write(SCCB_OV7725_ADDR, 0x97, 0x16);
    ov7725_write(SCCB_OV7725_ADDR, 0x98, 0x7b);
    ov7725_write(SCCB_OV7725_ADDR, 0x99, 0x91);
    ov7725_write(SCCB_OV7725_ADDR, 0x9a, 0x1e);
	
    ov7725_write(SCCB_OV7725_ADDR, 0x9b, 0x08);
    ov7725_write(SCCB_OV7725_ADDR, 0x9c, 0x20);
    ov7725_write(SCCB_OV7725_ADDR, 0x9e, 0x81);
    ov7725_write(SCCB_OV7725_ADDR, 0xa6, 0x04);
    //gamma
    ov7725_write(SCCB_OV7725_ADDR, 0x7e, 0x0c);
    ov7725_write(SCCB_OV7725_ADDR, 0x7f, 0x16);
    ov7725_write(SCCB_OV7725_ADDR, 0x80, 0x2a);
    ov7725_write(SCCB_OV7725_ADDR, 0x81, 0x4e);
    ov7725_write(SCCB_OV7725_ADDR, 0x82, 0x61);
    ov7725_write(SCCB_OV7725_ADDR, 0x83, 0x6f);
    ov7725_write(SCCB_OV7725_ADDR, 0x84, 0x7b);
    ov7725_write(SCCB_OV7725_ADDR, 0x85, 0x86);
    ov7725_write(SCCB_OV7725_ADDR, 0x86, 0x8e);
    ov7725_write(SCCB_OV7725_ADDR, 0x87, 0x97);
    ov7725_write(SCCB_OV7725_ADDR, 0x88, 0xa4);
    ov7725_write(SCCB_OV7725_ADDR, 0x89, 0xaf);
    ov7725_write(SCCB_OV7725_ADDR, 0x8a, 0xc5);
    ov7725_write(SCCB_OV7725_ADDR, 0x8b, 0xd7);
    ov7725_write(SCCB_OV7725_ADDR, 0x8c, 0xe8);
    ov7725_write(SCCB_OV7725_ADDR, 0x8d, 0x20);

    //night mode auto frame rate control
    ov7725_write(SCCB_OV7725_ADDR, 0x0e, 0x65);//auto frame rate control on
    
}

/*
 * ov9653 open routine. 
 * do nothing.
 */ 
int ov7725_open(struct inode * inode, struct file * file)
{
	if(0 == open_cnt++)
		return 0;
    return -1;
}

/*
 * ov9653 close routine. 
 * do nothing.
 *
 */ 
int ov7725_close(struct inode * inode, struct file * file)
{
    open_cnt--;
    return 0;
}

/* ioctl function
  * return 0:success, return -1: failed
 */
//int ov7725_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
int ov7725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	unsigned long val;
	unsigned char regaddr;
	unsigned char regval;
	enum output_format format;
	enum nightmode FrameRate;
	if (copy_from_user(&val, argp, sizeof(val)))
	{
		printk("\tOV7725_ERROR: WRONG copy val is %lu\n",val);
        	return -1;
	}
	switch(cmd)
	{
		case SET_OUTPUT_FORMAT:
		{			
			format = (enum output_format)(val & 0xff);
			if (set_output_format(format) )
			{
				return -1;
			}
			return 0;
		}
		case SET_OUTPUT_VGA:
		{			
			set_vga();
			return 0;
		}
		case SET_OUTPUT_QVGA:
		{
			set_qvga();
			return 0;
		}
		case SET_OUTPUT_QQVGA:
		{
			set_qqvga();
			return 0;
		}
		case SET_OUTPUT_CIF:
		{
			set_cif();
			return 0;
		}
		case SET_OUTPUT_QCIF:
		{
			set_qcif();
			return 0;
		}
		case SET_NIGHT_FRAMERATE:
		{			
			FrameRate = (enum nightmode)(val & 0xff);
			if(set_night_autoframerate(FrameRate))
			{
				return -1;
			}
			return 0;
		}
		case SET_BRIGHT:
		{
			regval= val & 0xff;
			set_bright(regval);
			return 0;
		}
		case SET_CONTRAST:
		{
			regval= val & 0xff;
			set_contrast(regval);
			return 0;
		}
		case SET_SATURATION:
		{
			regval= val & 0xff;
			set_saturation(regval);
			return 0;
		}
		case SET_HUE:
		{
			regval =  val & 0xff;
			set_hue(regval);
			return 0;
		}
		case READ_REGISTER:
		{
			regaddr= val & 0xff;
			regval = ov7725_read(SCCB_OV7725_ADDR, regaddr);
			copy_to_user(argp, &regval, sizeof(regval));
			return 0;
		}
		case WRITE_REGISTER:
		{
			regaddr = val & 0xff;
			regval = (val & 0xff00) >> 8;
			ov7725_write(SCCB_OV7725_ADDR, regaddr, regval);
			return 0;
		}
		default:
			return -1;
	}
}

static struct file_operations ov7725_fops = {
  .owner = THIS_MODULE,
  //.ioctl    = ov7725_ioctl,
  .unlocked_ioctl = ov7725_ioctl,
  .open   = ov7725_open,
  .release  = ov7725_close
};

static struct miscdevice ov7725_dev = {
  MISC_DYNAMIC_MINOR,
  "ov7725dev",
  &ov7725_fops,
};

static int ov7725_device_init(void)
{
    unsigned char regvalue, loop;
    ov7725_write(SCCB_OV7725_ADDR, COM7, 0x80); //reset all register
    for(loop=0;loop < 10;loop++)//delay
    {
        ;
    }
    regvalue = ov7725_read(SCCB_OV7725_ADDR, COM7); 
    regvalue &=0x7f;  
    ov7725_write(SCCB_OV7725_ADDR, COM7, regvalue);
    regvalue = ov7725_read(SCCB_OV7725_ADDR,PID);
    loop = ov7725_read(SCCB_OV7725_ADDR,VER);  
    if((regvalue != 0x77)||(loop != 0x21))
    {
        printk("read Prodect ID Number MSB is %x\n",regvalue);
        printk("read Prodect ID Number LSB is %x\n",loop);
        printk("check ov7725 ID error.\n");
        return -1;
    }
    ov7725_reg_init();
    set_output_drivecap(1);
    set_output_format(0);	 
    if (size == 0)
    {
        set_vga();//VGA
    }
    else if (size == 1)
    {
        set_qvga();//QVGA
    }
    else if (size == 2)
    {
        set_qqvga();//QQVGA
    }
    else if (size == 3)
    {
        set_cif();//CIF
    }
    else if (size == 4)
    {
        set_qcif();//QCIF
    }
    else
    {
	 printk("\tOV7725_ERROR: input param error!\n");
	 return -1;
    }
    return 0;
}

static int __init ov7725_init(void)
{
	int ret = 0;
	ret = misc_register(&ov7725_dev);
	if (ret)
	{
		printk("OV7725_ERROR:cannot regiter ov7725 device!\n");
		return ret;
	}
	if(ov7725_device_init())
	{
		misc_deregister(&ov7725_dev);
		printk("\tOV7725_ERROR:ov7725 driver init failed for device init error!\n");
		return -1;
	}
	printk("ov7725 driver init successful!\n");
	return 0;
}

static void __exit ov7725_exit(void)
{
	misc_deregister(&ov7725_dev);
	printk("ov7725 driver exist!\n");
}

module_init(ov7725_init);
module_exit(ov7725_exit);
module_param(size, int, S_IRUGO);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chenwen");

