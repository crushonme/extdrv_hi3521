
ifeq ($(PARAM_FILE), )
                PARAM_FILE:=../../Makefile.param
                include $(PARAM_FILE)
endif
			
obj-m := gpiocontrol.o
gpiocontrol-y += gpio.o

default:	
	@make -C $(LINUX_ROOT) M=$(PWD) modules
clean:
	@rm *.o -rf
	@make -C $(LINUX_ROOT) M=$(PWD) clean
