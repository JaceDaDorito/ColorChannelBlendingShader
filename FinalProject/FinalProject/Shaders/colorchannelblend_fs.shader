#version 330 core

uniform bool triplanar;

in vec4 w_pos;
in vec3 f_normal;
in vec3 w_normal;
//in vec4 TBN;

uniform vec3 cameraPos;

uniform vec4 light;
uniform vec3 lightColor;
uniform vec3 ambientLight;

uniform float diffusePower;
uniform int cell;
uniform float diffuseThreshold;

//RED CHANNEL
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

//GREEN CHANNEL
uniform sampler2D texture_diffuse2;
uniform sampler2D texture_normal2;

//BLUE CHANNEL
uniform sampler2D texture_diffuse3;
uniform sampler2D texture_normal3;

out vec4 outColor; // User-defined output variable for fragment color

vec3 saturate(vec3 inputVec){
    return clamp(inputVec, 0, 1);
}

void main () {
    vec3 N = normalize(f_normal);
    vec3 WN = normalize(w_normal);

    vec3 X = texture(texture_diffuse1, w_pos.zy).rgb;
    vec3 Y = texture(texture_diffuse2, w_pos.zx).rgb;
    vec3 Z = texture(texture_diffuse3, w_pos.xy).rgb;

    /*vec3 X_N = texture(texture_normal1, w_pos.zy).rgb;
    vec3 Y_N = texture(texture_normal2, w_pos.zx).rgb;
    vec3 Z_N = texture(texture_normal3, w_pos.xy).rgb;*/

    vec3 blendedValue = WN;
    blendedValue = WN * 1.4;
    blendedValue = pow(blendedValue, vec3(4,4,4));
    blendedValue = saturate(blendedValue);

    vec3 blendedTextureColor = Z;
    blendedTextureColor = mix(blendedTextureColor, X, blendedValue.x);
    blendedTextureColor = mix(blendedTextureColor, Y, blendedValue.y);

    /*vec3 blendedNormalTextureColor =  Z_N;
    blendedNormalTextureColor = mix(blendedNormalTextureColor, X_N, blendedValue.x);
    blendedNormalTextureColor = mix(blendedNormalTextureColor, Y_N, blendedValue.y);*/

    //If w = 0, its directional. If w = 1, its point.
    vec3 lightVector= light.xyz - (w_pos.xyz * light.w);
    lightVector = normalize(lightVector);

    vec3 V = normalize(cameraPos - w_pos.xyz);
    vec3 H = normalize(lightVector + V);
    float lambertian = clamp(dot(lightVector,WN), 0, 1);

    //If cell = 1, use stepped lambertian. If cell = 0, use just lambertian
    vec3 diffuse = (step(diffuseThreshold, lambertian) * cell + lambertian * (1 - cell)) * lightColor * diffusePower;

    outColor = vec4((blendedTextureColor + diffuse) * ambientLight, 1);
}
