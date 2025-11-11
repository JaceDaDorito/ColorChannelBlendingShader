//CODE BY JASON TORRES

/*MAKE SURE "OBJ_loader.h" AND "teapot.obj" IS UNDER THE SAME DIRECTORY*/

#define GLM_ENABLE_EXPERIMENTAL


#include <iostream>
#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shader.h"
#include "mesh.h"
#include "model.h"

using namespace std;
using namespace glm;

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 640;

const float MAX_FPS = 60.0;

//Perspective
const float FOV = 75;

//View
const float SPEED_COEFF = 1;
const float EYE_DISTANCE_FROM_POT = 3;

const float SPEED_COEFF_ROTATION = 0;

const float LIGHT_DISTANCE_FROM_POT = 1;
const float SPEED_COEFF_LIGHT_ORBIT = 3;

struct FileParams {
    string folder = string("Textures/");
    string fileSuffix = string(".png");
}fileParams;
 
struct PublicShaderParams {
    //Ambient
    vec3 bgColor = vec3(0.4, 0.6, 0.7);

    //Light
    vec3 ambientLight = vec3(0.65, 0.8, 0.8); //vec3(0.45, 0.45, 0.6)
    int point = 0;
    vec3 lightPosDirty = vec3(1.0, 1.0, 1.0);
    vec3 lightColor = vec3(0.2, 0.3, 0.4);

    //View
    vec3 eyePosDirty = vec3(1.0, 1.0, 1.0);

    //Divide into seperate channels eventually
    float diffusePower = 1;
    int cell = 1;
    float diffuseThreshold = 0.2;

    //RED CHANNEL

    //GREEN CHANNEL

    //BLUE CHANNEL
    
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

    Model teapot("Assets/teapot.obj");


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
                    vec3(1.0f,0.0f,0.0f)
                );

                rotationMatrix = rotate(
                    rotationMatrix,
                    (float)(now * SPEED_COEFF_ROTATION * 0.2),
                        vec3(0.0f, 1.0f, 1.0f)
                );

                mat4 scaleMatrix = scale(
                    mat4(1.0f),
                    vec3(1.0f, 1.0f, 1.0f)
                );

                channelTextureBlend.setMat4("P", projMatrix);
                channelTextureBlend.setMat4("V", viewMatrix);

                channelTextureBlend.setMat4("T", transMatrix);
                channelTextureBlend.setMat4("R", rotationMatrix);
                channelTextureBlend.setMat4("S", scaleMatrix);

                channelTextureBlend.setVec4("light", lightPos);

                channelTextureBlend.setVec3("cameraPos", eyePos);
                channelTextureBlend.setVec3("lightColor", shaderParams.lightColor);
                channelTextureBlend.setVec3("ambientLight", shaderParams.ambientLight);

                channelTextureBlend.setFloat("diffusePower", shaderParams.diffusePower);
                channelTextureBlend.setFloat("diffuseThreshold", shaderParams.diffuseThreshold);

                channelTextureBlend.setInt("cell", shaderParams.cell);

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
