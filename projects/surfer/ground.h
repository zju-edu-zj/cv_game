#ifndef GROUND_H
#define GROUND_H

#include <vector>

#include "../base/model.h"

class Ground : public Model {
public:
    Ground(float width, float length);
private:
    float _width;
    float _length;

    void generateGround();
};

#endif // GROUND_H
