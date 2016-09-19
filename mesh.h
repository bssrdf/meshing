#ifndef MESH_H
#define MESH_H

struct vertex;

class Mesh{
	unsigned vao, vbo, num_vertices;
public:
    Mesh();
    ~Mesh();
	void draw();
	void upload(size_t count, vertex* ptr);
};

#endif
