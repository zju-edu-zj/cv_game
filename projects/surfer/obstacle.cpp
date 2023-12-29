#include "obstacle.h"

Obstacle::Obstacle() {
    // Vertices representing a cube
    std::vector<Vertex> vertices = {
        // Front
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        // Back
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}}
    };

    // Indices for drawing triangles
    std::vector<uint32_t> indices = {
        // Front
        0, 1, 2,
        2, 3, 0,

        // Right
        1, 5, 6,
        6, 2, 1,

        // Back
        7, 6, 5,
        5, 4, 7,

        // Left
        4, 0, 3,
        3, 7, 4,

        // Bottom
        4, 5, 1,
        1, 0, 4,

        // Top
        3, 2, 6,
        6, 7, 3
    };

    // Set vertices and indices
    _vertices = vertices;
    _indices = indices;

    computeBoundingBox();
    initGLResources();
    initBoxGLResources();
    this->transform = transform;
}

Obstacle::Obstacle(Obstacle&& rhs):Model(std::move(rhs)){
    transform = rhs.transform; //just copy 
    initGLResources(); //gl resources have been freed 

    initBoxGLResources();
}

Obstacle& Obstacle::operator=(Obstacle&& rhs) {
    if (this != &rhs) {
        Model::operator=(std::move(rhs)); // 移动赋值基类部分

        transform = std::move(rhs.transform); // 移动赋值 transform
        // 对于其他成员变量进行移动赋值操作（如果有）

        // 重新初始化 OpenGL 资源
        initGLResources(); // 例如，重新初始化 _vao, _vbo, _ebo 等 OpenGL 资源
        initBoxGLResources(); // 初始化 _boxVao, _boxVbo, _boxEbo 等
    }
    return *this;
}
