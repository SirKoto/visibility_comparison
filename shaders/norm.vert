#version 430 core

layout(location = 0) in vec3 iPos;
layout(location = 1) in vec3 iNorm;
layout(location = 2) in vec2 iUv;
layout(location = 3) in vec2 iOffsetXZ;


layout(location = 0) uniform mat4 M;
layout(location = 1) uniform mat4 V;
layout(location = 2) uniform mat4 P;

out vec3 normWorld;
out vec2 uv;

void main(){
    
    uv = iUv;
    normWorld = (V * M * vec4(iNorm, 0.0)).xyz;

    vec4 posWorld = M * vec4(iPos, 1.0) + vec4(iOffsetXZ.x, 0, iOffsetXZ.y, 0);
    gl_Position = P * V * posWorld;
}
