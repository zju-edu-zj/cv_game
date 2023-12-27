#pragma once

#include "../base/model.h"

class Obstacle : public Model {
public:
    Obstacle();
    Obstacle(Obstacle&& rhs);
    //Obstacle(const Obstacle&) = delete; // 禁用拷贝构造函数
};
