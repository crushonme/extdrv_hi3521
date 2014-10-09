
ifeq ($(PARAM_FILE), )
        PARAM_FILE:=../../Makefile.param
        include $(PARAM_FILE)
endif
			
obj-m := ov7725.o
ov_7725-y += ov7725.o

EXTRA_CFLAGS += -D$(HI_FPGA) 
#EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c -I$(PWD)/../gpio-i2c1
EXTRA_CFLAGS += -I$(PWD)/../gpio_i2c_8b

default:
	@make -C $(LINUX_ROOT) M=$(PWD) modules 
#	cp ov7725.ko ../../../user 
clean: 
	@make -C $(LINUX_ROOT) M=$(PWD) clean 
#	@-rm ../../../user/ov7725.ko
