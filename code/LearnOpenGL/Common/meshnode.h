#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"
#include "texture2d.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace glm;

// 顶点
struct Vertex {
  vec3 Position;
  vec3 Normal;
  vec2 TexCoords;
  // tangent
  vec3 Tangent;
  // bitangent
  vec3 Bitangent;
};

class Mesh {
    public:
        /*  网格数据  */
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture2D *> textures;
        /*  函数  */
        Mesh(const vector<Vertex> &vertices, const vector<unsigned int> &indices, const vector<Texture2D *> &textures, const vec3 &center);
        void Draw(Shader *shader);
        void DrawAmount(Shader *shader, int amount);
        void Clean();
        /*  渲染数据  */
        unsigned int VAO, VBO, EBO;
    private:
        vec3 centerPos;
        /*  函数  */
        void setupMesh();
};  