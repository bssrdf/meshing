
#include "myglheaders.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "glprogram.h"
#include "UBO.h"
#include "mesh.h"
#include "timer.h"
#include "csgarray.h"
#include <thread>

using namespace std;
using namespace glm;

struct Uniforms{
	mat4 MVP;
	vec4 eye;
	vec4 light_pos;
};

void remesh(CSGArray* ary, CSGList* insertQueue, bool* done, float spu){
	for(auto& i : *insertQueue){
		ary->insert(i);
	}
	insertQueue->clear();
	ary->remesh(spu);
	*done = true;
}


double frameBegin(unsigned& i, double& t){
    double dt = glfwGetTime() - t;
    t += dt;
    i++;
    if(t >= 3.0){
    	double ms = (t / i) * 1000.0;
        printf("ms: %.6f, FPS: %.3f\n", ms, i / t);
        i = 0;
        t = 0.0;
        glfwSetTime(0.0);
    }
    return dt;
}

int main(int argc, char* argv[]){
	if(argc != 3){
		return 1;
	}
	
	Camera camera;
	const int WIDTH = atoi(argv[1]);
	const int HEIGHT = atoi(argv[2]);
	camera.resize(WIDTH, HEIGHT);
	camera.setEye(vec3(0.0f, 0.0f, 3.0f));
	camera.update();
	
	Window window(WIDTH, HEIGHT, 4, 3, "Meshing");
	Input input(window.getWindow());
	GLProgram colorProg("vert.glsl", "frag.glsl");
	
	CSGArray csgary(4.0f);
	Mesh brushMesh;
	brushMesh.init();
	VertexBuffer vb;
	
	Timer timer;
	
	Uniforms uni;
	uni.light_pos = vec4(5.0f, 5.0f, 5.0f, 0.0f);
	UBO unibuf(&uni, sizeof(uni), 0);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	
	float bsize = 0.2f;
	
	input.poll();
    unsigned i = 0;
    double t = glfwGetTime();
    int waitcounter = 10;
    bool brush_changed = true;
    bool box = false;
    bool edit = false;
    float spu = 7.0f;
    bool remeshed = false;
    CSGList workQueue[2];
    thread* worker = nullptr;
    int wqid = 0;
    while(window.open()){
        input.poll(frameBegin(i, t), camera);
    	waitcounter--;
    	
    	glm::vec3 at = camera.getEye() + 3.0f * camera.getAxis();
        
        mat4 brushMat = glm::translate(glm::mat4(), at);
        mat4 VP = camera.getVP();
        uni.MVP = VP * brushMat;
        uni.eye = vec4(camera.getEye(), 0.0f);
		uni.light_pos = vec4(camera.getEye(), 0.0f);
		unibuf.upload(&uni, sizeof(uni));
		
		if(glfwGetKey(window.getWindow(), GLFW_KEY_UP)){ bsize *= 1.1f; brush_changed = true;}
		else if(glfwGetKey(window.getWindow(), GLFW_KEY_DOWN)){ bsize *= 0.9f; brush_changed = true;}
		
		if(glfwGetKey(window.getWindow(), GLFW_KEY_1) && waitcounter < 0){ box = !box; waitcounter = 10; brush_changed = true;}
		if(glfwGetKey(window.getWindow(), GLFW_KEY_3)){ spu *= 1.01f;}
		else if(glfwGetKey(window.getWindow(), GLFW_KEY_4)){spu *= 0.99f;}
		
		if(brush_changed){
			unsigned sz = (vb.size()) ? (vb.size() >> 1) : 512;
			vb.clear();
			vb.reserve(sz);
			if(box)
				fillCells(vb, CSG(vec3(0.0f), vec3(bsize), &BOXADD, bsize, 1), spu);
			else
				fillCells(vb, CSG(vec3(0.0f), vec3(bsize), &SPHEREADD, bsize, 1), spu);
			brushMesh.update(vb);
			brush_changed = false;
		}
		
		colorProg.bind();
		brushMesh.draw();
		
		uni.MVP = VP;
		unibuf.upload(&uni, sizeof(uni));
		
		if(input.leftMouseDown() && waitcounter < 0){
			SDF_Base* type = &SPHERESADD;
			if(box) type = &BOXSADD;
			workQueue[wqid].push_back(CSG(at, vec3(bsize), type, bsize, 1));
			waitcounter = bsize * 30;
			edit = true;
		}
		else if(input.rightMouseDown() && waitcounter < 0){
			SDF_Base* type = &SPHERESUB;
			if(box) type = &BOXSUB;
			workQueue[wqid].push_back(CSG(at, vec3(bsize), type, bsize, 1)); 
			waitcounter = bsize * 30;
			edit = true;
		}
		
		if(edit && !worker){
			worker = new thread(&remesh, &csgary, &workQueue[wqid], &remeshed, spu);
			wqid = (wqid+1)%2;
			edit = false;
		}
		
		if(remeshed){
			worker->join();
			delete worker;
			worker = nullptr;
			csgary.update();
			remeshed = false;
		}
		
		//timer.begin();
		
		csgary.draw();
		
		//timer.endPrint();
        window.swap();
    }
    if(worker)worker->join();
    delete worker;
    brushMesh.destroy();
    return 0;
}

