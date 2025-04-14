# CPPFLAGS += -DNO_PROTOTYPES=1 -DHZ=100
CPPFLAGS += -DNO_PROTOTYPES=1 -DHZ=100 -DREG=register
LDLIBS += -lm -lrt

CFLAGS += -O3 -static 
all: dhrystone

dhrystone: dhry_1.o dhry_2.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o dhrystone

.PHONY: all clean
