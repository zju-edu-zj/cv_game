#pragma once
#include <glm/vec3.hpp>
#include "../base/model.h"
#include "../base/vertex.h"

#include <vector>
#include <cmath>

const std::vector<Vertex> sphereVertices = {
    // X      Y      Z
    {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}},

    // Bottom
    {{0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.0f}},

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

// Sphere index data
const std::vector<uint32_t> sphereIndices = {
    // Top cap
        0, 1, 2,
        2, 3, 0,

        // Bottom cap
        1, 5, 6,
        6, 2, 1,

        // Front face
        3, 2, 6,
        6, 7, 3,

        // Back face
        7, 6, 5,
        5, 4, 7,

        // Left face
        4, 0, 3,
        3, 7, 4,

        // Right face
        1, 5, 4,
        4, 0, 1
};

class Obstacle : public Model {
public:
    Obstacle();
    Obstacle(int shape);
    Obstacle(Obstacle&& rhs);
    Obstacle& operator=(Obstacle&& rhs);
    //Obstacle(const Obstacle&) = delete; // 禁用拷贝构造函数
    int _shape = 0;
    float _shapeInfo = 0;
    bool operator<(const Obstacle& other) const {
        return _vao < other._vao; // 或者使用其他逻辑来定义比较规则
    }
private:
    void createSphere(float radius, int sectors, int stacks);
    void createCylinder(float radius, float height, int sectors);
    void createCone(float radius, float height, int sectors);
    void createPrism(float radius, float height, int sides);
    void createFrustum(float radiusTop, float radiusBottom, float height, int sectors);
};
