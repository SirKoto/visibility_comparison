#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <filesystem>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Mesh.hpp"
#include "Args.hpp"
#include "testAABBoxInFrustum.h"


constexpr const char* MESH_TO_LOAD = "./models/Armadillo.ply";

constexpr const char* SHADER_FRAGMENT = "./shaders/norm.frag";
constexpr const char* SHADER_VERTEX =   "./shaders/norm.vert";


GLFWwindow* g_window;
uint32_t g_normProgram;

std::vector<glm::vec2> g_gridPositions;
std::vector<uint32_t> g_frustumCullingPos;
std::vector<uint32_t> g_occlusionCullingPos;

uint32_t g_gridResoulution;

std::vector<std::pair<double, double>> g_FramerateBuffer;
std::string g_outFileName;

Mesh* g_mesh;

double g_startTime = 0.0;
double g_endTime = 0.0;
double g_actualTime = 0.0;


glm::mat4 g_currentViewMatrix(1);
glm::mat4 g_currentProjMatrix(1);
glm::mat4 g_currentViewProjMatrix(1);

std::array<std::vector<uint32_t>, 2> g_queryObjects;
uint32_t g_currentBuffer = 0;

enum Mode {
    eUnoptimized = 0,
    eFrustumCulling = 1,
    eOcclusionCulling = 2
};

Mode g_mode = Mode::eUnoptimized;


const std::vector<glm::vec3> dirPoints = {
    {0,1,4},
    {0,1,2},
    {2/std::sqrt(2),1,2/std::sqrt(2)},
    {2,1,0},
    {2/std::sqrt(2),1,-2/std::sqrt(2)},
    {0,1,-2},
    {-2/std::sqrt(2),1,-2/std::sqrt(2)},
    {-2,1,0},
    {-2/std::sqrt(2),1,2/std::sqrt(2)},
    {0,1,2},
    {2/std::sqrt(2),1,2/std::sqrt(2)},
    {2,1,0},
    {2/std::sqrt(2),1,-2/std::sqrt(2)},
    {0,1,-2},
    {-2/std::sqrt(2),1,-2/std::sqrt(2)},
    {-2,1,0},

    {0.5,1,1},
    {0.5,0.5,0.5},
    {1,0.5,0.7},
    {0.85,0.5,0.76},
    {0.65,0.5,0.9},
    {0.5,0.5,1},
    {0.4,0.5,1},
    {0.3,0.5,1},
    {0.2,0.5,0.9},
    {0.1,0.5,0.8},
    {-0.1,0.5,0.7},
    {-0.1,0.5,0.6},
    {-0.1,0.5,0.5},
    {-0.1,0.5,0.4},
    {-0.1,0.5,0.3},
    {-0.1,0.5,0.2},
    {0.1,0.5,0.1},
    {0.2,0.5,0.1},
    {0.3,0.5,0.1},
    {0.4,0.5,0.1},
    {0.5,0.5,0.1},
    {0.5,0.5,-0.2},
    {0.5,0.5,-0.5},
    {0.5,0.6,-0.3},
    {0.5,0.7,-0.1},
    {0.5,0.8, 0.1},
    {0.5,0.9, 0.2},
    {0.5,1.2, 0.3},
    {0.5,1.5, 0.4},

    {0.5,2.0, 0.4},
    {0.5,3.0, 0.4},
    {0.5,4.0, 0.4},

    {0.5,3.0, 0.35},
    {0.45,3.0, 0.3},
    {0.4,3.0, 0.3},
    {0.4,3.0, 0.25},
    {0.6,3.0, 0.25},
    {0.7,3.0, 0.2},
    {0.6,3.0, 0.2},

    {0.5,1.5, 0.4},
    {0.5,1.2 , 0.5},
    {0.5,0.9 , 0.6},
    {0.5,0.7 , 0.7},
    {0.5,0.6 , 0.8},
    {0.5,0.5 , 0.9},

    {0.4,0.5, 0.6},
    {0.4,0.5, 0.5},
    {0.4,0.5, 0.4},
    {0.5,0.5, 0.4},
    {0.6,0.5, 0.4},
    {0.6,0.5, 0.5},
    {0.6,0.5, 0.6},
    {0.5,0.5,0.6},
    {0.4,0.5, 0.6},
    {0.4,0.5, 0.5},
    {0.4,0.5, 0.4},
    {0.5,0.5, 0.4},
    {0.6,0.5, 0.4},
    {0.6,0.5, 0.5},
    {0.6,0.5, 0.6},
    {0.5,0.5,0.6}

};

