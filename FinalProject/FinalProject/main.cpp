//CODE BY JASON TORRES

/*MAKE SURE "OBJ_loader.h" AND "teapot.obj" IS UNDER THE SAME DIRECTORY*/

#define GLM_ENABLE_EXPERIMENTAL


#include <iostream>
#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <string>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // value_ptr
#include <glm/gtx/string_cast.hpp>

#include "OBJ_loader.h"

#include "shader.h"
#include "mesh.h"
#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



using namespace std;
using namespace glm;

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 640;

const float MAX_FPS = 60.0;

//Perspective
const float FOV = 75;

//View
const float SPEED_COEFF = 1;
const float EYE_DISTANCE_FROM_POT = 4;

const float SPEED_COEFF_ROTATION = 0;

const float LIGHT_DISTANCE_FROM_POT = 1;
const float SPEED_COEFF_LIGHT_ORBIT = 0;

struct shaderTexStr {
    string RED_DIFFUSE = "texRedDiffuse";
    string RED_NORMAL= "texRedNormal";

    string GREEN_DIFFUSE = "texGreenDiffuse";
    string GREEN_NORMAL = "texGreenNormal";

    string BLUE_DIFFUSE = "texBlueDiffuse";
    string BLUE_NORMAL = "texBlueNormal";
}shaderTexStr;

struct FileParams {
    string textureFolder = string("Assets/Textures/");
    string textureFileSuffix = string(".png");

    string meshFolder = string("Assets/Meshes/");

    string currentMesh = ("monkey.obj");
}fileParams;
 
struct PublicShaderParams {
    //Ambient
    vec3 bgColor = vec3(0.885, 0.737, 0.69);

    //Light
    vec3 ambientLight = vec3(0.7, 0.7, 0.6); //vec3(0.45, 0.45, 0.6)
    int point = 0;
    vec3 lightPosDirty = vec3(1.0, 1.0, 1.0);
    vec3 lightColor = vec3(0.295, 0.245, 0.23);

    //View
    vec3 eyePosDirty = vec3(1.0, 0.5, 1.0);

    //Divide into seperate channels eventually
    float diffusePower = 1;
    int cell = 1;
    float diffuseThreshold = 0.7;

    //RED CHANNEL
    float redScale = 0.3;
    float redGloss = 0.773;
    float redSpecularStrength = 1;
    float redSpecularExponent = 4.15;
    string redDiffuse = string("texture_diffuse_rock");
    string redNormal = string("texture_normal_rock");

    //GREEN 
    float greenScale = 1;
    float greenGloss = 0;
    float greenSpecularStrength = 0;
    float greenSpecularExponent = 0;
    string greenDiffuse = string("texture_diffuse_grass");
    string greenNormal = string("texture_normal_grass");

    //BLUE CHANNEL
    float blueScale = 0.3;
    float blueGloss = 0.773;
    float blueSpecularStrength = 1;
    float blueSpecularExponent = 4.15;
    string blueDiffuse = string("texture_diffuse_rock");
    string blueNormal = string("texture_normal_rock");
    
}shaderParams;

/*------------------------------*/

static void glfw_error_handler(int error_code, const char* description) {
    printf("glfw err %d: %s", error_code, description);
}


/* Given a window, the key that was just signalled, and the action,
    check if the key is ESC and if it was pressed. If so set the given
    window to close
*/
static void key_callback(GLFWwindow* window, int key, int keycode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_1:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        case GLFW_KEY_2:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case GLFW_KEY_3:
            shaderParams.point = ~shaderParams.point;
            break;
        case GLFW_KEY_4:
            shaderParams.cell = ~shaderParams.cell;
            break;
        }
    }
}

