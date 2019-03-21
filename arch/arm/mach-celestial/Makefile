#
# Makefile for the linux kernel.
# 
#

obj-y					:= io.o irq.o  uart.o 
obj-$(CONFIG_MACH_CELESTIAL_CSM1201) += time.o csm1201.o
obj-$(CONFIG_MACH_CELESTIAL_CNC1800H) +=  time.o spi.o sata.o usb.o cnc1800h.o cnc1800h_mux.o nand.o csm1800_power_clk.o pcmcia.o devices-i2c.o
obj-$(CONFIG_MACH_CELESTIAL_CNC1800L) +=  cnc1800l_time.o spi.o usb.o cnc1800l.o nand.o cnc1800l_mux.o pcmcia.o mshci.o devices-i2c.o  cnc1800l_power_clk.o
