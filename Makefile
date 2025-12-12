CC:=gcc

CFLAGS:=-g -O0 -Wall -std=c17 -fsanitize=undefined -fsanitize=address
LDFLAGS:=-lm
BUILD_MODE:=DEBUG

ifdef release
CFLAGS:=-O3 -Wall -std=c17 -DNDEBUG
BUILD_MODE:=RELEASE
endif

DAYS:=$(wildcard day*)
TARGETS:=$(DAYS:%=build/%)

all: $(TARGETS)

build/%: %/main.c | build
	$(CC) $(CFLAGS) -MMD -MP $< -o $@ $(LDFLAGS)

$(DAYS): %: build/%

-include $(OBJS:.o=.d)

build:
	mkdir -p $@

clean:
	rm -rf build

.PHONY: all clean $(DAYS)

