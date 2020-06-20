#include "global.h"
#include "model.h"
#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

extern Shader *shader[SHADER_NUM] = {nullptr};
int OBJ_IDXS[] = {IDX_CUBE};

// point light
glm::vec3 gPointLightPositions[] = {
  glm::vec3( 0.7f,  0.2f,  2.0f),
  glm::vec3( 2.3f, -3.3f, -4.0f),
  glm::vec3(-4.0f,  2.0f, -12.0f),
  glm::vec3( 0.0f,  0.0f, -3.0f),
  glm::vec3( 1.2f,  1.0f,  2.0f),
};

void cleanShader() {
  for (int i=0; i<SHADER_NUM; i++) {
    if (shader[i]) delete shader[i];
    shader[i] = nullptr;
  }
}

void initShaders() {
  cleanShader();
  // shader source -> shader object -> shader program
  shader[IDX_CUBE] = new Shader("vertex_specular.shader", "fragment_multiple_lights.shader");
  shader[IDX_PLANE] = new Shader("vertex_specular.shader", "fragment_blinn_phong.shader");
  shader[IDX_LAMP] = new Shader("vertex_lighted.shader", "fragment_lamp.shader");


  // direct light
  vec3 ambientLight(0.f, 0.f, 0.f);
  vec3 diffuseLight(0.1f, 0.1f, 0.1f);
  vec3 specularLight(1.0f, 1.0f, 1.0f);
  vec3 directLight(0.0f, -1.0f, 0.0f);

  for (int i : OBJ_IDXS) {
    shader[i]->setVec3("directLight.ambient", glm::value_ptr(ambientLight));
    shader[i]->setVec3("directLight.diffuse", glm::value_ptr(diffuseLight));
    shader[i]->setVec3("directLight.specular", glm::value_ptr(specularLight));

    shader[i]->setVec3("directLight.direction", glm::value_ptr(directLight));
  }

  // spot light
  ambientLight = vec3(0.0f, 0.0f, 0.0f);
  diffuseLight = vec3(0.8f, 0.8f, 0.8f);
  specularLight = vec3(1.0f, 1.0f, 1.0f);

  for (int i: OBJ_IDXS) {
    shader[i]->setVec3("spotLight.ambient", glm::value_ptr(ambientLight));
    shader[i]->setVec3("spotLight.diffuse", glm::value_ptr(diffuseLight));
    shader[i]->setVec3("spotLight.specular", glm::value_ptr(specularLight));

    shader[i]->setFloat("spotLight.constant", 1.0f);
    shader[i]->setFloat("spotLight.linear", 0.022f);
    shader[i]->setFloat("spotLight.quadratic", 0.0019);

    shader[i]->setFloat("spotLight.cutoff", glm::cos(glm::radians(15.0f)));
    shader[i]->setFloat("spotLight.cutoff_outter", glm::cos(glm::radians(20.0f)));
  }
  
  for (int i=0; i<(sizeof(gPointLightPositions)/sizeof(glm::vec3)); i++) {
    for (int j : OBJ_IDXS) {
      shader[j]->setVec3("pointLights.ambient", glm::value_ptr(ambientLight), i);
      shader[j]->setVec3("pointLights.diffuse", glm::value_ptr(diffuseLight), i);
      shader[j]->setVec3("pointLights.specular", glm::value_ptr(specularLight), i);
      
      shader[j]->setFloat("pointLights.constant", 1.0f, i);
      shader[j]->setFloat("pointLights.linear", 0.14, i);
      shader[j]->setFloat("pointLights.quadratic", 0.07f, i);

      shader[j]->setVec3("pointLights.position", glm::value_ptr(gPointLightPositions[i]), i);
    }
  }

  for (int i: OBJ_IDXS)
    shader[i]->setFloat("material.shininess", 32.0f);

  vec3 lightColor(1.0f, 1.0f, 1.0f);
  vec3 objColor(1.0f, 1.0f, 1.0f);
  glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
  shader[IDX_PLANE]->setVec3("lightColor", glm::value_ptr(lightColor));
  shader[IDX_PLANE]->setVec3("objColor", glm::value_ptr(objColor));
  shader[IDX_PLANE]->setVec3("lightPos", glm::value_ptr(lightPos));
  shader[IDX_PLANE]->setInt("blinn", 0);  
}

void renderPointLights() {
  shader[IDX_LAMP]->use();
  glBindVertexArray(VAO[IDX_LAMP]);

  for (int i=0; i<(sizeof(gPointLightPositions)/sizeof(glm::vec3)); i++) {
    glm::vec3 pos = gPointLightPositions[i];
    glm::mat4 model(1.0f);
    model = glm::translate(model, pos);
    // model = glm::rotate(model, (float)glfwGetTime()+20.0f*(i+1), glm::vec3(0.5f, 0.5f, 1.0f));
    model = glm::scale(model, glm::vec3(0.2f));
    shader[IDX_LAMP]->setMatrix4("model", glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
}

void renderCamera(Camera::Camera *camera) {
    glm::mat4 projection(1.0f);
    projection = glm::perspective(glm::radians(camera->FieldOfView), float(WIN_WIDTH)/glm::max((float)WIN_HEIGHT,0.01f), NEAR_Z, FAR_Z);
    glm::mat4 view = camera->GetViewMatrix();
    for (int i=0; i<SHADER_NUM; i++) {
      shader[i]->setMatrix4("projection", glm::value_ptr(projection));
      shader[i]->setMatrix4("view", glm::value_ptr(view));
    }

    glm::vec3 lightPos = camera->Position;  // shader的位置
    glm::vec3 lightDir = camera->Front; //glm::normalize(camera->Position + camera->Front * 200.0f - lightPos);
    for (int i : OBJ_IDXS) {
      shader[i]->setVec3("spotLight.position", glm::value_ptr(lightPos));
      shader[i]->setVec3("spotLight.direction", glm::value_ptr(lightDir));
      shader[i]->setVec3("viewPos", glm::value_ptr(camera->Position));
    }
    
    // glm::mat4 model;
    
    // // 手电筒
    // glBindVertexArray(VAO[IDX_CUBE]);
    // shader[IDX_LAMP]->use();

    // // 右手持
    // lightPos += camera->Right + camera->Front * 3.0f; // 绘制光源的位置, 会有一点出入. 目的是保证手电筒的光锥是直的.
    // model = glm::mat4(1.0f);
    // model = glm::translate(model, lightPos);
    // model = glm::scale(model, glm::vec3(0.2f));
    // shader[IDX_LAMP]->setMatrix4("model", model);
    // glDrawArrays(GL_TRIANGLES, 0, 36);
}