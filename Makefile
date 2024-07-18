PROGRAMNAME := demo

all: ./build/$(PROGRAMNAME).html ./build/$(PROGRAMNAME)

.PHONY: assets
assets: ./src/embed.c
	cc -o build/embed $^ -std=c99 -Wall -Wextra -I./vendor/include -L./vendor/lib -l:libraylib.so
	./build/embed

./build/$(PROGRAMNAME).html: ./src/main.c
	mkdir -p $(shell dirname $@)
	/usr/lib/emscripten/emcc -o $@ $^ -Os -Wall -lraylib -lm -I./vendor/include/ -L./vendor/lib/ -s USE_GLFW=3 --shell-file ./src/release.html -DPLATFORM_WEB
	# /usr/lib/emscripten/emcc -o $@.html $^ -Os -Wall ./vendor/raylib/src/libraylib.a -I. -I./vendor/raylib/src/ -L. -L./vendor/raylib/src/ -s USE_GLFW=3 --shell-file /usr/lib/emscripten/src/shell.html -DPLATFORM_WEB

./build/$(PROGRAMNAME): ./src/main.c
	mkdir -p $(shell dirname $@)
	cc -o $@ $^ -L./vendor/lib -l:libraylib.so -I./vendor/include -lm
