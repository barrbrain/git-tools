default: all

LIBGIT2_PATH ?= $(HOME)/dev/libgit2

DEPS = $(shell PKG_CONFIG_PATH=$(LIBGIT2_PATH)/lib/pkgconfig pkg-config --cflags --libs libgit2)

#CFLAGS += $(DEPS)
CFLAGS += -g -O3 -I$(LIBGIT2_PATH)/include -I$(LIBGIT2_PATH)/src -L$(LIBGIT2_PATH) -lgit2

OBJECTS = \
  read-headers.o \
  object-info.o

LIBGIT2_INTERNALS = \
  $(LIBGIT2_PATH)/src/buffer.c \
  $(LIBGIT2_PATH)/src/util.c \
  $(LIBGIT2_PATH)/src/errors.c \
  $(LIBGIT2_PATH)/src/global.c

all: $(OBJECTS)
	gcc $(CFLAGS) -o object-info object-info.o $(LIBGIT2_INTERNALS)
	gcc $(CFLAGS) -o read-headers read-headers.o
