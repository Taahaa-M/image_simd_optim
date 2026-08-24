# OS?=WIN
OS?=LINUX

CC?=gcc
OBJ_FLAGS=-c
LIB_FLAGS=-Ithird_party
DEBUG_FLAGS=-g -Wall -mavx2 -o
RELEASE_FLAGS=-mavx2 -o

debug: image_process_debug.o image_process_debug

release: image_process

image_process_debug: main.c image_process_debug.o
	$(CC) $(LIB_FLAGS) $(DEBUG_FLAGS) image_process_debug image_process_debug.o

image_process_debug.o:
	$(CC) $(LIB_FLAGS) $(OBJ_FLAGS) $(DEBUG_FLAGS) image_process_debug.o main.c

image_process: main.c
	$(CC) $(LIB_FLAGS) $(RELEASE_FLAGS) image_process main.c

clean:
	ifneq($(OS), WIN)
		rm -f image_process.exe
		rm -f image_process_debug.exe
		rm -f image_process_debug.o
		rm -f brighten*.ppm
		rm -f invert*.ppm
		rm -f greyscale*.ppm
	else
		del /Q /F image_process.exe
		del /Q /F image_process_debug.exe
		del /Q /F image_process_debug.o
		del /Q /F brighten*.ppm
		del /Q /F invert*.ppm
		del /Q /F greyscale*.ppm
	endif
