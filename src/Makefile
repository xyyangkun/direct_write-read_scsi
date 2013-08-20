

all:$(EXEC)
	
.cpp.o:
	$(CPP) -c $(CFLAGS) $<
	
OBJS = direct_io_sda.o $(EXEC).o yearBlock.o blocks.o dayBlock.o secBlock.o
direct_io_sda.o:direct_io_sda.cpp blocks.h
blocks.o:		blocks.cpp blocks.h
yearBlock.o:	yearBlock.cpp yearBlock.h  blocks.h
dayBlock.o:		dayBlock.cpp dayBlock.h
secBlock.o:		secBlock.cpp secBlock.h
$(EXEC).o:		$(EXEC).cpp

$(EXEC): $(OBJS)
	$(CPP) -o $(EXEC) $(OBJS) $(CFLAGS)
	cp $(EXEC) /mnt/yk
	
clean:
	rm -f *.o
	rm $(EXEC)
