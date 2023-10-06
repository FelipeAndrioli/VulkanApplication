# Vulkan Application

As of now, this README is more to remember myself of what I'm currently working, to help me focus on the current work, 
to help me not lose track of what I'm doing, to help me remember the future work, and to set "features" for future work.

Once the engine is in a acceptable (ish) kind of state, I will change the whole README.

## Current Work

[X] - Render from Scene
[X] - Render more than one model from the scene
[X] - Change models from the scene
[X] - Render UI components from scene
[X] - Multiple objects rendering
	[X] - Change the graphics pipeline to support it
	[X] - Move the descriptor set layout back to graphics pipeline
	[X] - Move the descriptor pool back to the graphics pipeline
[X] - Fix Compute pipeline
[IN PROGRESS] - Make render/compute flow editable from scene
	[X] - Change scene virtual functions to model virtual functions
	[IN PROGRESS] - Update and Load UBOs from the model 
	[ ] - Load and set different shaders
	[ ] - Multiple flows from scene controlled by the graphics pipeline layout
		[ ] - Scene layout
		[ ] - Rasterization
		[ ] - Custom rendering
		[ ] - Compute
## Future Work (not in order)

[ ] - Ray Tracer??

## Bugs

[ ] - The updates happen faster when limiting frame rate
[ ] - The updates happen faster when mouse is moving on the screen
