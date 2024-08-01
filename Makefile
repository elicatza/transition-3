PROGRAMNAME := transition-3
CFLAGS := -Wall -Wextra -std=c99 -g
INCLUDES := -I./vendor/include
WEB_CFLAGS := -Wall -Wextra -Os
LIBS := $(INCLUDES) -L./vendor/lib -l:libraylib.so -lm
WEB_LIBS := $(INCLUDES) -L./vendor/lib/ -lraylib -lm

BUILD ?= RELEASE

ifeq ($(BUILD), DEBUG)
	CFLAGS := $(CFLAGS) -DDEBUG
endif


all: ./build/$(PROGRAMNAME).html ./build/$(PROGRAMNAME)

itch: ./build/$(PROGRAMNAME).html
	cp $^ build/index.html
	zip build/transition-3.zip build/index.html build/transition-3.js build/transition-3.wasm
	cd ./design_document && \
		htlatex ./main.tex

gdd:
	cd ./design_document && \
		pdflatex main.tex

./build/$(PROGRAMNAME).html: ./src/main.c ./build/puzzle_web.o ./build/core_web.o
	mkdir -p $(shell dirname $@)
	/usr/lib/emscripten/emcc -o $@ $^ $(WEB_CFLAGS) $(WEB_LIBS) -s USE_GLFW=3 --shell-file ./src/release.html -DPLATFORM_WEB

./build/puzzle_web.o: ./src/puzzle.c
	/usr/lib/emscripten/emcc -c -o $@ $^ $(WEB_CFLAGS) $(INCLUDES) -DPLATFORM_WEB

./build/core_web.o: ./src/core.c
	/usr/lib/emscripten/emcc -c -o $@ $^ $(WEB_CFLAGS) $(INCLUDES) -DPLATFORM_WEB

./build/$(PROGRAMNAME): ./src/main.c ./build/puzzle.o ./build/core.o
	mkdir -p $(shell dirname $@)
	cc -o $@ $^ $(CFLAGS) $(LIBS)

./build/puzzle.o: ./src/puzzle.c
	cc -c -o $@ $^ $(CFLAGS) $(INCLUDES)

./build/core.o: ./src/core.c
	cc -c -o $@ $^ $(CFLAGS) $(INCLUDES)

.PHONY: embed
embed: ./src/embed.c
	./assets/atlas.sh
	cc -o ./build/$@ $^ $(LIBS)
	./build/embed
