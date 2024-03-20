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
- [ ] Multisampling
- [ ] Sort
	- [ ] Objects per pipeline
	- [ ] Meshes per material
- [ ] Add normal to the vertices
- [ ] Default models
	- [ ] Plane
	- [ ] Cube
	- [ ] Sphere
- [ ] Lighting

## Small problems to remember to solve

- [ ] Add material instance per object and bind if from there to have customized properties of the same material
	
## Future Work (not in order)

- [ ] Compute Shaders
- [ ] GPU Driven Rendering
- [ ] glTF
- [ ] Ray Tracer
- [ ] Render Graph
- [ ] Scene selection
