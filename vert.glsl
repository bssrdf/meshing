
#version 330 core

in vec3 location;
in int id;
in vec3 normal;

flat out vec3 flocation;
flat out int fid;
flat out vec3 fnormal;
flat out int valid;

uniform mat4 MVP;
uniform vec3 eye;

void main(){
	gl_Position = MVP * vec4(location, 1.0);
    flocation = location;
    fid = id;
    fnormal = normal;
    gl_PointSize = 50.0 / gl_Position.w;
    
    if(dot(normal, eye - location) < 0.0)
        valid = 0;
    else
        valid = 1;
}
