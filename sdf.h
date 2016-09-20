#ifndef SDF_H
#define SDF_H

#include "glm/glm.hpp"

struct sdf{
	glm::vec3 location;
	int type;
    glm::vec4 rotation;
	glm::vec3 scale;
    int id;
};


#define SDF_SPHERE 0
#define SDF_BOX 1
#define SDF_PLANE 2

#endif