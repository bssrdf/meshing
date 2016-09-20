#ifndef VERTEX_H
#define VERTEX_H

#include "glm/glm.hpp"

struct vertex{
    glm::vec3 location;
    int id;
    glm::vec3 normal;
    float radius;
};

#endif