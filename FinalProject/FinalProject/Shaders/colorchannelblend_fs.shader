#version 330 core

uniform bool triplanar;

in vec4 w_pos;
in vec3 f_normal;
in vec3 w_normal;
in mat3 TBN;

in vec3 debugVector;

uniform vec3 cameraPos;

uniform vec3 lightColor;
uniform vec3 ambientLight;
uniform vec4 light;
uniform vec3 viewPos;

uniform float diffusePower;
uniform int cell;
uniform float diffuseThreshold;

//RED CHANNEL
uniform float redScale;
uniform float redGloss;
uniform float redSpecularStrength;
uniform float redSpecularExponent;
uniform sampler2D texRedDiffuse;
uniform sampler2D texRedNormal;

//GREEN CHANNEL
uniform float greenScale;
uniform float greenGloss;
uniform float greenSpecularStrength;
uniform float greenSpecularExponent;
uniform sampler2D texGreenDiffuse;
uniform sampler2D texGreenNormal;

//BLUE CHANNEL
uniform float blueScale;
uniform float blueGloss;
uniform float blueSpecularStrength;
uniform float blueSpecularExponent;
uniform sampler2D texBlueDiffuse;
uniform sampler2D texBlueNormal;

out vec4 outColor; // User-defined output variable for fragment color

vec3 saturate(vec3 inputVec){
    return clamp(inputVec, 0, 1);
}

void main () {
    vec3 N = normalize(f_normal);
    vec3 WN = normalize(TBN[2]);

    vec3 tangentLightPos = TBN * light.xyz;
    vec3 tangentViewPos = TBN * viewPos;

    vec3 X = texture(texRedDiffuse, w_pos.zy * redScale).rgb;
    vec3 Y = texture(texGreenDiffuse, w_pos.zx * greenScale).rgb;
    vec3 Z = texture(texBlueDiffuse, w_pos.xy * blueScale).rgb;

    vec3 X_N = texture(texRedNormal, w_pos.zy * redScale).rgb;
    vec3 Y_N = texture(texGreenNormal, w_pos.zx * greenScale).rgb;
    vec3 Z_N = texture(texBlueNormal, w_pos.xy * blueScale).rgb;

    vec3 blendedValue = WN;
    blendedValue = WN * 1.4;
    blendedValue = pow(blendedValue, vec3(4,4,4));
    blendedValue = saturate(blendedValue);

    vec3 blendedTextureColor = Z;
    blendedTextureColor = mix(blendedTextureColor, X, blendedValue.x);
    blendedTextureColor = mix(blendedTextureColor, Y, blendedValue.y);

    vec3 blendedNormalTextureColor =  Z_N;
    blendedNormalTextureColor = mix(blendedNormalTextureColor, X_N, blendedValue.x);
    blendedNormalTextureColor = mix(blendedNormalTextureColor, Y_N, blendedValue.y);

    float depth = 1;
    vec3 weights = blendedTextureColor;
    float map = max(max(weights.r, weights.g), weights.b) - depth;
    weights.r = max(weights.r - map, 0);
    weights.g = max(weights.g - map, 0);
    weights.b = max(weights.b - map, 0);
    weights = normalize(weights);
    weights = round(weights * 2) / 2;
    weights = normalize(weights);


    vec3 specularStrengthVec = vec3(redSpecularStrength, greenSpecularStrength, blueSpecularStrength) * weights;
    float specularStrength = specularStrengthVec.x + specularStrengthVec.y + specularStrengthVec.z;

    vec3 specularExponentVec = vec3(redSpecularExponent, greenSpecularExponent, blueSpecularExponent) * weights;
    float specularExponent = specularExponentVec.x + specularExponentVec.y + specularExponentVec.z;

    vec3 glossVec = vec3(redGloss, greenGloss, blueGloss) * weights;
    float gloss = glossVec.x + glossVec.y + glossVec.z;

    //If w = 0, its directional. If w = 1, its point.
    vec3 lightVector = normalize(tangentLightPos + w_pos.xyz);
    lightVector = normalize(lightVector);

    vec3 V = normalize(cameraPos - w_pos.xyz);
    vec3 H = normalize(lightVector + V);

    float lambertian = clamp(dot(lightVector, blendedNormalTextureColor), 0, 1);
    specularExponent = pow(gloss * specularExponent, 2) + 1;
    vec3 specularLight =  clamp(dot(H, blendedNormalTextureColor), 0, 1) * vec3(lambertian > 0, lambertian > 0, lambertian > 0);
    specularLight = vec3(pow(specularLight.x, specularExponent), pow(specularLight.y, specularExponent), pow(specularLight.z, specularExponent)) * gloss; //missing attenuation
    specularLight = specularLight * specularStrength * lightColor;
   


    //If cell = 1, use stepped lambertian. If cell = 0, use just lambertian
    vec3 diffuse = (step(diffuseThreshold, lambertian) * cell + lambertian * (1 - cell)) * lightColor * diffusePower;

    outColor = vec4((blendedTextureColor + diffuse) * ambientLight, 1); ///specs arent working
}
