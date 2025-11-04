#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
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

#shader fragment
#version 330 core

uniform bool triplanar;

in vec4 w_pos;
in vec3 f_normal;
in vec3 w_normal;

//RED CHANNEL
uniform sampler2D texRed;

//GREEN CHANNEL
uniform sampler2D texGreen;

//BLUE CHANNEL
uniform sampler2D texBlue;

out vec4 outColor; // User-defined output variable for fragment color

vec3 saturate(vec3 inputVec){
    return clamp(inputVec, 0, 1);
}

void main () {
    vec3 N = normalize(f_normal);
    vec3 WN = normalize(w_normal);

    vec3 X = texture(texRed, w_pos.zy).rgb;
    vec3 Y = texture(texGreen, w_pos.zx).rgb;
    vec3 Z = texture(texBlue, w_pos.xy).rgb;

    vec3 blendedValue = WN;
    blendedValue = WN * 1.4;
    blendedValue = pow(blendedValue, vec3(4,4,4));
    blendedValue = saturate(blendedValue);

    vec3 blendedTextureColor = Z;
    blendedTextureColor = mix(blendedTextureColor, X, blendedValue.x);
    blendedTextureColor = mix(blendedTextureColor, Y, blendedValue.y);

    outColor = vec4(blendedTextureColor, 1);
}
