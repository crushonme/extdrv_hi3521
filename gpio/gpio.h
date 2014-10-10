
#ifndef _GPIO_H
#define _GPIO_H

#define GPIO_MAX_BANK  11

#define GPIO_BASE 0x20150000
#define GPIO_BASE_BANK(bank) (GPIO_BASE + ((bank) << 16))
#define GPIO_DIR(bank)              IO_ADDRESS(GPIO_BASE_BANK(bank) + 0x400)
#define GPIO_DATA(bank,index)       IO_ADDRESS(GPIO_BASE_BANK(bank)  + (0x4 << index))
#define HW_REG(reg)         *((volatile unsigned int *)(reg))

/*pinmux*/
#define MUX_CTRL_BASE 0x200f0000

#define MUX_CTRL_REG(x)       IO_ADDRESS(MUX_CTRL_BASE + x * 4)

#define GPIO_CFG_END_SIGN -1

/* GPIO Pin Index, must be the same order as struct gpio_cfg gpio_cfgs */
enum gpio_pin {
	GPIO_I2C_SEL,

	GPIO_EMB_SHK,
	GPIO_EMB_PD,
	GPIO_EMB_RM,
	GPIO_EMB_F_R,

	GPIO_EXT_LC,
	GPIO_EXT_RS,
	GPIO_EXT_LSC,
	GPIO_EXT_F_R,

	GPIO_SENSOR_PWM1,
	GPIO_SENSOR_PWM2,

	GPIO_LCD_PWM,
	GPIO_LCD_RST,

	GPIO_MAX          /* Gpio Max Num */
};

enum gpio_level
{
	GPIO_LEVEL_LOW,
	GPIO_LEVEL_HIGH,
	GPIO_LEVEL_MAX
};
enum gpio_dir
{
	GPIO_DIR_OUTPUT,
	GPIO_DIR_INPUT,
	GPIO_DIR_MAX
};
enum gpio_cmd
{
	GPIO_CMD_READ,
	GPIO_CMD_WRITE,
	GPIO_CMD_MAX
};

struct gpio_cfg {
	int 			bank; /* always be 0 for Amba */
	int 			pin;  /* 0 ~ max-1 */
	enum gpio_dir 	dir;  /* GPIO_HW_DIR_INPUT/GPIO_HW_DIR_OUTPUT */
	enum gpio_level val;  /* 0 = low level; 1 = high level */
};
int brd_gpio_dir(int bank, int pin, enum gpio_dir dir);
int brd_gpio_wrt(int bank, int pin, enum gpio_level val);
int brd_gpio_rd(int bank, int pin);

#endif