unsigned int loadTexture(std::string texturePath, int textureUnit) {

    if (textureUnit > GL_MAX_TEXTURE_UNITS) {
        std::cout << "Inputted textureUnit is greater than " << (int)GL_MAX_TEXTURE_UNITS << "!" << std::endl;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + textureUnit);

    std::cout << "Active Texture Unit: " << std::hex << GL_TEXTURE0 + textureUnit << std::endl;
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int widthImg, heightImg, numColCh;
    unsigned char* bytes = stbi_load(texturePath.c_str(), &widthImg, &heightImg, &numColCh, 0);

    if (!bytes) {
        std::cout << "Texture " << texturePath.c_str() << " failed to load!" << std::endl;
    }
    else {
        std::cout << "Texture " << texturePath.c_str() << " was found!" << std::endl;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthImg, heightImg, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(bytes);

    return static_cast<unsigned int>(texture);
}


int main(void) {
    GLFWwindow* window;

    cout << "PROGRAM START\n---------------------------" << endl;

    glfwSetErrorCallback(glfw_error_handler);

    // Initialize the library
    if (!glfwInit()) {
        cout << "GLFW iitialization failed!" << endl;
        return -1;
    }
    cout << "GLFW initialized" << endl;

    //Create a window and OpenGL context
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Final Project: Jason Torres", NULL, NULL);
    //If window failed to create, terminate and return;
    if (!window) {
        glfwTerminate();
        return -1;
    }

    cout << "Window Created" << endl;

    //Set window's context to current
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        cout << "GLEW initialization failed!" << endl;
        return -1;
    }

    cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n" << endl;

    Shader channelTextureBlend("Shaders/colorchannelblend_vs.shader", "Shaders/colorchannelblend_fs.shader");


    //Subscribe the key_callback function to key actions
    glfwSetKeyCallback(window, key_callback);
    glEnable(GL_DEPTH_TEST);

    Model teapot(fileParams.meshFolder + fileParams.currentMesh);


    //Perspective
    float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    float near = 0.1, far = 100;
    mat4 projMatrix = perspective((float)radians(FOV), aspect, near, far);

    //View

    vec3 initEyePos = normalize(shaderParams.eyePosDirty) * EYE_DISTANCE_FROM_POT;
    vec4 initLight = vec4(normalize(shaderParams.lightPosDirty) * LIGHT_DISTANCE_FROM_POT, shaderParams.point);

    if (shaderParams.point == 0) {
        initLight = normalize(initLight);
    }

    unsigned int textureRed = loadTexture(fileParams.textureFolder + shaderParams.redDiffuse + fileParams.textureFileSuffix, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    unsigned int textureRedNormal = loadTexture(fileParams.textureFolder + shaderParams.redNormal + fileParams.textureFileSuffix, 1);
    glBindTexture(GL_TEXTURE_2D, 1);

    unsigned int textureGreen = loadTexture(fileParams.textureFolder + shaderParams.greenDiffuse + fileParams.textureFileSuffix, 2);
    glBindTexture(GL_TEXTURE_2D, 2);
    unsigned int textureGreenNormal = loadTexture(fileParams.textureFolder + shaderParams.greenNormal + fileParams.textureFileSuffix, 3);
    glBindTexture(GL_TEXTURE_2D, 3);

    unsigned int textureBlue = loadTexture(fileParams.textureFolder + shaderParams.blueDiffuse + fileParams.textureFileSuffix, 4);
    glBindTexture(GL_TEXTURE_2D, 4);
    unsigned int textureBlueNormal = loadTexture(fileParams.textureFolder + shaderParams.blueNormal + fileParams.textureFileSuffix, 5);
    glBindTexture(GL_TEXTURE_2D, 5);

    //Program
    const double fpsLimit = 1.0 / MAX_FPS;
    double lastFrameTime = 0; //number of seconds since the last frame
    /* Loop until the user closes the window */
    
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        if ((now - lastFrameTime) >= fpsLimit) {

            /* DRAW */
            {
                
                glClearColor(shaderParams.bgColor.x, shaderParams.bgColor.y, shaderParams.bgColor.z, 1.0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                
                vec3 eyePos = vec3(
                    initEyePos.x * -sin(now * SPEED_COEFF),
                    initEyePos.y,
                    initEyePos.z * cos(now * SPEED_COEFF));
                
                vec4 lightPos = vec4(vec3(
                    initLight.x * -sin(now * SPEED_COEFF_LIGHT_ORBIT),
                    initLight.y,
                    initLight.z * cos(now * SPEED_COEFF_LIGHT_ORBIT)),
                    shaderParams.point);

                mat4 viewMatrix = lookAt(eyePos, vec3(0,0,0), vec3(0, 1, 0));

                channelTextureBlend.use();

                //Center is translated to 0,0,0 anyway. Center of object is 0,0,0 after being rendered.
                mat4 transMatrix = translate(
                    mat4(1.0f),
                    vec3(0,0,0)
                );

                mat4 rotationMatrix = rotate(
                    mat4(1.0f),
                    (float)(now * SPEED_COEFF_ROTATION),
                    vec3(0.0f,1.0f,0.0f)
                );

                mat4 scaleMatrix = scale(
                    mat4(1.0f),
                    vec3(1.0f, 1.0f, 1.0f)
                );

                glUniform1i(glGetUniformLocation(channelTextureBlend.ID, shaderTexStr.RED_DIFFUSE.c_str()), 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureRed);

                glUniform1i(glGetUniformLocation(channelTextureBlend.ID, shaderTexStr.RED_NORMAL.c_str()), 1);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, textureRedNormal);

                glUniform1i(glGetUniformLocation(channelTextureBlend.ID, shaderTexStr.GREEN_DIFFUSE.c_str()), 2);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, textureGreen);

                glUniform1i(glGetUniformLocation(channelTextureBlend.ID, shaderTexStr.GREEN_NORMAL.c_str()), 3);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, textureGreenNormal);

                glUniform1i(glGetUniformLocation(channelTextureBlend.ID, shaderTexStr.BLUE_DIFFUSE.c_str()), 4);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, textureBlue);

                glUniform1i(glGetUniformLocation(channelTextureBlend.ID, shaderTexStr.BLUE_NORMAL.c_str()), 5);
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, textureBlueNormal);

                channelTextureBlend.setMat4("P", projMatrix);
                channelTextureBlend.setMat4("V", viewMatrix);

                channelTextureBlend.setMat4("T", transMatrix);
                channelTextureBlend.setMat4("R", rotationMatrix);
                channelTextureBlend.setMat4("S", scaleMatrix);

                channelTextureBlend.setVec4("light", lightPos);
                channelTextureBlend.setVec3("viewPos", eyePos);

                channelTextureBlend.setVec3("cameraPos", eyePos);
                channelTextureBlend.setVec3("lightColor", shaderParams.lightColor);
                channelTextureBlend.setVec3("ambientLight", shaderParams.ambientLight);

                channelTextureBlend.setFloat("diffusePower", shaderParams.diffusePower);
                channelTextureBlend.setFloat("diffuseThreshold", shaderParams.diffuseThreshold);

                channelTextureBlend.setInt("cell", shaderParams.cell);

                channelTextureBlend.setFloat("redScale", shaderParams.redScale);
                channelTextureBlend.setFloat("redGloss", shaderParams.redGloss);
                channelTextureBlend.setFloat("redSpecularExponent", shaderParams.redSpecularExponent);
                channelTextureBlend.setFloat("redSpecularStrength", shaderParams.redSpecularStrength);

                channelTextureBlend.setFloat("greenScale", shaderParams.greenScale);
                channelTextureBlend.setFloat("greenGloss", shaderParams.greenGloss);
                channelTextureBlend.setFloat("greenSpecularExponent", shaderParams.greenSpecularExponent);
                channelTextureBlend.setFloat("greenSpecularStrength", shaderParams.greenSpecularStrength);

                channelTextureBlend.setFloat("blueScale", shaderParams.blueScale);
                channelTextureBlend.setFloat("blueGloss", shaderParams.blueGloss);
                channelTextureBlend.setFloat("blueSpecularExponent", shaderParams.blueSpecularExponent);
                channelTextureBlend.setFloat("blueSpecularStrength", shaderParams.blueSpecularStrength);

                teapot.Draw(channelTextureBlend);

                glfwSwapBuffers(window);
            }
            /* END OF DRAW */

            //only set lastFrameTime when you actually draw something
            lastFrameTime = now;
        }
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
