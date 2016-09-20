#version 430 core

layout(local_size_x = 128) in;

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

layout(binding=0) buffer META_BUF
{
    int SDF_SZ, VERT_SZ, OCT_SZ, vert_tail;
    vec3 center; float radius;
    int sdf_tail;
};

layout(binding=1) buffer OCT_BUF
{
    int L[];
};

layout(binding=2) buffer SDF_BUF
{
    sdf items[];
};

layout(binding=3) buffer VERT_BUF
{
    vertex verts[];
};



float sphere(vec3 ray, float rad){
    return length(ray) - rad;
}

float map(vec3 location, int id){
    int type = items[id].type;
    location = location - items[id].location;
    vec3 scale = items[id].scale;
    switch(type){
        case SDF_SPHERE:
            return sphere(location, scale.x);
        default: 
            return radius * 100.0;
    }
}

vec3 map_normal(vec3 location, int id){
    vec3 eps = vec3(0.0001, 0.0, 0.0);
    return normalize(vec3(
        map(location + eps, id) - map(location - eps, id),
        map(location + eps.yxy, id) - map(location - eps.yxy, id),
        map(location + eps.yyx, id) - map(location - eps.yyx, id)
        ));
}

void make_point(vec3 location, int id){
    vec3 N = map_normal(location, id);
    for(int i = 0; i < 2; i++){
        float dis = map(location, id);
        location -= N * dis;
    }
    int obj_id = items[id].id;
    if(vert_tail < VERT_SZ){
        int tail = atomicAdd(vert_tail, 1);
        verts[tail] = vertex(location, obj_id, N, 0.0);
    }
}

uint depth_offset(uint depth){
    uint of = 0;
    uint sz = SDF_SZ >> 1;
    uint ct = 8;
    for(uint i = 1; i < depth; i++){
        of += sz * ct;
        sz = sz >> 1;
        ct = ct << 3;
    }
    return of;
}

void do_level(uint pdepth, uint cdepth, uint prev_group, uint group, vec3 loc, float rad){
    uint sz1 = SDF_SZ >> pdepth;
    uint sz2 = SDF_SZ >> cdepth;
    uint tail = 0;
    uint basea = depth_offset(pdepth);
    uint baseb = depth_offset(cdepth);
    for(uint i = 0; i < sz1 && tail < sz2; i++){
        int id = L[basea + prev_group * sz1 + i];
        if(id < 0)
            break;
        float dis = abs(map(loc, id));
        if(dis < rad){
            L[baseb + group * sz2 + tail] = id;
            tail++;
        }
    }
    for(; tail < sz2; tail++)
        L[baseb + group * sz2 + tail] = -1;
}

void do_base(uint group, vec3 loc, float rad){
    uint tail = 0;
    uint sz = SDF_SZ >> 1;
    for(int i = 0; i < sdf_tail && tail < sz; i++){
        float dis = abs(map(loc, i));
        if(dis < rad){
            L[group * sz + tail] = i;
            tail++;
        }
    }
    for(; tail < sz; tail++)
        L[group * sz + tail] = -1;
}

void do_leaf(uint pdepth, uint prev_group, vec3 loc, float rad){
    uint basea = depth_offset(pdepth);
    uint sz = SDF_SZ >> pdepth;
    for(uint i = 0; i < sz; i++){
        int id = L[basea + prev_group * sz + i];
        if(id < 0)
            break;
        float dis = abs(map(loc, id));
        if(dis < rad){
            make_point(loc, id);
        }
    }
}

void main(){
    
    vec3 newpos = center;
    float rad = radius;
    uint duplicates = 8 << 15;
    
    uint prev_group = 0;
    for(uint depth = 1; depth <= 6; depth++){
        rad *= 0.5;
        float qlen = 1.732051f * rad;
        duplicates = duplicates >> 3;
        uint group = gl_GlobalInvocationID.x / duplicates;
        uint offset = gl_GlobalInvocationID.x % duplicates;
        bool leader = offset == 0;
        
        newpos.x += ((group & 1) == 1) ? rad : -rad;
        newpos.y += ((group & 2) == 2) ? rad : -rad;
        newpos.z += ((group & 4) == 4) ? rad : -rad;
        
        if(leader){
            if(depth == 1)
                do_base(group, newpos, qlen);
            else if(depth == 6)
                do_leaf(depth-1, prev_group, newpos, qlen);
            else
                do_level(depth-1, depth, prev_group, group, newpos, qlen);
        }
        prev_group = group;
        barrier();
    }
}

