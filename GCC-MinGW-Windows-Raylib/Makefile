# Makefile for owdraw_rl / drawtest_rl  (raylib port, GCC/MinGW on Windows)
# ===========================================================================
# Released April 2026 by RetroNick2020
#
# SETUP
# -----
# 1. Download the raylib MinGW-w64 package from:
#      https://github.com/raysan5/raylib/releases
#    (e.g. raylib-5.0_win64_mingw-w64.zip)
# 2. Unzip and place files:
#      raylib.h       -> ./include/raylib.h
#      libraylib.a    -> ./lib/libraylib.a
# 3. Run:  mingw32-make
#    or:   make          (if your MinGW puts 'make' on PATH)
#
# TARGETS
#   make              - build testrl.exe
#   make clean        - remove .exe and .o files
# ===========================================================================

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -I./include
# -lopengl32, -lgdi32, -lwinmm  are required by raylib on Windows
LDFLAGS = -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm

TARGET  = testrl.exe
OBJS    = testrl.o drawrl.o

# ---- default target ----
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

drawtest_rl.o: testrl.c drawrl.h
	$(CC) $(CFLAGS) -c testrl.c

owdraw_rl.o: drawrl.c drawrl.h
	$(CC) $(CFLAGS) -c drawrl.c

# ---- clean ----
clean:
	del /Q $(TARGET) $(OBJS) 2>nul || rm -f $(TARGET) $(OBJS)

.PHONY: all clean
