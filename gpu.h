#ifndef GPU_H
#define GPU_H

#ifndef GLSL
    #include "glm/glm.hpp"
    using namespace glm;
#endif

struct sdf{
	vec3 location;
	int type;
    vec4 rotation;
	vec3 scale;
    int id;
};

struct vertex{
    vec3 location;
    int id;
    vec3 normal;
    float radius;
};

#define SDF_SPHERE 0
#define SDF_BOX 1
#define SDF_PLANE 2

#define SDF_COUNT 256
#define VERT_COUNT 50000

struct SDF_BUF
{
    sdf items[SDF_COUNT];
    int L[  (SDF_COUNT >> 1) * (8 << 0) + 
            (SDF_COUNT >> 2) * (8 << 3) +
            (SDF_COUNT >> 3) * (8 << 6) +
            (SDF_COUNT >> 4) * (8 << 9) + 
            (SDF_COUNT >> 5) * (8 << 12) 
        ];
    vertex verts[VERT_COUNT];
    vec3 center;
    float radius;
    int output_tail, sdf_tail, pad1, pad2;
};

#endif