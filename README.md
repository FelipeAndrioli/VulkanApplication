# Vulkan Application

As of now, this README is more to remember myself of what I'm currently working, to help me focus on the current work, 
to help me not lose track of what I'm doing, to help me remember the future work, and to set "features" for future work.

Once the engine is in a acceptable (ish) kind of state, I will change the whole README.

## Current Work (not in order)

- [x] Depth Buffer 
- [x] Texture 
- [x] Camera class
	- [x] Movement 
	- [x] Turn Movement
	- [x] Based on Input
- [x] Custom Model loading
- [x] Proper Model/Material/Shader/Texture Set
- [x] Single Buffer for Vertices and Indices 
- [x] Single Buffer for uniforms
- [x] Empty Scene
- [x] Send Array of Materials to Shader 
	- [x] Send Array of Textures to Shader
	- [x] Send one array of textures/material and the index to be used per material
- [x] Mipmaps
- [x] Multisampling
- [x] Custom meshes I
	- [x] Individual Cells Plane 
	- [x] Unique Cells Plane 
	- [x] Cube
- [x] glTF
- [x] Cubemap/Skybox 
- [x] Lighting
	- [x] Directional Light
	- [x] Point Light
	- [x] Spot Light
	- [x] Blinn-Phong
- [x] Stencil Buffer
- [x] Runtime Shader Compilation (Partially)
	- To avoid adding another dependency (glslang or shaderc), I decided to use VulkanSDK glslang,
	  however, the SDK version is built using MSVC compiler, not being compatible with code built
	  using Ninja or MinGW compiler, therefore the runtime shader compilation will only work when
	  built using MSVC compiler.
- [x] Blending
- [x] Framebuffers

## Next Up
- [ ] Instancing
- [ ] Custom Anti Aliasing
- [ ] Refactoring to Graphics Device/Backend/Command Context design

## Planning 
- [ ] Generate Normals (Mesh Generator) 
- [ ] Compute Shaders
- [ ] GPU Driven Rendering
- [ ] Ray Tracer
- [ ] Render Graph
- [ ] Scene selection
- [ ] Custom meshes II 
	- [ ] Sphere
	- [ ] Voxel

## Small problems to remember to solve

- [ ] Add material instance per object and bind if from there to have customized properties of the same material

