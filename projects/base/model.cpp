#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <fstream>
#include <sstream>

#include <tiny_obj_loader.h>

#include "model.h"

Model::Model(const std::string& filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn, err;

    std::string::size_type index = filepath.find_last_of("/");
    std::string mtlBaseDir = filepath.substr(0, index + 1);

    if (!tinyobj::LoadObj(
            &attrib, &shapes, &materials, &warn, &err, filepath.c_str(), mtlBaseDir.c_str())) {
        throw std::runtime_error("load " + filepath + " failure: " + err);
    }

    if (!warn.empty()) {
        std::cerr << "Loading model " + filepath + " warnings: " << std::endl;
        std::cerr << warn << std::endl;
    }

    if (!err.empty()) {
        throw std::runtime_error("Loading model " + filepath + " error:\n" + err);
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position.x = attrib.vertices[3 * index.vertex_index + 0];
            vertex.position.y = attrib.vertices[3 * index.vertex_index + 1];
            vertex.position.z = attrib.vertices[3 * index.vertex_index + 2];

            if (index.normal_index >= 0) {
                vertex.normal.x = attrib.normals[3 * index.normal_index + 0];
                vertex.normal.y = attrib.normals[3 * index.normal_index + 1];
                vertex.normal.z = attrib.normals[3 * index.normal_index + 2];
            }

            if (index.texcoord_index >= 0) {
                vertex.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
                vertex.texCoord.y = attrib.texcoords[2 * index.texcoord_index + 1];
            }

            // check if the vertex appeared before to reduce redundant data
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    _vertices = vertices;
    _indices = indices;

    computeBoundingBox();

    initGLResources();

    initBoxGLResources();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup();
        throw std::runtime_error("OpenGL Error: " + std::to_string(error));
    }
}
Model::Model(const std::string& filename,bool myloader){
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "v") {
            glm::vec3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        } else if (type == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (type == "vt") {
            glm::vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        } else if (type == "f") {  //we should support both 4 and 3 vertices
            std::vector<std::string> vertexData;
            for (int i = 0; i < 4; ++i) {
                std::string vertexStr;
                iss >> vertexStr;
                if(!vertexStr.empty()){
                    vertexData.push_back(vertexStr);
                }
            }
            if (vertexData.size() == 4) {
                std::vector<uint32_t> indices; //record temporarily
                for (int i = 0; i < 4; ++i) {
                    std::istringstream vertexStream(vertexData[i]);
                    try
                    {
                        std::string indexStr;
                        std::getline(vertexStream, indexStr, '/');
                        uint32_t positionIndex = std::stoi(indexStr) - 1;
                        std::getline(vertexStream, indexStr, '/');
                        uint32_t texCoordIndex = std::stoi(indexStr) - 1;
                        std::getline(vertexStream, indexStr, '/');
                        uint32_t normalIndex = std::stoi(indexStr) - 1;
                        Vertex cur_vertex(positions[positionIndex], normals[normalIndex], texCoords[texCoordIndex]);
                        if (uniqueVertices.count(cur_vertex) == 0) {
                            uniqueVertices[cur_vertex] = static_cast<uint32_t>(_vertices.size());
                            _vertices.push_back(cur_vertex);
                        }
                        indices.push_back(uniqueVertices[cur_vertex]);
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << '\n';
                        for(auto &s:vertexData){
                            std::cout << s << ' ' << std::endl; //for debug
                        }
                    }
                }
                //now push into true indices
                _indices.push_back(indices[0]);
                _indices.push_back(indices[1]);
                _indices.push_back(indices[2]);
                //the next triangle
                _indices.push_back(indices[2]);
                _indices.push_back(indices[3]);
                _indices.push_back(indices[0]);
            } else if(vertexData.size()==3){ // 如果是三角形面
                for (int i = 0; i < 3; ++i) {
                    std::istringstream vertexStream(vertexData[i]);
                    std::string indexStr;
                    std::getline(vertexStream, indexStr, '/');
                    uint32_t positionIndex = std::stoi(indexStr) - 1;
                    std::getline(vertexStream, indexStr, '/');
                    uint32_t texCoordIndex = std::stoi(indexStr) - 1;
                    std::getline(vertexStream, indexStr, '/');
                    uint32_t normalIndex = std::stoi(indexStr) - 1;
                    Vertex cur_vertex(positions[positionIndex], normals[normalIndex], texCoords[texCoordIndex]);

                    if (uniqueVertices.count(cur_vertex) == 0) {
                        uniqueVertices[cur_vertex] = static_cast<uint32_t>(_vertices.size());
                        _vertices.push_back(cur_vertex);
                    }

                    _indices.push_back(uniqueVertices[cur_vertex]);
                }
            }
        }
    }
    file.close();

    computeBoundingBox();

    initGLResources();

    initBoxGLResources();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup();
        throw std::runtime_error("OpenGL Error: " + std::to_string(error));
    }

}
Model::Model(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : _vertices(vertices), _indices(indices) {

    computeBoundingBox();

    initGLResources();

    initBoxGLResources();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup();
        throw std::runtime_error("OpenGL Error: " + std::to_string(error));
    }
}

