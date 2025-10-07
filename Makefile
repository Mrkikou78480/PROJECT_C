# Simple Makefile for PROJECT_C (Windows MSYS2/mingw64)
CC = gcc
CFLAGS = -Wall -g -O2 `pkg-config --cflags gtk4`
LDFLAGS = `pkg-config --libs gtk4` -lsqlite3
SRCDIR = .
GTKDIR = gtk
COREDIR = core
FUNCTIONDIR = gtk/function

SOURCES = $(wildcard $(SRCDIR)/*.c) \
		  $(wildcard $(GTKDIR)/*.c) \
		  $(wildcard $(COREDIR)/*.c) \
		  $(wildcard $(FUNCTIONDIR)/*.c)

# Native host is built as a separate executable and must be excluded from the
# main GUI binary (it contains its own main()). Define it explicitly so we can
# filter it out from the app sources.
NATIVE_HOST_SRC = $(COREDIR)/native_host.c

# Application sources: all sources except the native host
APP_SOURCES = $(filter-out $(NATIVE_HOST_SRC), $(SOURCES))

APP_OBJECTS = $(APP_SOURCES:.c=.o)
NATIVE_HOST_OBJ = $(NATIVE_HOST_SRC:.c=.o)

OBJECTS = $(SOURCES:.c=.o)

TARGET = mon_app.exe

.PHONY: all clean rebuild native_host

.PHONY: all clean rebuild

all: $(TARGET) native_host


$(TARGET): $(APP_OBJECTS)
	$(CC) -mwindows -o $@ $^ $(LDFLAGS)

# Build native host executable (no GTK flags)
native_host: $(NATIVE_HOST_OBJ)
	$(CC) -o $(COREDIR)/native_host.exe $(NATIVE_HOST_OBJ) -lsqlite3

# Pattern rule for normal compilation (application files with GTK flags)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile the native_host.c without GTK flags (avoid pkg-config gtk flags)
$(NATIVE_HOST_OBJ): $(NATIVE_HOST_SRC)
	$(CC) -Wall -g -O2 -c $< -o $@

clean:
	rm -f $(APP_OBJECTS) $(NATIVE_HOST_OBJ) $(TARGET) $(COREDIR)/native_host.exe

rebuild: clean all
