//CODE BY JASON TORRES

/*MAKE SURE "OBJ_loader.h" AND "teapot.obj" IS UNDER THE SAME DIRECTORY*/

#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

/*----- OBJ LOADING CODE ------*/

struct FileParams {
    std::string folder = std::string("Textures/");
    std::string fileSuffix = std::string(".png");
}fileParams;

struct PublicShaderParams {
    //Ambient
    glm::vec3 bgColor = glm::vec3(0.4, 0.6, 0.7);

    //Light
    glm::vec3 ambientLight = glm::vec3(0.65, 0.8, 0.8); //glm::vec3(0.45, 0.45, 0.6)
    int point = 0;
    glm::vec3 lightPosDirty = glm::vec3(1.0, 1.0, 1.0);
    glm::vec3 lightColor = glm::vec3(0.2, 0.3, 0.4);

    //View
    glm::vec3 eyePosDirty = glm::vec3(1.0, 1.0, 1.0);

    //Mesh
    std::string meshString = std::string("teapot.obj");

    //Divide into seperate channels eventually
    float diffusePower = 1;
    int cell = 1;
    float diffuseThreshold = 0.2;

    //RED CHANNEL
    std::string texRedString = std::string("texRock");

    //GREEN CHANNEL
    std::string texGreenString = std::string("texMoss");

    //BLUE CHANNEL
    std::string texBlueString = std::string("texRock");
    
}shaderParams;

struct Bounds {
    glm::vec3 center, min, max;
    float dia;
};

static Bounds computeBounds(const std::vector<objl::Vertex>& V, int n) {
    float
        min_x, max_x,
        min_y, max_y,
        min_z, max_z;
    min_x = max_x = V[0].Position.X;
    min_y = max_y = V[0].Position.Y;
    min_z = max_z = V[0].Position.Z;
    for (int i = 0; i < n; i++) {
        if (V[i].Position.X < min_x) min_x = V[i].Position.X;
        if (V[i].Position.X > max_x) max_x = V[i].Position.X;
        if (V[i].Position.Y < min_y) min_y = V[i].Position.Y;
        if (V[i].Position.Y > max_y) max_y = V[i].Position.Y;
        if (V[i].Position.Z < min_z) min_z = V[i].Position.Z;
        if (V[i].Position.Z > max_z) max_z = V[i].Position.Z;
    }
    struct Bounds bounds;
    bounds.min = glm::vec3(min_x, min_y, min_z);
    bounds.max = glm::vec3(max_x, max_y, max_z);
    std::cout << "Bounds = [" << glm::to_string(bounds.min) <<
        glm::to_string(bounds.max) << std::endl;
    bounds.center = (bounds.min + bounds.max) / 2.f;
    glm::vec3 diaVector = bounds.max - bounds.min;
    bounds.dia = glm::length(diaVector);
    std::cout << "radius: " << bounds.dia / 2 << std::endl;
    std::cout << "Center = " << glm::to_string(bounds.center) << std::endl;
    return bounds;
}

static std::vector<objl::Mesh> read_obj_file(const std::string& objFilePath) {
    objl::Loader* loader = new objl::Loader;
    // Load .obj File
    bool loadSuccess = loader->LoadFile(objFilePath);
    if (loadSuccess) {
        std::cout << "File " << objFilePath << " was found!" << std::endl;
    }
    return loader->LoadedMeshes;
}

struct {
    int nMeshes;
    unsigned int* vaos;
    struct Bounds* bounds;
    unsigned int* nelements;
}modelObject;

static void create_VAOs(std::vector<objl::Mesh>& loadedMeshes) {
    int nMeshes = loadedMeshes.size();
    std::cout << "Meshes #" << nMeshes << std::endl;
    modelObject.nMeshes = nMeshes;
    modelObject.vaos = new unsigned int[nMeshes];
    modelObject.nelements = new unsigned int[nMeshes];
    modelObject.bounds = new struct Bounds[nMeshes];
    int i = 0;
    for (const auto& _curMesh : loadedMeshes) {
        int nVerts = _curMesh.Vertices.size();
        int nBytesPerVertex = sizeof(_curMesh.Vertices[0]);
        int nFloatsPerVertex = nBytesPerVertex / sizeof(float);
        float* vertices = new float[nFloatsPerVertex * nVerts];
        int nElements = _curMesh.Indices.size();
        modelObject.nelements[i] = nElements;
        std::cout << "verts #" << nVerts << " size of floats per vertex: " <<
            nFloatsPerVertex << std::endl;
        std::cout << "elements #" << nElements << std::endl;
        modelObject.bounds[i] = computeBounds(_curMesh.Vertices, nVerts);
        memcpy(vertices, _curMesh.Vertices.data(), nBytesPerVertex * nVerts);
        unsigned int vao;
        glGenVertexArrays(1, &vao);
        modelObject.vaos[i] = vao;
        glBindVertexArray(vao);
        unsigned int vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, nBytesPerVertex * nVerts, vertices,
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, nBytesPerVertex, 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, nBytesPerVertex, (void*)(3* sizeof(float)));
        // To enable Texture coordinate attributes uncomments the followinglines.
        //glEnableVertexAttribArray(2);
        //glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, nBytesPerVertex, (void*)(6 * sizeof(float)));
        glBindVertexArray(0);
        i++;
    }
}

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

