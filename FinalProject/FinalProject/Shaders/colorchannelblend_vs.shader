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
out mat3 TBN;

out vec3 debugVector;


void main() {

    mat4 M = S * R * T;

    mat3 normalMatrix = transpose(inverse(mat3(M)));
    vec3 W_T = normalize(normalMatrix * tangent);
    vec3 W_N = normalize(normalMatrix * normal);
    vec3 W_B = normalize(cross(W_N, W_T));

    TBN = mat3(W_T, W_B, W_N);

    f_normal = normal;

    debugVector = W_B;

    w_pos = M * vec4(position, 1.0);
    gl_Position = P * V * w_pos;
}