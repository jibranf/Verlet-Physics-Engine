# Verlet Physics Engine

The Verlet Physics Engine is a personal project that's inspired by one of my favorite past times of watching random programming, math, and engineering related YouTube videos in my free time. I was inspired when I saw two very cool videos by Pezza's Work ([video here](https://www.youtube.com/watch?v=9IULfQH7E90)) and Gradience ([video here](https://www.youtube.com/watch?v=NorXFOobehY)) on YouTube covering their development of two and three dimensional physics engines using Verlet Integration. I found the simplicty of the math behind the physics to be fascinating, and I thought they had really amazing results, so I chose to blend their approaches together and create my own version of a physics engine capable of performing 2D particle simulations. 

This project is written purely in C and is still in progress. The latest version of this code contains a naive implementation of collision detection and simple rendering, allowing for about ~400 particles to spawn at a simulation rate of 60 FPS. I'm currently working on adding optimizations to this code to allow it to become more performant and spawn tens of thousands of particles.

## Future Optimizations
- [ ] Use Instance Rendering to draw particles with the GPU
- [ ] Overhaul naive collision detection algorithm and replace w/ Uniform Grid Spatial Partitioning
- [ ] Multithreading

### References
* [Michael Richardson's implementation](https://github.com/marichardson137/VerletIntegration) (@Gradience on YT) of a 3D Verlet Physics engine
* Mathc library by [Felipe Ferreira da Silva](https://github.com/felselva/mathc).
* [Jean Tampon's implementation](https://github.com/johnBuffer/VerletSFML-Multithread) (@Pezza's Work on YT) of a 2D Verlet Physics engine in C++
* OpenGL, GLEW, GLFW for window creation, management, and rendering graphics

  
