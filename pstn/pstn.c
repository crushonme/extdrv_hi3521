/* 
 *
 * Author:Robin
 * All right reserved.
 * Created: 20.9.2014
 *
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/poll.h>

#include <mach/hardware.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>

#include "pstn.h"
static int open_cnt = 0;
int pstn_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int val;
	
	char device_addr, reg_addr;
	short reg_val;
	
	
	switch(cmd)
	{
		case FXO_SET_HANGUP:
			break;
		case FXO_SET_PICKUP2CALL:
			break;
		case FXO_SET_TALK:
			break;
		case FXO_SET_RCV_RING:
			break;
		case FXO_EN_REMOTE_SPEAK:
			break;
		case FXO_DIS_REMOTE_SPEAK:
			break;
		case FXO_EN_REMOTE_LISTEN:
			break;
		case FXO_DIS_REMOTE_LISTEN:
			break;
		case FXO_GET_STATE:
			break;
		default:
			return -1;
	}
    return 0;
}
static int pstn_monitor_task(void *ptr)
{

}
int pstn_open(struct inode * inode, struct file * file)
{
	pid_t   tPid;
	tPid = kernel_thread(pstn_monitor_task, NULL, CLONE_FS|CLONE_FILES);
    if (tPid < 0) {
        return -1;
    }
	if(0 == open_cnt++)
		return 0;    	
	return -1 ;

}
int pstn_close(struct inode * inode, struct file * file)
{
	open_cnt--;
    return 0;
}


static struct file_operations pstn_fops = {
    .owner      = THIS_MODULE,
    //.ioctl      = pstn_ioctl,
    .unlocked_ioctl = pstn_ioctl,
    .open       = pstn_open,
    .release    = pstn_close
};


static struct miscdevice pstn_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "pstn",
   .fops  = &pstn_fops,
};

static int __init pstn_init(void)
{
    int ret;
    //unsigned int reg;
    
    ret = misc_register(&pstn_dev);
    if(0 != ret)
    	return -1;
        
    return 0;    
}

static void __exit pstn_exit(void)
{
    misc_deregister(&pstn_dev);
}

module_init(pstn_init);
module_exit(pstn_exit);
MODULE_LICENSE("GPL");