// quadratic b-spline
glm::vec3 evalBSpline(const std::vector<glm::vec3>& points, double u) {
    double t = u * double(points.size() - 2);
    uint32_t idx = std::min(size_t(std::floor(t)), points.size() - 2);
    t = t - std::floor(t);
    glm::vec3 O = 0.5f * glm::vec3(t*t - 2*t + 1, -2*t*t + 2*t +1, t*t);
    return points[idx + 0] * O.x + points[idx + 1] * O.y + points[idx + 2] * O.z;
}

int startupGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
    g_window = glfwCreateWindow(640, 512, "Visibility", NULL, NULL);
    if (g_window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(g_window);

    // Disable v sync
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return 1;
    }

    return 0;
}

bool readFile(const std::string& filename, std::string *shader_source) {

  std::ifstream infile(filename.c_str());

  if (!infile.is_open() || !infile.good()) {
    std::cerr << "Error " + filename + " not found." << std::endl;
    return false;
  }

  std::stringstream stream;
  stream << infile.rdbuf();
  infile.close();

  *shader_source = stream.str();
  return true;
}

uint32_t loadShader(const std::string& filename, uint32_t shaderType) {
    if(!std::filesystem::exists(filename)){
        std::cerr << "Shader " << filename << " not exists" << std::endl;
        return 0;
    }

    std::string src;
    if(!readFile(filename, &src)){
        std::cerr << "Shader " << filename << " can't be read" << std::endl;
        return 0;
    }

    uint32_t shader = glCreateShader(shaderType);
    const char* c_str = src.c_str();
    glShaderSource(shader, 1, &c_str, nullptr);
    glCompileShader(shader);
    // check for errors
    int32_t  success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        int32_t len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> errMessage(len, 0);

        glGetShaderInfoLog(shader, len, NULL, errMessage.data());
        std::cout << "Compilation error:\n" << errMessage.data() << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

uint32_t loadProgram(const std::string& vertexShaderPath,
                     const std::string& fragmentShaderPath){


    uint32_t vert = loadShader(vertexShaderPath, GL_VERTEX_SHADER);
    if(vert == 0) {
        return 0;
    }
    uint32_t frag = loadShader(fragmentShaderPath, GL_FRAGMENT_SHADER);
    if(frag == 0) {
        return 0;
    }

    uint32_t prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);

    glLinkProgram(prog);
    //there should not be linking errors....
    int32_t success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    assert(success);

    glDeleteShader(vert);
    glDeleteShader(frag);

    return prog;
}

void genGrid(uint32_t gridRes) {
    g_gridResoulution = gridRes;

    g_gridPositions.clear();
    g_gridPositions.reserve(g_gridResoulution * g_gridResoulution);
    for(uint32_t i = 0; i < g_gridResoulution; ++i) {
        for(uint32_t j = 0; j < g_gridResoulution; ++j) {
            g_gridPositions.push_back(glm::vec2(i, j));
        }
    }
}

void updateCamera(){

    double u = (g_actualTime - g_startTime) / (g_endTime - g_startTime);


    glm::vec3 center = glm::vec3(g_gridResoulution / 2.0,
                                 g_mesh->getSize().y / 2.f,
                                 g_gridResoulution / 2.0);

    glm::vec3 eye = evalBSpline(dirPoints, u) * glm::vec3(g_gridResoulution, g_mesh->getSize().y , g_gridResoulution);

    g_currentViewMatrix = glm::lookAt(glm::vec3(eye),
                                 center,
                                 glm::vec3(0,1,0));

    int32_t width, height;
    glfwGetWindowSize(g_window, &width, &height);

    g_currentProjMatrix = glm::perspective(glm::radians(45.f), float(width)/float(height) , 0.01f, 100.0f);

    g_currentViewProjMatrix = g_currentProjMatrix * g_currentViewMatrix;

    glUniformMatrix4fv(1, 1, GL_FALSE, &g_currentViewMatrix[0][0]);
    glUniformMatrix4fv(2, 1, GL_FALSE, &g_currentProjMatrix[0][0]);
}



void updateFrustumCulling() {
    g_frustumCullingPos.clear();
    g_frustumCullingPos.reserve(g_gridPositions.size());

    glm::vec3 size = g_mesh->getSize();
    uint32_t i = 0;
    for(const glm::vec2& p : g_gridPositions) {

        glm::vec3 p3(p.x, 0, p.y);

        glm::mat4 MVP = glm::translate(g_currentViewProjMatrix, p3);

        if(testAABBoxInFrustum(glm::vec3(0), size, MVP)) {
            g_frustumCullingPos.push_back(i);
        }
        ++i;
    }
}

void launchOcclusionQueries(uint32_t toBuffer) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    for (uint32_t i = 0; i < g_gridPositions.size(); ++i) {
        glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, g_queryObjects[toBuffer][i]);
        g_mesh->drawBBoxOnlyInstance(i);
        glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
    }
    glFlush();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
}

