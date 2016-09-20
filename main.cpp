

#include "myglheaders.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "glprogram.h"
#include "compute_shader.h"
#include "SSBO.h"
#include "mesh.h"
#include "timer.h"
#include "vertex.h"
#include "sdf.h"

#include <random>
#include "time.h"

using namespace std;
using namespace glm;


struct Meta_buf{
    int SDF_SZ, VERT_SZ, OCT_SZ, vert_tail;
    vec3 center;
    float radius;
    int sdf_tail;
};

float frameBegin(unsigned& i, float& t){
    float dt = (float)glfwGetTime() - t;
    t += dt;
    i++;
    if(t >= 3.0f){
    	float ms = (t / i) * 1000.0f;
        printf("ms: %.6f, FPS: %.3f\n", ms, i / t);
        i = 0;
        t = 0.0f;
        glfwSetTime(0.0f);
    }
    return dt;
}

int main(int argc, char** argv){
    
    int WIDTH = 1280;
    int HEIGHT = 720;
    
	if(argc >= 3){
        WIDTH = atoi(argv[1]);
		HEIGHT = atoi(argv[2]);
	}
    
    srand(time(NULL));
	
	Camera camera;
	camera.resize(WIDTH, HEIGHT);
	camera.setEye(vec3(0.0f, 0.0f, 3.0f));
	camera.update();
	
	Window window(WIDTH, HEIGHT, 4, 3, "Meshing");
	Input input(window.getWindow());
	
	GLProgram color("vert.glsl", "frag.glsl");
	ComputeShader depth("depth.glsl");
    
    const int t_count = 128;
    
    Meta_buf meta;
    meta.SDF_SZ = 1024;
    meta.OCT_SZ = (meta.SDF_SZ >> 1) * (8 << 0) + 
            (meta.SDF_SZ >> 2) * (8 << 3) +
            (meta.SDF_SZ >> 3) * (8 << 6) +
            (meta.SDF_SZ >> 4) * (8 << 9) + 
            (meta.SDF_SZ >> 5) * (8 << 12);
    meta.VERT_SZ = 50000;
    meta.center = vec3(0.0f);
    meta.radius = 1.0f;
    meta.vert_tail = 0;
    meta.sdf_tail = 0;
    
    int* L = new int[meta.OCT_SZ];
    vertex* verts = new vertex[meta.VERT_SZ];
    sdf* items = new sdf[meta.SDF_SZ];
        
    {
        sdf& item = items[0];
        item.location = vec3(0.0f);
        item.type = SDF_SPHERE;
        item.scale = vec3(0.5f);
        item.id = 0;
        meta.sdf_tail = 1;
    }
    
    SSBO meta_bo(&meta, sizeof(Meta_buf), 0);
    SSBO L_bo(L, sizeof(int) * meta.OCT_SZ, 1);
    SSBO items_bo(items, sizeof(sdf) * meta.SDF_SZ, 2);
    SSBO verts_bo(verts, sizeof(vertex) * meta.VERT_SZ, 3);
    
    Mesh mesh;
    Timer timer;
    
    depth.bind();
    timer.begin();
    depth.call((8 << 15) / t_count, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    timer.endPrint();
    
    // copy back data from ssbo
    meta_bo.download(&meta, 0, sizeof(Meta_buf));
    if(meta.vert_tail > meta.VERT_SZ)
        meta.vert_tail = meta.VERT_SZ;
    verts_bo.download(verts, 0, sizeof(vertex) * meta.vert_tail);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    printf("Points: %i\n", meta.vert_tail);
    mesh.upload(meta.vert_tail, verts);
    
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
	
	input.poll();
    unsigned frame_i = 0;
    float t = (float)glfwGetTime();
    while(window.open()){
        input.poll(frameBegin(frame_i, t), camera);
		
		color.bind();
        mat4 VP = camera.getVP();
        color.setUniform("MVP", VP);
        if(input.rightMouseDown())
            color.setUniform("light_position", camera.getEye());
        color.setUniform("eye", camera.getEye());
        color.setUniformInt("seed", rand());
        
        if(input.leftMouseDown() && (frame_i & 1) == 0){
            vec3 forward = camera.getEye() + normalize(camera.getAt() - camera.getEye());
            meta.sdf_tail++;
            meta.sdf_tail &= (meta.SDF_SZ - 1);
            sdf& item = items[meta.sdf_tail];
            item.location = forward;
            item.type = SDF_SPHERE;
            item.scale = vec3(0.01f);
            item.id = meta.sdf_tail;
            
            meta.vert_tail = 0;
            
            meta_bo.upload(&meta, sizeof(Meta_buf));
            items_bo.upload(items, sizeof(sdf) * meta.SDF_SZ);
            
            depth.bind();
            timer.begin();
            depth.call((8 << 15) / t_count, 1, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            timer.endPrint();
            
            // copy back data from ssbo
            meta_bo.download(&meta, 0, sizeof(Meta_buf));
            if(meta.vert_tail > meta.VERT_SZ)
                meta.vert_tail = meta.VERT_SZ;
            verts_bo.download(verts, 0, sizeof(vertex) * meta.vert_tail);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            printf("Points: %i\n", meta.vert_tail);
            mesh.upload(meta.vert_tail, verts);
            color.bind();
        }
        
        mesh.draw();
		
        window.swap();
    }
    
    delete[] verts;
    delete[] items;
    delete[] L;

    return 0;
}

