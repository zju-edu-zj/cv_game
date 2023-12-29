#pragma once

#include "../base/model.h"

class Obstacle : public Model {
public:
    Obstacle();
    Obstacle(Obstacle&& rhs);
    Obstacle& operator=(Obstacle&& rhs);
    //Obstacle(const Obstacle&) = delete; // 禁用拷贝构造函数

    bool operator<(const Obstacle& other) const {
        return _vao < other._vao; // 或者使用其他逻辑来定义比较规则
    }
};
