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
- [ ] Send Array of Materials to Shader
	- [x] Send Array of Textures to Shader
- [ ] Default models
	- [ ] Plane
	- [ ] Cube
	- [ ] Sphere
- [ ] Lighting

## Small problems to remember to solve

- [ ] Sort
	- [ ] Objects per pipeline
	- [ ] Meshes per material
- [ ] Add material instance per object and bind if from there to have customized properties of the same material
- [ ] How to handle empty scene? (Empty buffers) 
	
## Future Work (not in order)

- [ ] glTF
- [ ] Render Graph
- [ ] Ray Tracer
	- [ ] Compute Shaders
- [ ] Scene selection
