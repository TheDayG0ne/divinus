SRC = $(shell find ./ -name '*.c')
OBJ = ../divinus
GIT_REV := $(shell git rev-parse --short HEAD)$(shell git diff --quiet HEAD || echo "-mod")

.PHONY: clean divinus
divinus: $(OBJ)

$(OBJ): $(SRC:%.c=%.o)
	$(CC) $^ -rdynamic $(OPT) -DGIT_REV=\"$(GIT_REV)\" $(FFMPEG_LIBS) -o $@

uvc_encoder.o: OPT += $(FFMPEG_CFLAGS) -DUVC_HAVE_FFMPEG

%.o: %.c Makefile
	$(CC) -c $< $(OPT) -DGIT_REV=\"$(GIT_REV)\" -o $@

clean:
	rm -rf $(SRC:%.c=%.o) $(OBJ)
