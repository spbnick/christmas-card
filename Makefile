CCPFX=arm-none-eabi-

TARGET_CFLAGS = -mcpu=cortex-m3 -mthumb
COMMON_CFLAGS = $(TARGET_CFLAGS) -Wall -Wextra -Werror -g3
LIBS = -lstammer

# In order of symbol resolution
MODS = \
    leds \
    anim_fx_script \
    anim_fx \
    anim \
    card

OBJS = $(addsuffix .o, $(MODS))
DEPS = $(OBJS:.o=.d)

.PHONY: clean

all: card.bin

%.o: %.c
	$(CCPFX)gcc $(COMMON_CFLAGS) $(CFLAGS) -c -o $@ $<
	$(CCPFX)gcc $(COMMON_CFLAGS) $(CFLAGS) -MM $< > $*.d

%.bin: %.elf
	$(CCPFX)objcopy -O binary $< $@

card.elf: $(OBJS) $(LDSCRIPTS)
	$(CCPFX)ld $(LDFLAGS) -T libstammer.ld -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f card.elf
	rm -f card.bin
