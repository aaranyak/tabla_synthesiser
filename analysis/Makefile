CC = gcc # Compiler
MAKE = make
CFLAGS = -g -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-deprecated-declarations -Wall -pthread -I/usr/include/gtk-3.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/at-spi-2.0 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/gtk-3.0 -I/usr/include/gio-unix-2.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/fribidi -I/usr/include/harfbuzz -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng16 -I/usr/include/x86_64-linux-gnu -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include # Compile Flags
LDFLAGS = -lm -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -lharfbuzz -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 # Linker FLags
NAME = analyser

OBJECT_FILES = audio.o load_file.o # All the object files I want to link

all: $(NAME) clean # Build tasks

$(NAME): main.c $(OBJECT_FILES) # Compile executable
	$(CC) -o $(NAME) -no-pie $(CFLAGS) main.c $(OBJECT_FILES) $(LDFLAGS) # Compile and link

audio.o: audio.c # File containing GUI
	$(CC) -c -o audio.o $(CFLAGS) audio.c

load_file.o: load_file.c # File containing GUI
	$(CC) -c -o load_file.o $(CFLAGS) load_file.c


.PHONY: clean # Not a real target
clean: # Cleanup tasks
	rm $(OBJECT_FILES) # Delete all this stuff

