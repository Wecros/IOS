# @file Makefile
# @author Marek Filip (xfilip46), FIT BUT
# @date 2020-Apr-24
# @brief IOS-project-2
# @details Makefile for IOS project 2.
#		   Project only recompiles when files change.
#          Compiled: gcc 9.3

ZIP_FILE = proj2.zip

CC = gcc
CFLAGS  = -std=gnu99 -Wall -Wextra -pedantic -g
# CFLAGS  = -std=gnu99 -Wall -Wextra -Werror -pedantic -g # use this
# LDFLAGS = -lpthread -lrt
LDFLAGS = -pthread -lrt

PROGRAM = proj2

.PHONY: all pack clean

all: proj2

pack: $(ZIP_FILE)
$(ZIP_FILE): *.c *.h Makefile
	zip $@ $^

clean:
	@rm -rf *.o

	@echo Removed all nonessential files.
