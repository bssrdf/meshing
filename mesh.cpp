#include "myglheaders.h"
#include "mesh.h"
#include "debugmacro.h"
#include "vertex.h"

Mesh::Mesh(){
	num_vertices = 0;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	glEnableVertexAttribArray(0);	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    
    glEnableVertexAttribArray(1);   // id
    glVertexAttribPointer(1, 1, GL_INT, GL_FALSE, sizeof(vertex), (void*)(sizeof(float) * 3));
	
	glEnableVertexAttribArray(2);	// normal
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(float)*4));
	
	glEnableVertexAttribArray(3);	// radius
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(sizeof(float)*7));
	
	MYGLERRORMACRO
}

Mesh::~Mesh(){
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	MYGLERRORMACRO
}

void Mesh::upload(size_t count, vertex* ptr){
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex)*count, ptr, GL_STATIC_DRAW);
	MYGLERRORMACRO
	num_vertices = (unsigned)count;
}

void Mesh::draw(){
	if(!num_vertices)return;
	glBindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, num_vertices);
	MYGLERRORMACRO
}

