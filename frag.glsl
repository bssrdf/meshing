#version 430 core

flat in vec3 flocation;
flat in int fid;
flat in vec3 fnormal;
flat in int valid;

out vec4 out_color;

uniform mat4 MVP;
uniform vec3 light_position;
uniform vec3 eye;
uniform int seed;

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 id_to_color(){
    float hue = sin(fid * 0.01) * 360.0;
    float sat = 0.9;
    float val = 0.9;
    return hsv2rgb(vec3(hue, sat, val));
}

float randUni(inout uint f){
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10);
}

void main(){
    if(valid == 0)
        discard;
    uint s = uint(dot(gl_FragCoord.xy, gl_PointCoord.xy) * seed);
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float dis = length(coord);
    if(dis > randUni(s)){
        discard;
    }
    vec3 N = normalize(fnormal);
#ifdef NORMAL
    out_color = vec4(0.5 * N + 0.5, 1.0);
    return;
#endif
vec3 ambient = vec3(0.001, 0.0005, 0.002);
	vec3 base_color = id_to_color();
	vec3 lvec = light_position - flocation;
	vec3 L = normalize(lvec);
	vec3 H = normalize(L + normalize(eye - flocation));
	float D = max(0.0f, dot(N, L));
	float S = ( D > 0.0f ) ? pow(max(0.f, dot(H, N)), 64.f) : 0.f;
	vec3 color = ambient + (D * base_color + S) / dot(lvec, lvec);
	out_color = vec4(pow(color, vec3(1.0f/2.2f)), 1.0f);
}
