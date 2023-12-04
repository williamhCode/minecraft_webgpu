.PHONY: build

BACKEND = dawn
# TYPE = debug
TYPE = release

build:
	cmake --build build/$(BACKEND)/$(TYPE)
	cp build/$(BACKEND)/$(TYPE)/compile_commands.json .

build-tint:
	cmake --build build/$(BACKEND)/$(TYPE) --target tint
	cp build/$(BACKEND)/$(TYPE)/_deps/dawn-build/tint .

build-setup:
	cmake . -B build/dawn/debug -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja
	cmake . -B build/dawn/release -DCMAKE_BUILD_TYPE=Release -GNinja

xcode-setup:
	cmake . -B xcode -GXcode

run:
	build/$(BACKEND)/$(TYPE)/App
