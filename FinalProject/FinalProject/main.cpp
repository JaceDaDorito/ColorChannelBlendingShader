//CODE BY JASON TORRES

/*MAKE SURE "OBJ_loader.h" AND "teapot.obj" IS UNDER THE SAME DIRECTORY*/

#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <string>
#include <sstream>

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
const float EYE_DISTANCE_FROM_POT = 3;

const float SPEED_COEFF = 0;
const float SPEED_COEFF_ROTATION = 2;

const float BG_RED = 0.3;
const float BG_GREEN = 0.3;
const float BG_BLUE = 0.3;


/*----- OBJ LOADING CODE ------*/

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
    unsigned int ShaderProgram = CreateProgram(sources.VertexSource, sources.FragmentSource);


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
    glm::vec3 initEyePos = glm::normalize(glm::vec3(1.0, 1.0, 1.0)) * EYE_DISTANCE_FROM_POT;

    //Texture


    GLuint tex0Uni = glGetUniformLocation(ShaderProgram, "tex0");
    std::cout << tex0Uni << std::endl;

    //Program
    const double fpsLimit = 1.0 / MAX_FPS;
    double lastFrameTime = 0; //number of seconds since the last frame
    /* Loop until the user closes the window */
    glUseProgram(ShaderProgram);
    
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        if ((now - lastFrameTime) >= fpsLimit) {

            /* DRAW */
            {
                
                glClearColor(BG_RED, BG_GREEN, BG_BLUE, 1.0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                
                glm::vec3 eyePos = glm::vec3(initEyePos.x * -glm::sin(now * SPEED_COEFF), initEyePos.y, initEyePos.z * glm::cos(now * SPEED_COEFF));
                glm::mat4 viewMatrix = glm::lookAt(eyePos, glm::vec3(0,0,0), glm::vec3(0, 1, 0));

                glUseProgram(ShaderProgram);


                for (int i = 0; i < modelObject.nMeshes; i++) {

                    //Center is translated to 0,0,0 anyway. Center of object is 0,0,0 after being rendered.
                    glm::mat4 transMatrix = glm::translate(
                        glm::mat4(1.0f),
                        -modelObject.bounds[i].center
                    );

                    glm::mat4 rotationMatrix = glm::rotate(
                        glm::mat4(1.0f),
                        (float)(now * SPEED_COEFF_ROTATION),
                        glm::vec3(1.0f,0.0f,1.0f)
                    );

                    glm::mat4 scaleMatrix = glm::scale(
                        glm::mat4(1.0f),
                        glm::vec3(1.0f / (modelObject.bounds[i].dia/2))
                    );

                    GLint matrixPLocation = glGetUniformLocation(ShaderProgram, "P");
                    GLint matrixVLocation = glGetUniformLocation(ShaderProgram, "V");

                    GLint matrixTLocation = glGetUniformLocation(ShaderProgram, "T");
                    GLint matrixRLocation = glGetUniformLocation(ShaderProgram, "R");
                    GLint matrixSLocation = glGetUniformLocation(ShaderProgram, "S");

                    glUniformMatrix4fv(matrixPLocation, 1, GL_FALSE, glm::value_ptr(projMatrix));
                    glUniformMatrix4fv(matrixVLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));

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