Model::Model(Model&& rhs) noexcept
    : _vertices(std::move(rhs._vertices)), _indices(std::move(rhs._indices)),
      _boundingBox(std::move(rhs._boundingBox)), _vao(rhs._vao), _vbo(rhs._vbo), _ebo(rhs._ebo),
      _boxVao(rhs._boxVao), _boxVbo(rhs._boxVbo), _boxEbo(rhs._boxEbo) {
    _vao = 0;
    _vbo = 0;
    _ebo = 0;
    _boxVao = 0;
    _boxVbo = 0;
    _boxEbo = 0;
}
Model& Model::operator=(Model&& rhs) noexcept {
    if (this != &rhs) {
        // 释放当前对象的资源
        cleanup();

        // 移动赋值资源
        _vertices = std::move(rhs._vertices);
        _indices = std::move(rhs._indices);
        _boundingBox = std::move(rhs._boundingBox);
        // 还可以继续移动其他成员

        // 使 rhs 的资源处于有效但未定义的状态
        rhs._vao = 0;
        rhs._vbo = 0;
        rhs._ebo = 0;
        rhs._boxVao = 0;
        rhs._boxVbo = 0;
        rhs._boxEbo = 0;
    }
    return *this;
}

Model::~Model() {
    cleanup();
}

BoundingBox Model::getBoundingBox() const {
    return _boundingBox;
}

