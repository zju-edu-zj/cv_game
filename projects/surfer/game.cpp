#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <random>
#include <algorithm>

#include "../base/transform.h"
#include "game.h"
#include "ground.h"
//#include "obstacle.h"

const std::string modelRelPath = "obj/villager.obj";

const std::string earthTextureRelPath = "texture/miscellaneous/earthmap.jpg";
const std::string planetTextureRelPath = "texture/miscellaneous/planet_Quom1200.png";

const std::string groundTextureRelPath = "texture/ground/final1.jpg";

const std::vector<std::string> skyboxTextureRelPaths = {
    "texture/skybox/Right_Tex.jpg", "texture/skybox/Left_Tex.jpg",  "texture/skybox/Up_Tex.jpg",
    "texture/skybox/Down_Tex.jpg",  "texture/skybox/Front_Tex.jpg", "texture/skybox/Back_Tex.jpg"};

Game::Game(const Options& options) : Application(options) {

    initModelResources(); //all the light,sky and character
    // init shaders
    initTextureShader();
    initPhongShader();

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init();
}
void Game::initModelResources(){
    // init model
    if(_character==nullptr){
        _character.reset(new Model(getAssetFullPath(modelRelPath)));
        //testOn(); //test obj loader
        float angle = glm::radians(-90.0f);
        glm::quat rotation_more = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
        _character->transform.rotation = rotation_more* _character->transform.rotation; //rotation 90°
    }
    float height = _character->getBoundingBox().min.y; //get the height of the character
    _character->transform.position = glm::vec3(0.0,-height,5.0); //move to exactly the ground
    //init ground
    _ground.reset(new Ground(20.0,10.0)); //long enough
    _groundTexture.reset(new ImageTexture2D(getAssetFullPath(groundTextureRelPath)));
    _groundTexture->bind();
    
    Transform new_transform;
    _groundTransforms.clear();
    _groundTransforms.push_back(new_transform);
    new_transform.position.z -= 10;
    _groundTransforms.push_back(new_transform);
    new_transform.position.z -= 10;
    _groundTransforms.push_back(new_transform);
    
    _obstacles.clear();
    initObstacles();

    // init textures
    // std::shared_ptr<Texture2D> earthTexture =
    //     std::make_shared<ImageTexture2D>(getAssetFullPath(earthTextureRelPath));
    // // init materials
    // _simpleMaterial.reset(new SimpleMaterial);
    // _simpleMaterial->mapKd = earthTexture;

    _phongMaterial.reset(new PhongMaterial);
    _phongMaterial->ka = glm::vec3(8.0/256,8.0/256,8.0/256);
    _phongMaterial->kd = glm::vec3(1.0,1.0,1.0);
    _phongMaterial->ks = glm::vec3(1.0,1.0,1.0);
    _phongMaterial->ns = 10; //for ground

    // init skybox
    if(_skybox==nullptr){
        std::vector<std::string> skyboxTextureFullPaths;
        for (size_t i = 0; i < skyboxTextureRelPaths.size(); ++i) {
            skyboxTextureFullPaths.push_back(getAssetFullPath(skyboxTextureRelPaths[i]));
        }
        _skybox.reset(new SkyBox(skyboxTextureFullPaths));
    }
    
    // init camera
    if(_camera==nullptr){
        _camera.reset(new PerspectiveCamera(
            glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 10000.0f));
    }
    _camera->transform.position = glm::vec3(0.0,3.0,12.0);

    // init lights
    _ambientLight.reset(new AmbientLight);
    _ambientLight->intensity = 1.0;
    _directionalLight.reset(new DirectionalLight);
    _directionalLight->intensity = 0.2f;
    _directionalLight->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    _spotLight.reset(new SpotLight);
    _spotLight->intensity = 3.0f;
    _spotLight->angle = glm::radians(150.0f);
    _spotLight->transform.position = glm::vec3(0.0f, 1.0f, 4.5f);
    _spotLight->transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
}
Game::~Game() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Game::initTextureShader() {
    const char* vsCode =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"
        "out vec2 fTexCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"

        "void main() {\n"
        "    fTexCoord = aTexCoord;\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* fsCode =
        "#version 330 core\n"
        "in vec2 fTexCoord;\n"
        "out vec4 color;\n"
        "struct AmbientLight {\n"
        "    vec3 color;\n"
        "    float intensity;\n"
        "};\n"

        "uniform sampler2D mapKd;\n"
        "uniform AmbientLight ambientLight;\n"

        "void main() {\n"
        "    vec3 result = ambientLight.color* ambientLight.intensity* texture(mapKd, fTexCoord).rgb;\n"
        "    color = vec4(result,1.0f);\n"
        "}\n";

    _textureShader.reset(new GLSLProgram);
    _textureShader->attachVertexShader(vsCode);
    _textureShader->attachFragmentShader(fsCode);
    _textureShader->link();
}
void Game::initPhongShader() {
    const char* vsCode =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"

        "out vec3 fPosition;\n"
        "out vec3 fNormal;\n"

        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        

        "void main() {\n"
        "    fPosition = vec3(model * vec4(aPosition, 1.0f));\n"
        "    fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* fsCode =
        "#version 330 core\n"
        "in vec3 fPosition;\n"
        "in vec3 fNormal;\n"
        "out vec4 color;\n"

        "// material data structure declaration\n"
        "struct Material {\n"
        "    vec3 ka;\n"
        "    vec3 kd;\n"
        "    vec3 ks;\n"
        "    float ns;\n"
        "};\n"
        "// ambient light data structure declaration\n"
        "struct AmbientLight {\n"
        "    vec3 color;\n"
        "    float intensity;\n"
        "};\n"
        "// directional light data structure declaration\n"
        "struct DirectionalLight {\n"
        "    vec3 direction;\n"
        "    float intensity;\n"
        "    vec3 color;\n"
        "};\n"

        "// spot light data structure declaration\n"
        "struct SpotLight {\n"
        "    vec3 position;\n"
        "    vec3 direction;\n"
        "    float intensity;\n"
        "    vec3 color;\n"
        "    float angle;\n"
        "    float kc;\n"
        "    float kl;\n"
        "    float kq;\n"
        "};\n"

        "// uniform variables\n"
        "uniform Material material;\n"
        "uniform DirectionalLight directionalLight;\n"
        "uniform SpotLight spotLight;\n"
        "uniform AmbientLight ambientLight;\n"
        "uniform vec3 viewPos;\n"

        "vec3 calcDirectionalLight(vec3 normal,vec3 viewDir) {\n"
        "    vec3 lightDir = normalize(-directionalLight.direction);\n"
        "    //vec3 ambient = material.ka * directionalLight.color;\n"
        "    vec3 diffuse = directionalLight.color * max(dot(lightDir, normal), 0.0f) * "
        "material.kd;\n"
        "    vec3 reflectDir = reflect(-lightDir, normal);\n"
        "    vec3 specular = directionalLight.color * pow(max(dot(viewDir, reflectDir), 0.0),material.ns) *"
        "material.ks;\n"
        "    return directionalLight.intensity * (diffuse+specular);\n"
        "}\n"

        "vec3 calcSpotLight(vec3 normal,vec3 viewDir) {\n"
        "    vec3 lightDir = normalize(spotLight.position - fPosition);\n"
        "    float theta = acos(-dot(lightDir, normalize(spotLight.direction)));\n"
        "    if (theta > spotLight.angle) {\n"
        "        return vec3(0.0f, 0.0f, 0.0f);\n"
        "    }\n"
        "    //vec3 ambient = material.ka * spotLight.color;\n"
        "    vec3 diffuse = spotLight.color * max(dot(lightDir, normal), 0.0f) * material.kd;\n"
        "    vec3 reflectDir = reflect(-lightDir, normal);\n"
        "    vec3 specular = spotLight.color * pow(max(dot(viewDir, reflectDir), 0.0),material.ns) *"
        "material.ks;\n"
        "    float distance = length(spotLight.position - fPosition);\n"
        "    float attenuation = 1.0f / (spotLight.kc + spotLight.kl * distance + spotLight.kq * "
        "distance * distance);\n"
        "    return spotLight.intensity * attenuation * (diffuse+specular);\n"
        "}\n"

        "void main() {\n"
        "    vec3 normal = normalize(fNormal);\n"
        "    vec3 viewDir = normalize(viewPos - fPosition);"
        "    vec3 total = calcDirectionalLight(normal,viewDir) + calcSpotLight(normal,viewDir) + "
        "ambientLight.color *ambientLight.intensity *material.ka;\n"
        "    color = vec4(total, 1.0f);\n"
        "}\n";
    // ------------------------------------------------------------

    _usualShader.reset(new GLSLProgram);
    _usualShader->attachVertexShader(vsCode);
    _usualShader->attachFragmentShader(fsCode);
    _usualShader->link();
}
void Game::update(){
    float challenge = 0.001f;
    _speed += challenge*_deltaTime; //more and more difficult

    _moveForward += _speed*_deltaTime;
    _character->transform.position += _speed*_deltaTime*_camera->transform.getFront();
    _camera->transform.position += _speed*_deltaTime*_camera->transform.getFront(); //auto
    _spotLight->transform.position += _speed*_deltaTime*_camera->transform.getFront();
    //std::cout << _spotLight->transform.position.x << _spotLight->transform.position.y << _spotLight->transform.position.z<< std::endl;
    
    bool flag = collisionDetect();
    if(flag){
        //game over
        std::cout << "collision detected" << std::endl;
        _speed = 0; //stop
        _velocity = 0; //jump invalid too
        isFailed = true;
    }
    if(!isSpaceValid){ //the jumping process
        _character->transform.position.y += _velocity*_deltaTime; //jump up
        float height = _character->getBoundingBox().min.y;
        if(_character->transform.position.y <= height){ //return to ground
            _velocity = 0;
            isSpaceValid = true;
            std::cout << "return back" << std::endl;
        }else{
            _velocity += accelation*_deltaTime;
            //std::cout << _velocity << std::endl;
        }
    }
    //extend the scenes and destroy objects automatically
    float camera_pos = _camera->transform.position.z;
    for (auto it = _obstacles.begin(); it != _obstacles.end();) {
        if (it->transform.position.z >= camera_pos) {
            it = _obstacles.erase(it);
        }else {
            ++it;  //to prohibit invalid iterators
        }
    }

    if(_groundTransforms.front().position.z > camera_pos){
        _groundTransforms.pop_front();
        Transform new_trans = _groundTransforms.back();
        new_trans.position.z -= 10;
        _groundTransforms.push_back(new_trans);
    }


    const float far_view = 10.0f;
    if(_moveForward >= far_view){
        float character_pos = _character->transform.position.z;
        int num = 6;
        generateRandomObstacles(num,1.0,5.0,-8.0,8.0,character_pos-15,character_pos-5);
        // std::cout << _obstacles.size() << std::endl;
        _moveForward = 0; //clear
    }
    // std::cout << "end update" << std::endl;
}
void Game::handleInput() {
    
    
    //mouse event
    float zoomFacter = 0.05f;
    if(_camera->fovy <= glm::radians(80.0f)&& _camera->fovy >= glm::radians(20.0f)){
        _camera->fovy -= _input.mouse.scroll.yOffset*zoomFacter ;
    }
    //keyboard event
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }
    if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        //std::cout << "W" << std::endl;
        //move both character and camera
        _character->transform.position += _speed*_deltaTime*_camera->transform.getFront();
        _camera->transform.position += _speed*_deltaTime*_camera->transform.getFront(); 
    }

    if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        //just move character and character can only move in the panel
        if(_character->transform.position.x >= -panel_width){
            _character->transform.position -= _speed*_deltaTime*_camera->transform.getRight();
        }

    }
    //static bool is_stop = true;
    static float temp_speed = 0.0f;
    //stop temporarily
    if (isStopValid && _input.keyboard.keyStates[GLFW_KEY_S] == GLFW_PRESS) {
        float t = _speed;
        _speed = temp_speed;
        temp_speed = t;  //just swap the speeds
        isStopValid = false;
    }
    if(_input.keyboard.keyStates[GLFW_KEY_S] == GLFW_RELEASE){
        isStopValid = true;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        //std::cout << "D" << std::endl;
        if(_character->transform.position.x <= panel_width){
            //std::cout << _character->transform.position.x <<' ' << panel_width << std::endl;
            _character->transform.position += _speed*_deltaTime*_camera->transform.getRight();
        }
    }
    //Fit
    if (_input.keyboard.keyStates[GLFW_KEY_F] != GLFW_RELEASE) {
        _camera->fovy = glm::radians(50.0f); //zoom to a fit fovy
    }
    //jump
    if (isSpaceValid && _input.keyboard.keyStates[GLFW_KEY_SPACE] == GLFW_PRESS) { 
        _velocity = jump_velocity; //start jumping
        isSpaceValid = false; //can jump only once
    }
    
    static bool isRestartValid = true;
    if (isFailed && isRestartValid && _input.keyboard.keyStates[GLFW_KEY_R] == GLFW_PRESS) { 
        //_obstacles.clear();
        initModelResources(); //clear resources automatically
        _speed = 4.0f;
        isRestartValid = false;
    }
    if(_input.keyboard.keyStates[GLFW_KEY_S] == GLFW_RELEASE){
        isRestartValid = true;
        isFailed = false;  //restart now
    }


    _input.forwardState();

    update(); //update the logical state of the game
}

