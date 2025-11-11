#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 texCoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 P;
uniform mat4 V;

uniform mat4 T;
uniform mat4 R;
uniform mat4 S;

out vec4 w_pos;
out vec3 f_normal;
out vec3 w_normal;

void main() {

    mat4 M = S * R * T;

    f_normal = normal;
    w_normal = (transpose(inverse(R)) * vec4(normal, 1.0)).xyz;

    w_pos = M * vec4(position, 1.0);
    gl_Position = P * V * w_pos;
}