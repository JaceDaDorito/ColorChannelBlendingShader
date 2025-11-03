#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec3 f_normal;
void main() {
    f_normal = normal;
    gl_Position = P * V * M * vec4(position, 1.0);
}

#shader fragment
#version 330 core

uniform bool triplanar;

in vec3 f_normal;
out vec4 outColor; // User-defined output variable for fragment color
void main () {
    vec3 N = normalize(f_normal);
    //vec3 color = (N+1.0)/2;
    float diffuseScalar1 = dot(N, vec3(0,1,0));
    //float diffuseScalar2 = dot()

    vec3 ambientColor = vec3(0,0,0.4);
    vec3 diffuseColor = diffuseScalar1 * vec3(1,0,0);
    vec3 underTone = -diffuseScalar1 * vec3(0.2, 0.1, 0.2);
    outColor = vec4(diffuseColor + underTone + ambientColor,1);
}
