#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <random>

#include "../base/transform.h"
#include "game.h"
#include "ground.h"
//#include "obstacle.h"

const std::string modelRelPath = "obj/villager.obj";

const std::string earthTextureRelPath = "texture/miscellaneous/earthmap.jpg";
const std::string planetTextureRelPath = "texture/miscellaneous/planet_Quom1200.png";

const std::vector<std::string> skyboxTextureRelPaths = {
    "texture/skybox/Right_Tex.jpg", "texture/skybox/Left_Tex.jpg",  "texture/skybox/Up_Tex.jpg",
    "texture/skybox/Down_Tex.jpg",  "texture/skybox/Front_Tex.jpg", "texture/skybox/Back_Tex.jpg"};

Game::Game(const Options& options) : Application(options) {
    // init model
    _character.reset(new Model(getAssetFullPath(modelRelPath)));
    float height = _character->getBoundingBox().min.y; //get the height of the character
    _character->transform.position = glm::vec3(0.0,-height,5.0); //move to exactly the ground
    float angle = glm::radians(-90.0f);
    glm::quat rotation_more = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    _character->transform.rotation = rotation_more* _character->transform.rotation; //rotation 90°
    //init ground
    _ground.reset(new Ground(5000.0,5000.0)); //big enough
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
    std::vector<std::string> skyboxTextureFullPaths;
    for (size_t i = 0; i < skyboxTextureRelPaths.size(); ++i) {
        skyboxTextureFullPaths.push_back(getAssetFullPath(skyboxTextureRelPaths[i]));
    }
    _skybox.reset(new SkyBox(skyboxTextureFullPaths));

    // init camera
    _camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 10000.0f));
    _camera->transform.position = glm::vec3(0.0,3.0,10.0);

    // init light
    _light.reset(new DirectionalLight());
    _light->intensity = 1.0;
    _light->color = glm::vec3(1.0,1.0,1.0);
    //_light->intensity = 0.5; default intensity and color
    _light->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f)));

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
        "uniform sampler2D mapKd;\n"
        "void main() {\n"
        "    color = texture(mapKd, fTexCoord);\n"
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

        "// directional light data structure declaration\n"
        "struct DirectionalLight {\n"
        "    vec3 direction;\n"
        "    float intensity;\n"
        "    vec3 color;\n"
        "};\n"

        "// uniform variables\n"
        "uniform Material material;\n"
        "uniform DirectionalLight directionalLight;\n"
        "uniform vec3 viewPos;\n"

        "vec3 calcDirectionalLight(vec3 normal,vec3 viewDir) {\n"
        "    vec3 lightDir = normalize(-directionalLight.direction);\n"
        "    vec3 ambient = material.ka * directionalLight.color;\n"
        "    vec3 diffuse = directionalLight.color * max(dot(lightDir, normal), 0.0f) * "
        "material.kd;\n"
        "    vec3 reflectDir = reflect(-lightDir, normal);\n"
        "    vec3 specular = directionalLight.color * pow(max(dot(viewDir, reflectDir), 0.0),material.ns) *"
        "material.ks;\n"
        "    return directionalLight.intensity * (ambient+diffuse+specular);\n"
        "}\n"

        "void main() {\n"
        "    vec3 normal = normalize(fNormal);\n"
        "    vec3 viewDir = normalize(viewPos - fPosition);"
        "    vec3 total = calcDirectionalLight(normal,viewDir);\n"
        "    color = vec4(total, 1.0f);\n"
        "}\n";
    // ------------------------------------------------------------

    _usualShader.reset(new GLSLProgram);
    _usualShader->attachVertexShader(vsCode);
    _usualShader->attachFragmentShader(fsCode);
    _usualShader->link();
}
void Game::handleInput() {
    // constexpr float cameraMoveSpeed = 5.0f;
    // constexpr float cameraRotateSpeed = 0.02f;
    bool flag = collisionDetect();
    if(!flag){
        //game over
    }
    // _character->transform.position += _speed*_deltaTime*_camera->transform.getFront();
    // _camera->transform.position += _speed*_deltaTime*_camera->transform.getFront(); //auto

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
        //just move character
        _character->transform.position -= _speed*_deltaTime*_camera->transform.getRight(); //move right according to camera

    }

    if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        //std::cout << "S" << std::endl;
        //move both character and camera
        _character->transform.position -= _speed*_deltaTime*_camera->transform.getFront();
        _camera->transform.position -= _speed*_deltaTime*_camera->transform.getFront(); 
    }

    if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        //std::cout << "D" << std::endl;
        _character->transform.position += _speed*_deltaTime*_camera->transform.getRight();
    }

    //jump
    if (isSpaceValid && _input.keyboard.keyStates[GLFW_KEY_SPACE] == GLFW_PRESS) { 
        _velocity = jump_velocity; //start jumping
        isSpaceValid = false; //can jump only once
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

    _input.forwardState();
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
    // _textureShader->use();
    // // 2. transfer mvp matrices to gpu
    // _textureShader->setUniformMat4("projection", projection);
    // _textureShader->setUniformMat4("view", view);
    // _textureShader->setUniformMat4("model", _character->transform.getLocalMatrix());
    // // 3. enable textures and transform textures to gpu
    // _simpleMaterial->mapKd->bind();
    // _textureShader->unuse();
    //_sphere->draw();
    
    // for the usual shader
    _usualShader->use();
    // 1. transfer mvp matrix to the shader
    _usualShader->setUniformMat4("projection", projection);
    _usualShader->setUniformMat4("view", view);
    _usualShader->setUniformMat4("model", _ground->transform.getLocalMatrix());
    _usualShader->setUniformVec3("material.ka", _phongMaterial->ka);
    _usualShader->setUniformVec3("material.kd", _phongMaterial->kd);
    _usualShader->setUniformVec3("material.ks", _phongMaterial->ks);
    _usualShader->setUniformFloat("material.ns", _phongMaterial->ns);
    _usualShader->setUniformVec3("directionalLight.direction", _light->transform.getFront());
    _usualShader->setUniformFloat("directionalLight.intensity", _light->intensity);
    _usualShader->setUniformVec3("directionalLight.color", _light->color);
    _ground->draw();

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

        ImGui::Text("Directional light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity", &_light->intensity, 0.0f, 2.0f);
        ImGui::ColorEdit3("color", (float*)&_light->color);
        ImGui::NewLine();

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Game::initObstacles(){
    int num = 5; //easy mode
    generateRandomObstacles(num,-10.0,10.0,-10.0,0);
}
void Game::generateRandomObstacles(int obstacleCount, float minX, float maxX, float minZ, float maxZ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(minX, maxX);
    std::uniform_real_distribution<float> distZ(minZ, maxZ);
    for (int i = 0; i < obstacleCount; ++i) {
        // Generate random x and z positions within the specified range
        float x = distX(gen);
        float z = distZ(gen);
        bool flag = detectHurdle(x,z); //to detect if the current x,z don't collide with others
        if(!flag){
            i--;
            continue;
        }
        Obstacle cur;
        cur.transform.position.x = x;
        cur.transform.position.z = z;
        float height = cur.getBoundingBox().min.y; //get the height of the character
        cur.transform.position.y = -height; //move to exactly the ground
        _obstacles.emplace_back(std::move(cur)); //cur is of no use now
    }
}
bool Game::detectHurdle(float x, float z){
    for(auto& hurdle: _obstacles){
        float hurdlex = hurdle.transform.position.x;
        float distx = hurdle.getBoundingBox().max.x; //perhaps 0.5
        float hurdlez = hurdle.transform.position.z;
        float distz = hurdle.getBoundingBox().max.z; //perhaps 0.5
        if(abs(x-hurdlex)>2*distx && abs(z-hurdlez)>2*distz){

        }else{
            return false;
        }
    }
    return true;
}

bool Game::collisionDetect(){
    /*
    *TO DO
    */
    return true;
}