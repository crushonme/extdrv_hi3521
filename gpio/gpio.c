
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/bitops.h>

#include "gpio.h" 


/* GPIO Pin Table, must be the same order as enum gpio_pin */
static struct gpio_cfg gpio_cfgs[] = {
	/* bank, pin, dir, default val */
	{7, 3, GPIO_DIR_OUTPUT, 1},
	{7, 0, GPIO_DIR_OUTPUT, 1},
	{7, 4, GPIO_DIR_OUTPUT, 1},
	{7, 1, GPIO_DIR_OUTPUT, 0},
	{7, 2, GPIO_DIR_OUTPUT, 0},
	{8, 4, GPIO_DIR_OUTPUT, 1},
	{7, 5, GPIO_DIR_OUTPUT, 1},
	{8, 5, GPIO_DIR_OUTPUT, 1},
	{8, 3, GPIO_DIR_OUTPUT, 1},
	{11, 3, GPIO_DIR_OUTPUT, 1},
	{11, 2, GPIO_DIR_OUTPUT, 1},
	{8, 3, GPIO_DIR_OUTPUT, 1},
	{8, 3, GPIO_DIR_OUTPUT, 1},

	{GPIO_CFG_END_SIGN}  /* End Flag */
};

/*
 * Set GPIO direct
 * dir: GPIO_HW_DIR_INPUT/GPIO_HW_DIR_OUTPUT
 */

int brd_gpio_dir(int bank, int pin, enum gpio_dir dir)
{
	int reg = 0;

	if (bank > GPIO_MAX_BANK) {
		printk("brd_gpio_dir  args error!\n");
		return -1;
	}

	reg = GPIO_DIR(bank);
	
	switch (dir){
	case GPIO_DIR_INPUT:
		HW_REG(reg)  &= ~(1 << pin);
		break;
		
	case GPIO_DIR_OUTPUT:
		HW_REG(reg)  |= (1 << pin);
		break;
		
	default:
		break;
	}

	return 0;

}

/*
 * Set GPIO output level
 * val: 0/1
 */
int brd_gpio_wrt(int bank, int pin, enum gpio_level val)
{
	int reg = 0;
	if (bank > GPIO_MAX_BANK || (pin > 7) ||(val != 0 && val != 1)) {
		printk("brd_gpio_wrt args error!\
			bank=0x%x,ping = %x,val=0x%x\n",bank,pin,val);
		return -1;
	}

	brd_gpio_dir(bank,pin,GPIO_DIR_OUTPUT);

	reg = GPIO_DATA(bank,pin);

	switch (val){
	case GPIO_LEVEL_LOW:
		HW_REG(reg)  &= ~(1 << pin);
		break;
		
	case GPIO_LEVEL_HIGH:
		HW_REG(reg)  |= (1 << pin);
		break;
		
	default:
		break;
	}

	return 0;
}

/*
 * get GPIO input level
 * return: 0/1
 */
int brd_gpio_rd(int bank, int pin)
{
	int reg = 0,data ,pinmask = (1 << pin);
	if (bank > GPIO_MAX_BANK || (pin > 7)) {
		printk("brd_gpio_rd  args error!bank=0x%x,val=0x%x\n",bank,pin);
		return -1;
	}

	reg = GPIO_DATA(bank,pin);
	data = HW_REG(reg) ;

	if( (data & pinmask) == pinmask ) 
		return GPIO_LEVEL_HIGH;
	else
		return GPIO_LEVEL_LOW;

}

/*
 * set GPIO limited output level
 * val: 0/1
 */
int brd_gpio_set(enum gpio_pin gpio, enum gpio_level val)
{
	brd_gpio_wrt(gpio_cfgs[gpio].bank, gpio_cfgs[gpio].pin, val);
	return 0;
}

/*
 * get GPIO limited input level
 * return: 0/1
 */
int brd_gpio_get(enum gpio_pin gpio)
{
	return brd_gpio_rd(gpio_cfgs[gpio].bank, gpio_cfgs[gpio].pin);
}

void gpio_pinmux_init(void)
{
	/* GPIO pinmux */
	HW_REG(MUX_CTRL_REG(69)) = 0; /* GPIO7_3 */
	HW_REG(MUX_CTRL_REG(66)) = 0; /* GPIO7_0 */
	HW_REG(MUX_CTRL_REG(70)) = 0; /* GPIO7_4 */
	HW_REG(MUX_CTRL_REG(67)) = 0; /* GPIO7_1 */
	HW_REG(MUX_CTRL_REG(68)) = 0; /* GPIO7_2 */
	HW_REG(MUX_CTRL_REG(78)) = 0; /* GPIO8_4 */
	HW_REG(MUX_CTRL_REG(71)) = 0; /* GPIO7_5 */
	HW_REG(MUX_CTRL_REG(79)) = 0; /* GPIO8_5 */
	HW_REG(MUX_CTRL_REG(77)) = 0; /* GPIO8_3 */
	HW_REG(MUX_CTRL_REG(113)) = 0; /* GPIO11_3 */
	HW_REG(MUX_CTRL_REG(114)) = 0; /* GPIO11_2 */
	HW_REG(MUX_CTRL_REG(67)) = 0; /* GPIOx_x */
	HW_REG(MUX_CTRL_REG(67)) = 0; /* GPIOx_x */
}

/*
 * Set GPIO to default mode, must be called before other function
 */
int brd_gpio_init(void)
{
	int max = 0;
	struct gpio_cfg *cfg_p = &gpio_cfgs[0];

	gpio_pinmux_init();

	while (cfg_p->bank != GPIO_CFG_END_SIGN) {
		brd_gpio_dir(cfg_p->bank, cfg_p->pin, cfg_p->dir);

		if (cfg_p->dir == GPIO_DIR_OUTPUT) {
			brd_gpio_wrt(cfg_p->bank, cfg_p->pin, cfg_p->val);
		}
		brd_gpio_rd(cfg_p->bank, cfg_p->pin);

		cfg_p++;
		max++;
	}

	/* check gpio pin number */
	if ((int)GPIO_MAX != max) {
		GPIO_ERR("!!!Gpio define err, please check code!\n");
	}
	
	/* release some chip from reset */
	msleep(5);
	//brd_gpio_set(GPIO_SENSOR_RST, GPIO_LEVEL_HIGH);
	//brd_gpio_set(GPIO_IRLED_EN, GPIO_LEVEL_HIGH);

	return 0;
}


