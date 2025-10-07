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

OBJECTS = $(SOURCES:.c=.o)

TARGET = mon_app.exe

.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -mwindows -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean all
