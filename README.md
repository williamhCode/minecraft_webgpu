# Minecraft WebGPU
Minecraft clone in C++ and WebGPU just for fun!  

2023/10/10:  

#### Features:  
Block placing/destroying, SSAO, simple terrain gen, global shadows, translucent water, imgui-integration

#### Controls:  
left click - destroy block  
right click - place block  
numbers - switch block  
g - show framerate + graph  
esc - show options

Tested on macOS (m1).

<img width="1312" alt="Terrain with Shadows" src="https://github.com/williamhCode/minecraft_webgpu/assets/83525937/c0eb237b-8d90-4bde-80d5-88f9781bd0cd">
<img width="1312" alt="Shadows Working" src="https://github.com/williamhCode/minecraft_webgpu/assets/83525937/a6d144d9-dc09-418c-9736-1f9da897b2c5">

2023/12/21:  
normal looking trees added yay
<img width="1312" alt="normal looking trees" src="https://github.com/williamhCode/minecraft_webgpu/assets/83525937/83551ace-7523-4f86-9339-69b59c2a0bb5">

## Build/Run Instructions
```
git clone https://github.com/williamhCode/minecraft_webgpu.git --recurse-submodules
cd minecraft_webgpu
make build-setup
make build
make run
```
