CC := gcc
CFLAGS := -Wall -Wextra
EXECUTABLE := ./build/KewlEditor
SRC_FILES := ./src/main.c \
			 ./src/linux/LinuxMain.c \
			 ./src/OS.c \
			 ./src/TextBox.c \
			 ./src/TextLine.c \
			 ./UTF8String/UTFString.c \


all :
	# make build directory
	mkdir -p ./build
	# copy font
	cp ./NotoSansKR-Medium.otf ./build/

	$(CC) $(CFLAGS) -o ./build/KewlEditor $(SRC_FILES) -I./src/linux/ -I./src/ -I./UTF8String/ -lSDL2 -lSDL2_ttf -lX11