int mainLoop() {
    g_mesh = new Mesh();
    try {
        g_mesh->loadMesh(MESH_TO_LOAD);
    } catch(const std::exception& e){
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "Loaded mesh with:\n\t" << g_mesh->numVertices() <<
                 " vertices\n\t" << g_mesh->numFaces() << " faces" << std::endl;

    g_normProgram = loadProgram(SHADER_VERTEX, SHADER_FRAGMENT);
    if(g_normProgram == 0){
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

    glUseProgram(g_normProgram);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(g_mesh->getModelMatrix()));


    g_mesh->setInstances(g_gridPositions);
    if(g_mode == Mode::eOcclusionCulling) {
        g_mesh->setBBInstances(g_gridPositions);
        g_queryObjects[0].resize(g_gridPositions.size());
        glGenQueries(g_gridPositions.size(), g_queryObjects[0].data());
        g_queryObjects[1].resize(g_gridPositions.size());
        glGenQueries(g_gridPositions.size(), g_queryObjects[1].data());
        g_occlusionCullingPos.reserve(g_gridPositions.size());
    }

    g_startTime = glfwGetTime();
    g_actualTime = g_startTime;
    g_endTime += g_startTime;
    while (!glfwWindowShouldClose(g_window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        updateCamera();

        switch (g_mode) {
        case Mode::eUnoptimized:
            g_mesh->draw();
            break;
        case Mode::eFrustumCulling:
            updateFrustumCulling();
            for(uint32_t i = 0; i < g_frustumCullingPos.size(); ++i){
                g_mesh->drawOnlyInstance(g_frustumCullingPos[i]);
            }
            break;
        case Mode::eOcclusionCulling:
            launchOcclusionQueries(g_currentBuffer);
            g_occlusionCullingPos.clear();
            g_currentBuffer = (g_currentBuffer + 1) % 2;
            uint32_t samplePassed;
            for (uint32_t i = 0; i < g_gridPositions.size(); ++i) {

                glGetQueryObjectuiv(g_queryObjects[g_currentBuffer][i], GL_QUERY_RESULT, &samplePassed);
                if(samplePassed) {
                    g_occlusionCullingPos.push_back(i);
                }
            }

            for(uint32_t i = 0; i < g_occlusionCullingPos.size(); ++i){
                g_mesh->drawOnlyInstance(g_occlusionCullingPos[i]);
            }

            break;
        default:
            assert(false);
            break;
        }
        glfwSwapBuffers(g_window);

        double newTime = glfwGetTime();
        g_FramerateBuffer.push_back({newTime - g_startTime, newTime - g_actualTime});
        g_actualTime = newTime;
        if(g_endTime < g_actualTime){
            glfwSetWindowShouldClose(g_window, GLFW_TRUE);
        }

        glfwPollEvents();
    }

    glDeleteProgram(g_normProgram);
    delete g_mesh;
    return 0;
}

void printUsage(){
    std::cout <<
        "./visibility resolution [-time=time] [-mode=mode] [-out=outfile]\n"
        "\tresolution = int\n"
        "\ttime = double (default 30)\n"
        "\tmode = int (default 0)\n"
        "\t\t 0 is without optimizations\n"
        "\t\t 1 is frustum culling\n"
        "\t\t 2 is occlusion culling\n"

        "\toutfile = output file for framerate (optional)\n"
          <<       std::endl;
}

bool getArgs(const Args& args) {

    if(args.has("h") || args.has("help") || args.numArgs() < 2){
            printUsage();
            return false;
    }

    if(args.has("time")){
        g_endTime = std::stod( args.get("time") );
        assert(g_endTime > 0.0);
    }else{
        g_endTime = 30.0;
    }
    if(args.has("mode")) {
        g_mode = static_cast<Mode>(std::stoi(args.get("mode")));
        assert(g_mode < 4);
    }else{
        g_mode = Mode::eUnoptimized;
    }
    if(args.has("out")) {
        g_outFileName = args.get("out");
    }

    uint32_t resoulution = std::stoi( args.get(1) );
    assert(resoulution != 0);
    genGrid(resoulution);

    return true;
}

void writeFramerate() {
    std::ofstream stream(g_outFileName, std::ofstream::out | std::ofstream::trunc);

    assert(stream);

    for(const auto& t : g_FramerateBuffer) {
        stream << t.first << "\t" << t.second << "\n";
    }

    stream.close();
}

int main(int argc, char** argv) {
    
    int ret = 1;
    if(getArgs(Args(argc, argv)))
    {
        startupGLFW();

        g_FramerateBuffer.reserve(60 * g_endTime);

        ret = mainLoop();

        if(!g_outFileName.empty()) {
            writeFramerate();
        }

        glfwTerminate();
    }

    
    return ret;
}
