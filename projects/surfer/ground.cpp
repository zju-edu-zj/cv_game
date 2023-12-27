#include "ground.h"

Ground::Ground(float width, float length) {
    _width = width;
    _length = length;
    generateGround();
    computeBoundingBox();
    initGLResources();
    initBoxGLResources();
}

void Ground::generateGround() {
    // 定义地面的四个顶点坐标
    std::vector<Vertex> vertices = {
        {{-_width / 2.0f, 0.0f, _length / 2.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-_width / 2.0f, 0.0f, -_length / 2.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{_width / 2.0f, 0.0f, _length / 2.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{_width / 2.0f, 0.0f, -_length / 2.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}
    };

    // 定义地面的两个三角形（组成矩形）
    std::vector<uint32_t> indices = {0, 1, 2, 2, 1, 3};

    // 将顶点和索引设置给父类的成员变量
    _vertices = vertices;
    _indices = indices;
}
