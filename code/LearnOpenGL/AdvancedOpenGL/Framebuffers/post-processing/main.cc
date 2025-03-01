#include <iostream>
using namespace std;

#include "global.h"
#include "camera.h"

#include <vector>
#include <map>
#include <GLFW/glfw3.h>

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

void cleanCubeData();
void initCubeData();
void renderCubes();
void renderPlane();
void renderMonitor();

void cleanShader();
void initShaders();
void renderCamera();
void renderPointLights();

void cleanModels();
void initModels(int n, char *path[]);
void renderModels();
void switchModels();

void fillFrameBuffer();
void drawScreen();

extern unsigned int WIN_WIDTH = 800;
extern unsigned int WIN_HEIGHT = 800;
extern float NEAR_Z = 0.1f;
extern float FAR_Z = 1000.0f;
extern Camera::Camera *camera = nullptr;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX=WIN_WIDTH*0.5f, lastY=WIN_HEIGHT*0.5f;
float sensitive = 0.2f;
bool firstMouse = true;
bool fillType = true;
map<unsigned int, float> key_lastFrame;

int main(int argc, char *argv[]) {
  glfwInit();
  
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // two lines tell the version of OpenGL is 3.3
  // 用 Lab/Test/version.cc 工具查看OpenGL版本

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // will use core-profile
  // 我们只会用到OpenGL的子集，无需向后兼容的特性

#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);   // open in Mac OS X
#endif
#ifdef FULL_SCREEN
  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);

  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  WIN_WIDTH = mode->width;
  WIN_HEIGHT = mode->height;

  GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, __FILE__, monitor, NULL);
#else
  GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, __FILE__, NULL, NULL);
#endif
  if (window == NULL)
  {
      cout << "Failed to create GLFW window" << endl;
      glfwTerminate();
      return -1;
  }
  glfwMakeContextCurrent(window);         // 当前线程与window绑定

  // walk around
  glfwSetWindowSizeCallback(window, framebuffer_size_callback);
  // look around
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPosCallback(window, mouse_callback);

  // zoom in/out
  glfwSetScrollCallback(window, scroll_callback);

  // glad 必须在 windows 创建成功后初始化
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    cout << "Failed to initialize GLAD" << endl;
    return -1;
  }

  glEnable(GL_DEPTH_TEST);

  camera = new Camera::Camera(0.0f);
  
  initShaders();
  initModels(argc-1, argv+1);
  initCubeData();

  // render loop:
  while(!glfwWindowShouldClose(window))
  {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // input
    processInput(window);
    
    // 第一处理阶段(Pass)
    // 绘制自定义帧缓冲
    fillFrameBuffer();

    // 第二处理阶段
    // 默认帧缓冲绘制贴图
    drawScreen();

    // 绘制监视器 unavaliable
    renderMonitor();

    glfwSwapBuffers(window);              // double buffer switch
    glfwPollEvents();                     // keyboard/mouse event
  }

  delete camera;

  // optional: de-allocate all resources once they've outlived their purpose:
  // ------------------------------------------------------------------------
  cleanShader();
  cleanModels();
  cleanCubeData();

  glfwTerminate();
  return 0;
}

void fillFrameBuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, FRAME_BUFF_ID);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 我们现在不使用模板缓冲

    //////////////////////////////// render camera & spotlight 手电筒
    renderCamera(); // 相机的信息, 作用于所有shader
    
    //////////////////////////////// render loading_model
    renderModels();
    
    ////////////////////////////////
    renderPlane();
    renderCubes();
        
    //////////////////////////////// render point light
    renderPointLights();
}

void drawScreen() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 返回默认
    glDisable(GL_DEPTH_TEST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(VAO[IDX_DBUG]);

    shader[IDX_DBUG]->use();  
    cube_texture[TEX_QUAD]->use();
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, cube_texture[TEX_QUAD]->ID);
    shader[IDX_DBUG]->setInt(cube_texture[TEX_QUAD]->uniform_name, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    Texture2D::reset();
}

void renderMonitor() {
  // glEnable(GL_DEPTH_TEST);
  
  std::map<float, glm::vec3> sorted;
  for (glm::vec3 pos : monitor_positions) {
      float distance = glm::length(camera->Position - pos);
      sorted[distance] = pos;
  }

  Shader *s = shader[IDX_MONITOR];
  unsigned int v = VAO[IDX_MONITOR];
  Texture2D *tex = cube_texture[TEX_QUAD];

  glBindVertexArray(v);
  
  for (map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); it++) {
    s->use();  
    tex->use();
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, cube_texture[TEX_QUAD]->ID);
    s->setInt(tex->uniform_name, 0);

    glm::mat4 model(1.0f);
    model = glm::translate(model, it->second);
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    s->setMatrix4("model", glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);
   
    Texture2D::reset();
  }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  camera->ProcessMouseScroll(yoffset);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  if(firstMouse) {
      lastX = xpos;
      lastY = ypos;
      firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos; // 注意这里是相反的，因为y坐标是从底部往顶部依次增大的
  lastX = xpos;
  lastY = ypos;
  
  // cout << xoffset << " " << yoffset << endl;
  camera->ProcessMouseMovement(xoffset, yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  cout << "view change " << width << " " << height << endl;
  glViewport(0, 0, width, height);

  WIN_WIDTH = width;
  WIN_HEIGHT = height;
}  

bool checkKeySensitive(unsigned int key) {
  float currentFrame = deltaTime + lastFrame;
  if (currentFrame - key_lastFrame[key] > sensitive) {
    key_lastFrame[key] = currentFrame;
    return true;
  }
  return false;
}

void processInput(GLFWwindow *window)
{
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);           // response to ESC

  if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera->ProcessKeyboard(Camera::FORWARD, deltaTime);
  if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera->ProcessKeyboard(Camera::BACKWARD, deltaTime);
  if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera->ProcessKeyboard(Camera::LEFT, deltaTime);
  if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera->ProcessKeyboard(Camera::RIGHT, deltaTime);
  if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    camera->ProcessKeyboard(Camera::RISE, deltaTime);
  if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    camera->ProcessKeyboard(Camera::FALL, deltaTime);

  
  if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && checkKeySensitive(GLFW_KEY_SPACE)) {
    fillType = !fillType;
    if (fillType) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

  if(glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && checkKeySensitive(GLFW_KEY_TAB)) {
    switchModels();
  }

  if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && checkKeySensitive(GLFW_KEY_TAB)) {
    // <Enter>: reload shader
    initShaders();
  }
}