CC?=gcc
OBJ_FLAGS=-c
LIB_FLAGS=
RM=
DEBUG_FLAGS=-Ithird_party -g -Wall -Wextra -mavx2 -o
RELEASE_FLAGS=-Ithird_party -mavx2 -o

ifeq ($(OS), Windows_NT)
RM=del /Q /F
else
LIB_FLAGS += -lm
RM=rm -f
endif

debug: image_process_debug

release: image_process

image_process_debug: main.c
	$(CC) $(DEBUG_FLAGS) $@ $< $(LIB_FLAGS) 

image_process: main.c
	$(CC) $(RELEASE_FLAGS) $@ $< $(LIB_FLAGS) 

clean:
	$(RM) image_process*
	$(RM) brighten*.ppm
	$(RM) invert*.ppm
	$(RM) greyscale*.ppm
