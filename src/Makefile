SRC = $(shell find ./ -name '*.c')
OBJ = ../divinus
GIT_REV := $(shell git rev-parse --short HEAD)$(shell git diff --quiet HEAD || echo "-mod")

.PHONY: clean divinus
divinus: $(OBJ)

$(OBJ): $(SRC:%.c=%.o)
	$(CC) $^ -rdynamic $(OPT) -DGIT_REV=\"$(GIT_REV)\" $(FFMPEG_LIBS) -o $@

ifdef UVC_FFMPEG
FFMPEG_CFLAGS := $(shell pkg-config --cflags libavcodec libavutil libswscale)
FFMPEG_LIBS := $(shell pkg-config --libs libavcodec libavutil libswscale)
hal/uvc_encoder.o: override OPT += $(FFMPEG_CFLAGS) -DUVC_HAVE_FFMPEG
endif

%.o: %.c Makefile
	$(CC) -c $< $(OPT) -DGIT_REV=\"$(GIT_REV)\" -o $@

clean:
	rm -rf $(SRC:%.c=%.o) $(OBJ)