struct ShaderProgramSource {
    std::string VertexSource;
    std::string FragmentSource;
};



static ShaderProgramSource ParseShader(const std::string& filepath) {
    std::ifstream stream(filepath);

    enum class ShaderType {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while (std::getline(stream, line)) {
        if (line.find("#shader") != std::string::npos) {

            if (line.find("vertex") != std::string::npos) {
                type = ShaderType::VERTEX;
            }

            else if (line.find("fragment") != std::string::npos) {
                type = ShaderType::FRAGMENT;
            }
        }
        else {
            ss[(int)type] << line << '\n';
        }
    }

    return{ ss[0].str(), ss[1].str() };
}

static void checkForCompileTimeError(unsigned int shaderObj) {
    int success;
    glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(shaderObj, sizeof(message), NULL, message);
        std::cout << "Failed to compile Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(shaderObj);
    }
}

static unsigned int CompileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " << ((type == GL_VERTEX_SHADER) ? "VertexShader" : "Fragment Shader") << std::endl; std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static unsigned int CreateProgram(const std::string& vertexShader, const
    std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    checkForCompileTimeError(vs);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    checkForCompileTimeError(fs);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

unsigned int loadTexture(std::string texturePath, int textureUnit) {

    if (textureUnit > GL_MAX_TEXTURE_UNITS) {
        std::cout << "Inputted textureUnit is greater than " << (int)GL_MAX_TEXTURE_UNITS  << "!" << std::endl;
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

glm::vec3 computeNormal(glm::vec3 const& a, glm::vec3 const& b, glm::vec3 const& c){
    return glm::normalize(glm::cross(c - a, b - a));
}

int main(void) {
    GLFWwindow* window;

    std::cout << "PROGRAM START\n---------------------------" << std::endl;

    glfwSetErrorCallback(glfw_error_handler);

    // Initialize the library
    if (!glfwInit()) {
        std::cout << "GLFW iitialization failed!" << std::endl;
        return -1;
    }



    std::cout << "GLFW initialized" << std::endl;

    //Create a window and OpenGL context
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Final Project: Jason Torres", NULL, NULL);
    //If window failed to create, terminate and return;
    if (!window) {
        glfwTerminate();
        return -1;
    }

    std::cout << "Window Created" << std::endl;

    //Set window's context to current
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW initialization failed!" << std::endl;
        return -1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n" << std::endl;
    

    ShaderProgramSource sources = ParseShader("channeltextureblend.shader");
    std::cout << "VERTEX" << std::endl;
    std::cout << sources.VertexSource << std::endl;
    std::cout << "FRAGMENT" << std::endl;
    std::cout << sources.FragmentSource << std::endl;
    unsigned int ChannelTextureBlend = CreateProgram(sources.VertexSource, sources.FragmentSource);


    //Subscribe the key_callback function to key actions
    glfwSetKeyCallback(window, key_callback);
    glEnable(GL_DEPTH_TEST);

    std::vector<objl::Mesh> meshes = read_obj_file("teapot.obj");
    int nMeshes = meshes.size();
    create_VAOs(meshes);

    

    //Perspective
    float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    float near = 0.1, far = 100;
    glm::mat4 projMatrix = glm::perspective((float)glm::radians(FOV), aspect, near, far);

    //View

    glm::vec3 initEyePos = glm::normalize(shaderParams.eyePosDirty) * EYE_DISTANCE_FROM_POT;
    glm::vec4 initLight = glm::vec4(glm::normalize(shaderParams.lightPosDirty) * LIGHT_DISTANCE_FROM_POT, shaderParams.point);

    if (shaderParams.point == 0) {
        initLight = glm::normalize(initLight);
    }

    unsigned int textureRed = loadTexture(fileParams.folder + shaderParams.texRedString + fileParams.fileSuffix, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    unsigned int textureGreen = loadTexture(fileParams.folder + shaderParams.texGreenString + fileParams.fileSuffix, 1);
    glBindTexture(GL_TEXTURE_2D, 1);
    unsigned int textureBlue = loadTexture(fileParams.folder + shaderParams.texBlueString + fileParams.fileSuffix, 2);
    glBindTexture(GL_TEXTURE_2D, 2);

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
                
                
                glm::vec3 eyePos = glm::vec3(
                    initEyePos.x * -glm::sin(now * SPEED_COEFF),
                    initEyePos.y,
                    initEyePos.z * glm::cos(now * SPEED_COEFF));
                
                glm::vec4 lightPos = glm::vec4(glm::vec3(
                    initLight.x * -glm::sin(now * SPEED_COEFF_LIGHT_ORBIT),
                    initLight.y,
                    initLight.z * glm::cos(now * SPEED_COEFF_LIGHT_ORBIT)),
                    shaderParams.point);

                glm::mat4 viewMatrix = glm::lookAt(eyePos, glm::vec3(0,0,0), glm::vec3(0, 1, 0));

                glUseProgram(ChannelTextureBlend);


                //Texture
                glUniform1i(glGetUniformLocation(ChannelTextureBlend, "texRed"), 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureRed);

                glUniform1i(glGetUniformLocation(ChannelTextureBlend, "texGreen"), 1);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, textureGreen);

                glUniform1i(glGetUniformLocation(ChannelTextureBlend, "texBlue"), 2);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, textureBlue);

                for (int i = 0; i < modelObject.nMeshes; i++) {

                    //Center is translated to 0,0,0 anyway. Center of object is 0,0,0 after being rendered.
                    glm::mat4 transMatrix = glm::translate(
                        glm::mat4(1.0f),
                        -modelObject.bounds[i].center
                    );

                    glm::mat4 rotationMatrix = glm::rotate(
                        glm::mat4(1.0f),
                        (float)(now * SPEED_COEFF_ROTATION),
                        glm::vec3(1.0f,0.0f,0.0f)
                    );

                    rotationMatrix = glm::rotate(
                        rotationMatrix,
                        (float)(now * SPEED_COEFF_ROTATION * 0.2),
                        glm:: vec3(0.0f, 1.0f, 1.0f)
                    );

                    glm::mat4 scaleMatrix = glm::scale(
                        glm::mat4(1.0f),
                        glm::vec3(1.0f / (modelObject.bounds[i].dia/2))
                    );

                    GLint matrixPLocation = glGetUniformLocation(ChannelTextureBlend, "P");
                    GLint matrixVLocation = glGetUniformLocation(ChannelTextureBlend, "V");
                    GLint vec3CameraPosLocation = glGetUniformLocation(ChannelTextureBlend, "cameraPos");
                    GLint vec4LightLocation = glGetUniformLocation(ChannelTextureBlend, "light");
                    GLint vec3LightColorLocation = glGetUniformLocation(ChannelTextureBlend, "lightColor");
                    GLint vec3AmbientLightLocation = glGetUniformLocation(ChannelTextureBlend, "ambientLight");
                    GLint floatDiffusePower = glGetUniformLocation(ChannelTextureBlend, "diffusePower");
                    GLint floatDiffuseThreshold = glGetUniformLocation(ChannelTextureBlend, "diffuseThreshold");
                    GLint intCellShadingThreshold = glGetUniformLocation(ChannelTextureBlend, "cell");

                    GLint matrixTLocation = glGetUniformLocation(ChannelTextureBlend, "T");
                    GLint matrixRLocation = glGetUniformLocation(ChannelTextureBlend, "R");
                    GLint matrixSLocation = glGetUniformLocation(ChannelTextureBlend, "S");

                    glUniformMatrix4fv(matrixPLocation, 1, GL_FALSE, glm::value_ptr(projMatrix));
                    glUniformMatrix4fv(matrixVLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
                    glUniform3f(vec3CameraPosLocation, eyePos.x, eyePos.y, eyePos.z);
                    glUniform4f(vec4LightLocation, lightPos.x, lightPos.y, lightPos.z, lightPos.w);
                    glUniform3f(vec3LightColorLocation, shaderParams.lightColor.x, shaderParams.lightColor.y, shaderParams.lightColor.z);
                    glUniform3f(vec3AmbientLightLocation, shaderParams.ambientLight.x, shaderParams.ambientLight.y, shaderParams.ambientLight.z);
                    glUniform1f(floatDiffusePower, shaderParams.diffusePower);
                    glUniform1f(floatDiffuseThreshold, shaderParams.diffuseThreshold);
                    glUniform1i(intCellShadingThreshold, shaderParams.cell);

                    glUniformMatrix4fv(matrixTLocation, 1, GL_FALSE, glm::value_ptr(transMatrix));
                    glUniformMatrix4fv(matrixRLocation, 1, GL_FALSE, glm::value_ptr(rotationMatrix));
                    glUniformMatrix4fv(matrixSLocation, 1, GL_FALSE, glm::value_ptr(scaleMatrix));


                    glBindVertexArray(modelObject.vaos[i]);
                    glDrawArrays(GL_TRIANGLES, 0, meshes[i].Vertices.size());
                }

                
                glBindVertexArray(0);
                glUseProgram(0);

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
