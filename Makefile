.PHONY: build

TYPE = debug
BACKEND = wgpu

build:
	cmake --build build/$(BACKEND)/$(TYPE)
	cp build/$(BACKEND)/$(TYPE)/compile_commands.json .

build-setup:
	cmake . -B build/wgpu/debug -DWEBGPU_BACKEND=WGPU -DCMAKE_BUILD_TYPE=Debug -GNinja
	cmake . -B build/wgpu/release -DWEBGPU_BACKEND=WGPU -DCMAKE_BUILD_TYPE=Release -GNinja
	cmake . -B build/dawn/debug -DWEBGPU_BACKEND=DAWN -DCMAKE_BUILD_TYPE=Debug -GNinja
	cmake . -B build/dawn/release -DWEBGPU_BACKEND=DAWN -DCMAKE_BUILD_TYPE=Release -GNinja

run:
	build/$(BACKEND)/$(TYPE)/App