void Game::renderFrame() {
    // some options related to imGUI
    static bool wireframe = false;

    // trivial things
    showFpsInWindowTitle();

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    const glm::mat4 projection = _camera->getProjectionMatrix();
    const glm::mat4 view = _camera->getViewMatrix();

    // 1. use the shader
    _textureShader->use();
    
    // 2. transfer mvp matrices to gpu
    _groundTexture->bind(0);
    // _textureShader->setUniformInt("mapKd", 0);

    _textureShader->setUniformMat4("projection", projection);
    _textureShader->setUniformMat4("view", view);
    _textureShader->setUniformMat4("model", _character->transform.getLocalMatrix());
    _textureShader->setUniformVec3("ambientLight.color", _ambientLight->color);
    _textureShader->setUniformFloat("ambientLight.intensity", _ambientLight->intensity);

    for (auto &it : _groundTransforms){
        //_ground->transform = it;
        _textureShader->setUniformMat4("model", it.getLocalMatrix());
        _ground->draw();
    }
    // // 3. enable textures and transform textures to gpu
    // _simpleMaterial->mapKd->bind();
    // _textureShader->unuse();
    // _sphere->draw();
    
    // for the usual shader
    _usualShader->use();
    // 1. transfer mvp matrix to the shader
    _usualShader->setUniformMat4("projection", projection);
    _usualShader->setUniformMat4("view", view);
    _usualShader->setUniformMat4("model", _ground->transform.getLocalMatrix());

    _usualShader->setUniformVec3("viewPos", _camera->transform.position);

    _usualShader->setUniformVec3("material.ka", _phongMaterial->ka);
    _usualShader->setUniformVec3("material.kd", _phongMaterial->kd);
    _usualShader->setUniformVec3("material.ks", _phongMaterial->ks);
    _usualShader->setUniformFloat("material.ns", _phongMaterial->ns);
    
    _usualShader->setUniformVec3("ambientLight.color", _ambientLight->color);
    _usualShader->setUniformFloat("ambientLight.intensity", _ambientLight->intensity);
    _usualShader->setUniformVec3("spotLight.position", _spotLight->transform.position);
    _usualShader->setUniformVec3("spotLight.direction", _spotLight->transform.getFront()); //point down
    _usualShader->setUniformFloat("spotLight.intensity", _spotLight->intensity);
    _usualShader->setUniformVec3("spotLight.color", _spotLight->color);
    _usualShader->setUniformFloat("spotLight.angle", _spotLight->angle);
    _usualShader->setUniformFloat("spotLight.kc", _spotLight->kc);
    _usualShader->setUniformFloat("spotLight.kl", _spotLight->kl);
    _usualShader->setUniformFloat("spotLight.kq", _spotLight->kq);
    _usualShader->setUniformVec3(
        "directionalLight.direction", _directionalLight->transform.getFront());
    _usualShader->setUniformFloat("directionalLight.intensity", _directionalLight->intensity);
    _usualShader->setUniformVec3("directionalLight.color", _directionalLight->color);

    _usualShader->setUniformMat4("model", _character->transform.getLocalMatrix());
    _character->draw();
    //std::cout << _obstacles.size() << std::endl;
    for(auto &obstacle:_obstacles){
        _usualShader->setUniformMat4("model", obstacle.transform.getLocalMatrix());
        obstacle.draw();
        //std::cout << obstacle.getVao() << std::endl;
    }
    // draw skybox
    _skybox->draw(projection, view); //draw at last

    // draw ui elements
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("Render Mode");
        ImGui::Separator();

        ImGui::ColorEdit3("ka##3", (float*)&_phongMaterial->ka);
        ImGui::ColorEdit3("kd##3", (float*)&_phongMaterial->kd);
        ImGui::ColorEdit3("ks##3", (float*)&_phongMaterial->ks);
        ImGui::SliderFloat("ns##3", &_phongMaterial->ns, 1.0f, 50.0f);
        ImGui::NewLine();

        ImGui::Text("ambient light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity##1", &_ambientLight->intensity, 0.0f, 1.0f);
        ImGui::ColorEdit3("color##1", (float*)&_ambientLight->color);
        ImGui::NewLine();

        ImGui::Text("directional light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity##2", &_directionalLight->intensity, 0.0f, 1.0f);
        ImGui::ColorEdit3("color##2", (float*)&_directionalLight->color);
        ImGui::NewLine();

        ImGui::Text("spot light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity##3", &_spotLight->intensity, 0.0f, 3.0f);
        ImGui::ColorEdit3("color##3", (float*)&_spotLight->color);
        ImGui::SliderFloat(
            "angle##3", (float*)&_spotLight->angle, 0.0f, glm::radians(180.0f), "%f rad");
        ImGui::NewLine();

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Game::initObstacles() {
    int num = 5; // easy mode
    //generateRandomObstacles(num,1.0,5.0, -10.0, 10.0, -10.0, 0);
}

void Game::generateRandomObstacles(
    int obstacleCount, float minLength, float maxLength, float minX, float maxX, float minZ,
    float maxZ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(minX, maxX);
    std::uniform_real_distribution<float> distZ(minZ, maxZ);
    std::uniform_real_distribution<float> distOffset(-1.0f, 1.0f);
    std::uniform_real_distribution<float> distLength(minLength, maxLength); // 设置障碍物的长度范围
    std::uniform_real_distribution<float> distHeight(0.5f, 1.5f);
    std::uniform_real_distribution<> shape(0, 6);
    std::set<std::pair<float, float>> obstaclePositions;

    int generatedCount = 0;
    while (generatedCount < obstacleCount) {
        float x = distX(gen);
        float z = distZ(gen);
        float length = 1.0f; // 随机生成障碍物的长度
        float Height = distHeight(gen);//  随机生成障碍物的高度
        if (Height <= 1.0f) {
            Height = 0.8f;
            length = distLength(gen);
        } else if (Height > 1.0f) {
            Height = 3.0f;
            std::uniform_real_distribution<float> distLength1(1.0f, 2.0f);
            length = distLength1(gen);
        }
        // // 添加随机偏移量
        // float offsetX = distOffset(gen);
        // float offsetZ = distOffset(gen);
        // x += offsetX;
        // z += offsetZ;

        bool flag = detectHurdle(x, z);
        if (!flag) {
            //std::cout << "failed" << std::endl;
            continue;
        }
        int shape_ = shape(gen);
        Obstacle cur(shape_);
        cur.transform.position.x = x;
        cur.transform.position.z = z;
        float height = cur.getBoundingBox().min.y; // get the height of the character
        cur.transform.position.y = -height;        // move to exactly the ground
        // cur.transform.scale.x = length;
        // cur.transform.scale.y = Height;

        _obstacles.insert(std::move(cur));
        ++generatedCount;
    }

}

bool Game::detectHurdle(float x, float z){
    for(auto& hurdle: _obstacles){
        float hurdlex = hurdle.transform.position.x;
        float distx = hurdle.getBoundingBox().max.x; //perhaps 0.5
        float hurdlez = hurdle.transform.position.z;
        float distz = hurdle.getBoundingBox().max.z; //perhaps 0.5
        if(abs(x-hurdlex)>2*distx || abs(z-hurdlez)>2*distz){
            
        }else{
            // std::cout << x << "  " << z << "  " << hurdlex << "  " << hurdlez << std::endl;
            return false;
        }
    }
    return true;
}

bool Game::collisionDetect(){
    BoundingBox box1 = transformBoundingBox(_character->getBoundingBox(),_character->transform.getLocalMatrix());
    glm::vec3 aabb_center = _character->transform.position;
    for(auto it=_obstacles.begin();it!=_obstacles.end();++it){
        if(it->_shape==1){  //sphere
            glm::vec3 sphere_center = it->transform.position;
            //glm::vec3 diff = sphere_center - aabb_center;
            glm::vec3 clamped = glm::clamp(sphere_center,box1.min,box1.max);
            glm::vec3 difference = clamped - sphere_center;
            if(glm::length(difference) < it->_shapeInfo){  //in the sphere
                return true;
            }
            continue;
        }

        BoundingBox box2 = transformBoundingBox(it->getBoundingBox(),it->transform.getLocalMatrix());
        // 检查每个维度上的重叠
        bool xOverlap = (box1.min.x <= box2.max.x) && (box1.max.x >= box2.min.x);
        bool yOverlap = (box1.min.y <= box2.max.y) && (box1.max.y >= box2.min.y);
        bool zOverlap = (box1.min.z <= box2.max.z) && (box1.max.z >= box2.min.z);
        bool flag = xOverlap && yOverlap && zOverlap;
        if(flag){ // 如果三个维度上都有重叠，则认为发生碰撞
            return true;
        }
    }

    return false;
}

BoundingBox Game::transformBoundingBox(const BoundingBox& box, const glm::mat4& transform) {
    BoundingBox transformedBox;

    // Transform each corner of the bounding box
    glm::vec3 corners[8] = {
        glm::vec3(box.min.x, box.min.y, box.min.z),
        glm::vec3(box.min.x, box.min.y, box.max.z),
        glm::vec3(box.min.x, box.max.y, box.min.z),
        glm::vec3(box.min.x, box.max.y, box.max.z),
        glm::vec3(box.max.x, box.min.y, box.min.z),
        glm::vec3(box.max.x, box.min.y, box.max.z),
        glm::vec3(box.max.x, box.max.y, box.min.z),
        glm::vec3(box.max.x, box.max.y, box.max.z)
    };

    for (int i = 0; i < 8; ++i) {
        glm::vec4 transformedPoint = transform * glm::vec4(corners[i], 1.0f);
        transformedBox.min = glm::min(transformedBox.min, glm::vec3(transformedPoint));
        transformedBox.max = glm::max(transformedBox.max, glm::vec3(transformedPoint));
    }

    return transformedBox;
}

void Game::testOn(){
    //first write character to test_character.obj
    const std::string writePath = "test_character.obj";
    bool flag = _character->exportToOBJ(writePath);
    if(!flag){
        perror("cannot open file");
    }
    _character = nullptr;
    _character.reset(new Model(writePath,true)); //reload to see if it's the same;
}
