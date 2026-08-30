Q ?= @
CC = arm-none-eabi-gcc
NWLINK = npx --yes -- nwlink@0.0.19
LINK_GC = 1
LTO = 1

CFLAGS += -O3 -ffast-math -DNDEBUG
CFLAGS += $(shell $(NWLINK) eadk-cflags-device)
LDFLAGS = -Wl,--relocatable
LDFLAGS += -nostartfiles
LDFLAGS += --specs=nano.specs

ifeq ($(LINK_GC),1)
CFLAGS += -fdata-sections -ffunction-sections
LDFLAGS += -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level
LDFLAGS += -Wl,--gc-sections
endif

ifeq ($(LTO),1)
CFLAGS += -flto -fno-fat-lto-objects
CFLAGS += -fwhole-program
CFLAGS += -fvisibility=internal
LDFLAGS += -flinker-output=nolto-rel
endif

CFLAGS += -Isrc

objs = $(addprefix output/, \
  main.o \
  icon.o \
)

.PHONY: build
build: output/hue.nwa

output/hue.nwa: $(objs)
	@echo "LD      $@"
	$Q $(CC) $(CFLAGS) $(LDFLAGS) $^ -lm -o $@

output/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "CC      $^"
	$Q $(CC) $(CFLAGS) -c $^ -o $@

output/icon.o: src/icon.png
	@echo "ICON    $<"
	$(Q) $(NWLINK) png-icon-o $< $@

.PHONY: clean
clean:
	@echo "CLEAN"
	$Q rm -rf output
