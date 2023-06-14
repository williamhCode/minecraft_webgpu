.PHONY: build

BACKEND = dawn
TYPE = debug

build:
	-cmake --build build/$(BACKEND)/$(TYPE)
	cp build/$(BACKEND)/$(TYPE)/compile_commands.json .

build-setup:
	cmake . -B build/dawn/debug -DCMAKE_BUILD_TYPE=Debug -GNinja
	cmake . -B build/dawn/release -DCMAKE_BUILD_TYPE=Release -GNinja

xcode-setup:
	cmake . -B xcode -GXcode

run:
	build/$(BACKEND)/$(TYPE)/App
