# Notes

- Every WGPU object has a descriptor
- Pass pointer of descriptor to function to create object

## Setting up

### Create Instance

- First start up by creating an `WGPUInstance`
```cpp
WGPUInstanceDescriptor desc = {};
desc.nextInChain = nullptr;
WGPUInstance instance = wgpuCreateInstance(&desc);
if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
return 1;
}
```

### Create Window

- Next, set up the window
```cpp
GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
if (!window) {
    std::cerr << "Could not open window!" << std::endl;
return 1;
}
```
