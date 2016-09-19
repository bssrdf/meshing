

#include "myglheaders.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "glprogram.h"
#include "compute_shader.h"
#include "SSBO.h"
#include "mesh.h"
#include "gpu.h"
#include "timer.h"

#include <random>
#include "time.h"

using namespace std;
using namespace glm;

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
    
	SDF_BUF* sdf_buf = new SDF_BUF();
    
    // init sdf_buf data
    
    printf("Size of gpu buffer: %zu\n", sizeof(SDF_BUF));
    
    sdf_buf->center = vec3(0.0f);
    sdf_buf->radius = 1.0f;
    sdf_buf->output_tail = 0;
    sdf_buf->sdf_tail = 0;
    
    {
        sdf& item = sdf_buf->items[sdf_buf->sdf_tail];
        item.location = vec3(0.0f);
        item.type = SDF_SPHERE;
        item.scale = vec3(0.5f);
        item.id = sdf_buf->sdf_tail;
        sdf_buf->sdf_tail++;
    }
    
    Timer timer;
	SSBO sdfbuf(sdf_buf, sizeof(SDF_BUF), 3);
    
    depth.bind();
    timer.begin();
    depth.call(262144 / 64, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    timer.endPrint();
    
    // copy back data from ssbo
    sdfbuf.download(sdf_buf, 0, sizeof(SDF_BUF));
    if(sdf_buf->output_tail > VERT_COUNT)
        sdf_buf->output_tail = VERT_COUNT;
    printf("Points: %i\n", sdf_buf->output_tail);
    
    Mesh mesh;
    mesh.upload(sdf_buf->output_tail, &sdf_buf->verts[0]);
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
            sdf_buf->sdf_tail++;
            sdf_buf->sdf_tail &= (SDF_COUNT - 1);
            sdf& item = sdf_buf->items[sdf_buf->sdf_tail];
            item.location = forward;
            item.type = SDF_SPHERE;
            item.scale = vec3(0.05f);
            item.id = sdf_buf->sdf_tail;
            
            sdf_buf->output_tail = 0;
            
            sdfbuf.upload(sdf_buf, sizeof(SDF_BUF));
            
            depth.bind();
            timer.begin();
            depth.call((8 << 15) / 64, 1, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            timer.endPrint();
            
            // copy back data from ssbo
            sdfbuf.download(sdf_buf, 0, sizeof(SDF_BUF));
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            if(sdf_buf->output_tail > VERT_COUNT)
                sdf_buf->output_tail = VERT_COUNT;
            printf("Points: %i\n", sdf_buf->output_tail);
            mesh.upload(sdf_buf->output_tail, &sdf_buf->verts[0]);
            color.bind();
        }
        
        mesh.draw();
		
        window.swap();
    }
    
    delete sdf_buf;

    return 0;
}