void Model::draw() const {
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Model::drawBoundingBox() const {
    glBindVertexArray(_boxVao);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

GLuint Model::getVao() const {
    return _vao;
}

GLuint Model::getBoundingBoxVao() const {
    return _boxVao;
}

size_t Model::getVertexCount() const {
    return _vertices.size();
}

size_t Model::getFaceCount() const {
    return _indices.size() / 3;
}

void Model::initGLResources() {
    // create a vertex array object
    glGenVertexArrays(1, &_vao);
    // create a vertex buffer object
    glGenBuffers(1, &_vbo);
    // create a element array buffer
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(Vertex) * _vertices.size(), _vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(uint32_t), _indices.data(),
        GL_STATIC_DRAW);

    // specify layout, size of a vertex, data type, normalize, sizeof vertex array, offset of the
    // attribute
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Model::computeBoundingBox() {
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();

    for (const auto& v : _vertices) {
        minX = std::min(v.position.x, minX);
        minY = std::min(v.position.y, minY);
        minZ = std::min(v.position.z, minZ);
        maxX = std::max(v.position.x, maxX);
        maxY = std::max(v.position.y, maxY);
        maxZ = std::max(v.position.z, maxZ);
    }

    _boundingBox.min = glm::vec3(minX, minY, minZ);
    _boundingBox.max = glm::vec3(maxX, maxY, maxZ);
}

void Model::initBoxGLResources() {
    std::vector<glm::vec3> boxVertices = {
        glm::vec3(_boundingBox.min.x, _boundingBox.min.y, _boundingBox.min.z),
        glm::vec3(_boundingBox.max.x, _boundingBox.min.y, _boundingBox.min.z),
        glm::vec3(_boundingBox.min.x, _boundingBox.max.y, _boundingBox.min.z),
        glm::vec3(_boundingBox.max.x, _boundingBox.max.y, _boundingBox.min.z),
        glm::vec3(_boundingBox.min.x, _boundingBox.min.y, _boundingBox.max.z),
        glm::vec3(_boundingBox.max.x, _boundingBox.min.y, _boundingBox.max.z),
        glm::vec3(_boundingBox.min.x, _boundingBox.max.y, _boundingBox.max.z),
        glm::vec3(_boundingBox.max.x, _boundingBox.max.y, _boundingBox.max.z),
    };

    std::vector<uint32_t> boxIndices = {0, 1, 0, 2, 0, 4, 3, 1, 3, 2, 3, 7,
                                        5, 4, 5, 1, 5, 7, 6, 4, 6, 7, 6, 2};

    glGenVertexArrays(1, &_boxVao);
    glGenBuffers(1, &_boxVbo);
    glGenBuffers(1, &_boxEbo);

    glBindVertexArray(_boxVao);
    glBindBuffer(GL_ARRAY_BUFFER, _boxVbo);
    glBufferData(
        GL_ARRAY_BUFFER, boxVertices.size() * sizeof(glm::vec3), boxVertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _boxEbo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, boxIndices.size() * sizeof(uint32_t), boxIndices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Model::cleanup() {
    if (_boxEbo) {
        glDeleteBuffers(1, &_boxEbo);
        _boxEbo = 0;
    }

    if (_boxVbo) {
        glDeleteBuffers(1, &_boxVbo);
        _boxVbo = 0;
    }

    if (_boxVao) {
        glDeleteVertexArrays(1, &_boxVao);
        _boxVao = 0;
    }

    if (_ebo != 0) {
        glDeleteBuffers(1, &_ebo);
        _ebo = 0;
    }

    if (_vbo != 0) {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }

    if (_vao != 0) {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }
}

bool Model::exportToOBJ(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    std::unordered_map<glm::vec3, uint32_t> positionIndexMap;
    std::unordered_map<glm::vec3, uint32_t> normalIndexMap;
    std::unordered_map<glm::vec2, uint32_t> texCoordIndexMap;

    for (const auto& vertex : _vertices) {
        auto pos = vertex.position;
        auto norm = vertex.normal;
        auto tex = vertex.texCoord;

        if (positionIndexMap.find(pos) == positionIndexMap.end()) {
            positionIndexMap[pos] = static_cast<uint32_t>(positionIndexMap.size() + 1);
            file << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
        }

        if (normalIndexMap.find(norm) == normalIndexMap.end()) {
            normalIndexMap[norm] = static_cast<uint32_t>(normalIndexMap.size() + 1);
            file << "vn " << norm.x << " " << norm.y << " " << norm.z << "\n";
        }

        if (texCoordIndexMap.find(tex) == texCoordIndexMap.end()) {
            texCoordIndexMap[tex] = static_cast<uint32_t>(texCoordIndexMap.size() + 1);
            file << "vt " << tex.x << " " << tex.y << "\n";
        }
    }

    for (size_t i = 0; i < _indices.size(); i += 3) {
        file << "f ";
        for (int j = 0; j < 3; ++j) {
            uint32_t positionIndex = positionIndexMap[_vertices[_indices[i + j]].position];
            uint32_t texCoordIndex = texCoordIndexMap[_vertices[_indices[i + j]].texCoord];
            uint32_t normalIndex = normalIndexMap[_vertices[_indices[i + j]].normal];

            file << positionIndex << "/" << texCoordIndex << "/" << normalIndex << " ";
        }
        file << "\n";
    }

    file.close();
    return true;
}
