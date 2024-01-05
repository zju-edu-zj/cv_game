#include "obstacle.h"

const std::vector<Vertex> vertices = {
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
const std::vector<uint32_t> indices = {
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

Obstacle::Obstacle() {
    // Vertices representing a cube
    

    // Set vertices and indices
    _vertices = vertices;
    _indices = indices;

    computeBoundingBox();
    initGLResources();
    initBoxGLResources();
    this->transform = transform;
}

Obstacle::Obstacle(int shape):_shape(shape){
    switch (shape)
    {
        case 0:
            _vertices = vertices;
            _indices = indices;
            break;
        case 1:
            createSphere(0.6, 100, 50);
            _shapeInfo = 0.6; //record radius
            break; 
        case 2:
            createCylinder(0.7, 1.2, 50);
            break;
        case 3:
            createCylinder(0.7, 1.2, 50);
            break;
        case 4:
            createPrism(0.7, 1.3, 10);
            break;
        case 5:
            createFrustum(0.3, 0.7, 1.3, 50);
            break;
        default:
            break;
    }
    computeBoundingBox();
    initGLResources();
    initBoxGLResources();
    this->transform = transform;
}

Obstacle::Obstacle(Obstacle&& rhs):Model(std::move(rhs)){
    transform = rhs.transform; //just copy 
    _shape = rhs._shape;
    initGLResources(); //gl resources have been freed 

    initBoxGLResources();
}

Obstacle& Obstacle::operator=(Obstacle&& rhs) {
    if (this != &rhs) {
        Model::operator=(std::move(rhs)); // 移动赋值基类部分

        _shape = rhs._shape;
        transform = std::move(rhs.transform); // 移动赋值 transform
        // 对于其他成员变量进行移动赋值操作（如果有）

        // 重新初始化 OpenGL 资源
        initGLResources(); // 例如，重新初始化 _vao, _vbo, _ebo 等 OpenGL 资源
        initBoxGLResources(); // 初始化 _boxVao, _boxVbo, _boxEbo 等
    }
    return *this;
}


// Function to generate sphere vertices and indices
void Obstacle::createSphere(float radius, int sectors, int stacks) {
    int vertexCount = (sectors + 1) * (stacks + 1);
    int indexCount = sectors * stacks * 6;

    int vIndex = 0, iIndex = 0;
    Vertex vertex;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = static_cast<float>(M_PI / 2 - i * M_PI / stacks);
        float y = radius * sin(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = static_cast<float>(2 * M_PI * j / sectors);
            float x = radius * cos(stackAngle) * cos(sectorAngle);
            float z = radius * cos(stackAngle) * sin(sectorAngle);

            glm::vec3 position(x, y, z);
            glm::vec3 normal = glm::normalize(position);
            glm::vec2 texCoord(static_cast<float>(j) / sectors, static_cast<float>(i) / stacks);

            Vertex vertex(position, normal, texCoord);
            _vertices.push_back(vertex);

            if (i < stacks && j < sectors) {
                int currentRow = i * (sectors + 1);
                int nextRow = (i + 1) * (sectors + 1);

                _indices.push_back(currentRow + j);
                _indices.push_back(nextRow + j);
                _indices.push_back(currentRow + j + 1);

                _indices.push_back(currentRow + j + 1);
                _indices.push_back(nextRow + j);
                _indices.push_back(nextRow + j + 1);
            }
        }
    }
}

void Obstacle::createCylinder(float radius, float height, int sectors) {
    float sectorStep = 2.0f * M_PI / sectors;

    // Generate cylinder vertices
    for (int i = 0; i <= sectors; ++i) {
        float angle = i * sectorStep;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        glm::vec3 positionTop(x, height / 2, z);
        glm::vec3 positionBottom(x, -height / 2, z);
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        glm::vec2 texCoord(static_cast<float>(i) / sectors, 0.0f);

        _vertices.emplace_back(positionTop, normal, texCoord);
        _vertices.emplace_back(positionBottom, normal, texCoord);
    }

    // Generate cylinder indices
    for (int i = 0; i < sectors; ++i) {
        int nextVertex = (i + 1) % sectors;

        // Top face
        _indices.push_back(i * 2);
        _indices.push_back(nextVertex * 2);
        _indices.push_back(i * 2 + 1);

        // Bottom face
        _indices.push_back(nextVertex * 2 + 1);
        _indices.push_back(nextVertex * 2);
        _indices.push_back(i * 2 + 1);
    }
}

void Obstacle::createCone(float radius, float height, int sectors) {
    float sectorStep = 2.0f * M_PI / sectors;

    // Apex vertex
    glm::vec3 apex(0.0f, height / 2, 0.0f);

    // Generate cone vertices
    for (int i = 0; i <= sectors; ++i) {
        float angle = i * sectorStep;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        glm::vec3 position(x, -height / 2, z);
        glm::vec3 normal = glm::normalize(glm::cross(glm::vec3(0, -1, 0), glm::vec3(x, 0, z)));

        glm::vec2 texCoord(static_cast<float>(i) / sectors, 0.0f);

        _vertices.emplace_back(position, normal, texCoord);
    }

    // Generate cone indices
    for (int i = 0; i < sectors; ++i) {
        int nextVertex = (i + 1) % sectors;

        // Base of the cone
        _indices.push_back(i);
        _indices.push_back(nextVertex);
        _indices.push_back(static_cast<uint32_t>(_vertices.size()) / 2);

        // Side faces
        _indices.push_back(i);
        _indices.push_back(nextVertex);
        _indices.push_back(static_cast<uint32_t>(_vertices.size()) / 2 + 1);
    }
}

void Obstacle::createPrism(float radius, float height, int sides) {
    float sectorStep = 2.0f * M_PI / sides;

    // Generate prism vertices
    for (int i = 0; i < sides; ++i) {
        float angle = i * sectorStep;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        glm::vec3 positionTop(x, height / 2, z);
        glm::vec3 positionBottom(x, -height / 2, z);
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        glm::vec2 texCoord(static_cast<float>(i) / sides, 0.0f);

        _vertices.emplace_back(positionTop, normal, texCoord);
        _vertices.emplace_back(positionBottom, normal, texCoord);
    }

    // Generate prism indices
    for (int i = 0; i < sides; ++i) {
        int nextVertex = (i + 1) % sides;

        // Top face
        _indices.push_back(i * 2);
        _indices.push_back(nextVertex * 2);
        _indices.push_back(i * 2 + 1);

        // Bottom face
        _indices.push_back(nextVertex * 2 + 1);
        _indices.push_back(nextVertex * 2);
        _indices.push_back(i * 2 + 1);

        // Side faces
        _indices.push_back(i * 2);
        _indices.push_back(nextVertex * 2);
        _indices.push_back(nextVertex * 2 + 1);

        _indices.push_back(nextVertex * 2 + 1);
        _indices.push_back(i * 2 + 1);
        _indices.push_back(i * 2);
    }
}

void Obstacle::createFrustum(float radiusTop, float radiusBottom, float height, int sectors) {
    float sectorStep = 2.0f * M_PI / sectors;

    // Generate frustum vertices
    for (int i = 0; i <= sectors; ++i) {
        float angle = i * sectorStep;
        float xTop = radiusTop * cos(angle);
        float zTop = radiusTop * sin(angle);
        float xBottom = radiusBottom * cos(angle);
        float zBottom = radiusBottom * sin(angle);

        glm::vec3 positionTop(xTop, height / 2, zTop);
        glm::vec3 positionBottom(xBottom, -height / 2, zBottom);
        glm::vec3 normal = glm::normalize(glm::cross(positionTop - positionBottom, glm::vec3(xTop, 0.0f, zTop)));

        glm::vec2 texCoord(static_cast<float>(i) / sectors, 0.0f);

        _vertices.emplace_back(positionTop, normal, texCoord);
        _vertices.emplace_back(positionBottom, normal, texCoord);
    }

    // Generate frustum indices
    for (int i = 0; i < sectors; ++i) {
        int nextVertex = (i + 1) % sectors;

        // Top face
        _indices.push_back(i * 2);
        _indices.push_back(nextVertex * 2);
        _indices.push_back(i * 2 + 1);

        // Bottom face
        _indices.push_back(nextVertex * 2 + 1);
        _indices.push_back(nextVertex * 2);
        _indices.push_back(i * 2 + 1);

        // Side faces
        _indices.push_back(i * 2);
        _indices.push_back(nextVertex * 2);
        _indices.push_back(nextVertex * 2 + 1);

        _indices.push_back(nextVertex * 2 + 1);
        _indices.push_back(i * 2 + 1);
        _indices.push_back(i * 2);
    }
}
