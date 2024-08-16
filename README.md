# FPtest
This is an older project I made in Visual Studio 2022. These are the source files, solution should not be necessary (this could be built outside of VS, I'm just not sure how 😅).
<br/><br/>
This uses SDL2 version 2.26.5.

### FPtest
This folder contains the original source heavily based on @jdah's [Doomenstein 3D](https://github.com/jdah/doomenstein-3d). It is designed to spawn a window where WASD are used to move and the left and right arrow keys are used to pan the camera back and forth. A level can be loaded from a text file describing where wall vertices begin/end. I still need to return to this, update any stupid errors, and add more features (such as a wall sorting algorithm, as they are currently rendered in a hard-coded order. Looking at them from different angles is weird).
### FPtestGL
This folder, on the other hand, contains the very beginning of an OpenGL project that I started a while ago. If I recall correctly, all it does is display a triangle with a color gradient on a solid background. Originally, I was going to port FPtest to OpenGL (following in the footsteps of @jdah), but this has not come to fruition yet.
