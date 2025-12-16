CC = gcc
SHELL = cmd.exe
TARGET = mon_app.exe

# Source files
SOURCES = main.c \
          gtk/main_gtk.c \
          gtk/ui.c \
          gtk/settings.c \
          gtk/generator.c \
          gtk/manager.c \
          core/password.c \
          core/db.c \
          core/config.c \
          crypto/sha256.c \
          gtk/auth_ui.c \
          core/auth.c \
          crypto/simplecrypt.c

# Objects
OBJECTS = $(SOURCES:.c=.o)

# Flags
CFLAGS = -Wall -g -O2 -Icrypto
GTK_CFLAGS = $(shell pkg-config --cflags gtk4)
GTK_LIBS = $(shell pkg-config --libs gtk4)
LDFLAGS = -mwindows $(GTK_LIBS) -lsqlite3 -ladvapi32

.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

clean:
	del /Q main.o gtk\*.o core\*.o crypto\*.o $(TARGET) 2>nul || exit 0

rebuild: clean all
