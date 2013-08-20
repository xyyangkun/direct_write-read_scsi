export EMBED=1
export CC		=/opt/hisi-linux/x86-arm/arm-hisiv200-linux/target/bin/arm-hisiv200-linux-gcc
export CPP		=/opt/hisi-linux/x86-arm/arm-hisiv200-linux/target/bin/arm-hisiv200-linux-g++
export ROOTDIR = $(PWD)
export EXEC=read_disk_to_live555
export INCLUDE += -I$(ROOTDIR)/include -I$(ROOTDIR)/extern_src -I$(ROOTDIR)/driver_include
export LIBS = -L$(ROOTDIR)/lib -lsgutils2
export CFLAGS = $(INCLUDE) $(LIBS)



all:
	cd $(ROOTDIR)/extern_src/gtipcs	;make
	cd $(ROOTDIR)/extern_src/avilib	;make
	cd src							;make


	
clean:
	rm -f *.o
	cd src							;make clean
	cd $(ROOTDIR)/extern_src/gtipcs	;make clean
	cd $(ROOTDIR)/extern_src/avilib	;make clean