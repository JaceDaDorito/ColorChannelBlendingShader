#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
uniform mat4 P;
uniform mat4 V;

uniform mat4 T;
uniform mat4 R;
uniform mat4 S;


out vec3 f_normal;
out vec3 w_normal;

void main() {

    mat4 M = S * R * T;

    f_normal = normal;
    w_normal = (transpose(inverse(R)) * vec4(normal, 1.0)).xyz;

    gl_Position = P * V * M * vec4(position, 1.0);
}

#shader fragment
#version 330 core

uniform bool triplanar;

in vec3 f_normal;
in vec3 w_normal;

out vec4 outColor; // User-defined output variable for fragment color

vec3 saturate(vec3 input){
    return clamp(input, 0, 1);
}

void main () {
    vec3 N = normalize(f_normal);
    vec3 WN = normalize(w_normal);

    vec3 blendedValue = saturate(pow(WN * 1.4, vec3(4,4,4)));
    //vec3 blendedT = 


    outColor = vec4(blendedValue,1);
}
