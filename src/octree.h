#ifndef OCTREE_H
#define OCTREE_H

#include "csg.h"
#include "stdlib.h"
#include <unordered_set>
#include <mutex>
#include "math.h"

namespace oct{

#define LEAF_DEPTH 5

struct OctNode;
struct leafData;

static std::vector<leafData*> ldata, n_upload;
static std::unordered_set<leafData*> n_remesh;
static std::mutex leaf_mtex, remesh_mtex, upload_mtex;
static CSGList csg_ops;

struct leafData{
    VertexBuffer vb;
    CSGList items;
    Mesh mesh;
    glm::vec3 center;
    float length;
    inline void remesh(){
        vb.clear();
        fillCells(vb, items, center, length);
    }
    inline void upload(){
        mesh.update(vb);
    }
};

static float octlen = 8.0f;

struct OctNode{
    OctNode* children;
    leafData* data;
    glm::vec3 center;
    char depth;
    float length(){
        float len = octlen;
        for(char i = 0; i < depth; i++)
            len *= 0.5f;
        return len;
    }
    float qlen(){
        // ratio of cube's side to diagonal: sqrt(len^2 + len^2 + len^2) => sqrt(len^2 * 3) => x * sqrt(3)
        return 1.732051f * this->length();
    }
    // always instantiate root node with depth 0
    OctNode(const glm::vec3& c, char d=0)
        : children(nullptr), data(nullptr), center(c), depth(d){
        const float len = this->length();
        if(depth == LEAF_DEPTH){ // if a leaf
            data = new leafData();
            data->center = this->center;
            data->length = len;
            std::lock_guard<std::mutex> guard(leaf_mtex);
            ldata.push_back(data);
        }
    }
    ~OctNode(){
        if(!depth){
            // clean up our csgs
            for(CSG* i : csg_ops)
                delete i;
            csg_ops.clear();
            // clean up our leaf datas
            for(auto* i : ldata){
                i->mesh.destroy();
                delete i;
            }
            ldata.clear();
            n_remesh.clear();
            n_upload.clear();
        }
        if(children){
            for(int i = 0; i < 8; i++)
                (children + i)->~OctNode();
            free(children);
        }
    }
    inline bool isLeaf(){return !children;}
    inline void makeChildren(){
        if(children)return;
        const float nlen = length() * 0.5f;
        children = (OctNode*)malloc(sizeof(OctNode) * 8);
        for(int i = 0; i < 8; i++){
            glm::vec3 n_c(center);
            n_c.x += (i&4) ? nlen : -nlen;
            n_c.y += (i&2) ? nlen : -nlen;
            n_c.z += (i&1) ? nlen : -nlen;
            new (children + i) OctNode(n_c, depth + 1);
        }
    }
    // always pass an item on the heap here
    inline void insert(CSG* item){
        if(item->func(center) >= qlen()){
            if(!depth)
                delete item;
            return;
        }
        if(!depth)
            csg_ops.push_back(item);
        if(data){
            data->items.push_back(item);
            std::lock_guard<std::mutex> guard(remesh_mtex);
            n_remesh.insert(data);
            return;
        }
        makeChildren();
        for(int i = 0; i < 8; i++)
            children[i].insert(item);
    }

};

void remesh_nodes(){
    if(!n_remesh.size())
        return;
    std::lock_guard<std::mutex> guard(remesh_mtex);
    for(auto* o : n_remesh){
        o->remesh();
    }
    std::lock_guard<std::mutex> guard2(upload_mtex);
    n_upload.insert(n_upload.end(), n_remesh.begin(), n_remesh.end());
    n_remesh.clear();
}

// only use main thread for this!
void upload_meshes(){
    if(!n_upload.size())
        return;
    std::lock_guard<std::mutex> guard(upload_mtex);
    for(auto* o : n_upload){
        o->upload();
    }
    n_upload.clear();
}

// only use main thread for this!
void draw_points(){
    std::lock_guard<std::mutex> guard(leaf_mtex);
    for(auto* o : ldata){
        o->mesh.draw();
    }
}

};

#endif // OCTREE_H
