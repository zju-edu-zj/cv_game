## CG Projects

### Introduction
This is the assignments for the Computer Graphics course, through which you'll learn how to:
+ construct the simplest framework for a Computer Graphics program;
+ display geometry and pixel data on the screen;
+ use modern OpenGL API (3.3+);
+ use GLSL, a Shading Language compatible with OpenGL;

This will help you understanding:
+ the basic concepts in Computer Graphics:
    + geometry
    + lighting
    + texture
    + graphics pipleine etc;
+ how GPU works to display a frame on the screen;

Basically, I follow the [Learning OpenGL](https://learnopengl-cn.github.io/) tutorials and refactor the old version assignments in an object oriented way. The students should have some knowledge of modern C++. Don't be panic if you haven't learnt C++ yet. You'll write no more than 20 lines of code for most of the assignments.

You can find more materials in [Bilibili](https://space.bilibili.com/52683403/channel/collectiondetail?sid=749547&ctype=0)

### Get the repository
```shell
git clone --depth=1 https://github.com/syby119/CG-projects.git
cd CG-projects
git submodule init
git submodule update
```

### How to run

#### Preliminaries
+ CMake >= 3.20
+ C++ Compiler supports at least C++14

#### Build and Compile
```shell
cmake -Bbuild .
cmake --build build --config Release --parallel 8
```




