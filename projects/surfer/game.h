#pragma once

#include <memory>
#include <string>
#include <set>
#include <unordered_set>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/skybox.h"
#include "../base/texture2d.h"
#include "obstacle.h"

struct SimpleMaterial {
    std::shared_ptr<Texture2D> mapKd;
};

struct PhongMaterial {
    glm::vec3 ka;
    glm::vec3 kd;
    glm::vec3 ks;
    float ns;
};

class Game : public Application {
public:
    Game(const Options& options);

    ~Game();

private:
    std::unique_ptr<Model> _character;

    std::unique_ptr<Model> _ground;

    std::unique_ptr<SimpleMaterial> _simpleMaterial;
    std::unique_ptr<PhongMaterial> _phongMaterial;
    
    std::unique_ptr<PerspectiveCamera> _camera;
    std::unique_ptr<DirectionalLight> _light;

    std::unique_ptr<GLSLProgram> _textureShader; //for texture
    std::unique_ptr<GLSLProgram> _usualShader; //the ususal ones

    std::unique_ptr<SkyBox> _skybox;

    std::set<Obstacle> _obstacles; 

    float _speed = 4.0f; //character move speed
    float _velocity = 0;
    const float accelation = -10.0; //gravity
    const float jump_velocity = 6.0;

    const float panel_width = 5.0;

    bool isSpaceValid = true;

    float _moveForward = 0; //move forward distance
    
    void initTextureShader();
    void initPhongShader();

    void initObstacles();
    void generateRandomObstacles(int obstacleCount, float minX, float maxX, float minZ, float maxZ);
    bool detectHurdle(float x, float z);
    
    /*
    * 碰撞检测
    */
    bool collisionDetect();
    BoundingBox transformBoundingBox(const BoundingBox& box, const glm::mat4& transform);

    void handleInput() override;

    void renderFrame() override;
    /*
    * process the logical updating of states
    */
    void update();
};