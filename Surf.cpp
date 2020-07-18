#include <vector>
#include <glm/glm.hpp>
#include "Surf.h"
#include <glm/gtc/matrix_transform.hpp>

static const int N = 50;
static const int NUM_VERTICES = N * N;
static const int PRIMI_RESTART = 3000;
static const int NUM_INSTANCED = 9;
static const int N_Strips = N - 1;
static const int NumIndicesPerStrip = 2*N;

//The sinc surface
static glm::vec3 surf(int i, int j)
{
    const float center = 0.5f*N;
    const float xy_scale = 20.0f / N;
    const float z_scale = 10.0f;

    float x = xy_scale * (i - center);
    float y = xy_scale * (j - center);

    float r = sqrt(x*x + y * y);
    float z = 1.0f;

    if (r != 0.0f)
    {
        z = sin(r) / r;
    }
    z = z * z_scale;

    return 0.05f*glm::vec3(x, y, z);
}

//Sinc surface normals
static glm::vec3 normal(int i, int j)
{
    glm::vec3 du = surf(i + 1, j) - surf(i - 1, j);
    glm::vec3 dv = surf(i, j + 1) - surf(i, j - 1);
    return glm::normalize(glm::cross(du, dv));
}

inline glm::vec2 tex_coord(int i, int j) {
    return glm::vec2(float(i) / N, float(j) / N);  //u, v
}
GLuint create_transM() {
    //function to set the position (transform Matrix) of each 9 surf
    std::vector<glm::mat4> matrices;
    float a = 1.2f;
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(-a, -a, 0)));
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(-a, 0, 0)));
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(-a, a, 0)));
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(0, -a, 0)));
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0)));
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(0, a, 0)));
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(a, -a, 0)));
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(a, 0, 0)));
    matrices.push_back(glm::translate(glm::mat4(1.0), glm::vec3(a, a, 0)));

    GLuint trans_vbo;
    glGenBuffers(1, &trans_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, trans_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4)*matrices.size(), matrices.data(), GL_STATIC_DRAW);
    return trans_vbo;
}
GLuint create_color_vbo() {

    std::vector<glm::vec3> colors; //create 9 colors
    colors.push_back(glm::vec3(1.0f, 0.5f, 1.0f));
    colors.push_back(glm::vec3(1.0f, 0.5f, 0.6f));
    colors.push_back(glm::vec3(1.0f, 0.5f, 0.2f));
    colors.push_back(glm::vec3(1.0f, 1.0f, 0.5f));
    colors.push_back(glm::vec3(0.6f, 1.0f, 0.5f));
    colors.push_back(glm::vec3(0.2f, 1.0f, 0.5f));
    colors.push_back(glm::vec3(0.5f, 1.0f, 1.0f));
    colors.push_back(glm::vec3(0.5f, 0.6f, 1.0f));
    colors.push_back(glm::vec3(0.5f, 0.2f, 1.0f));

    GLuint color_vbo;
    glGenBuffers(1, &color_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*colors.size(), colors.data(), GL_STATIC_DRAW);
    return color_vbo;
}
GLuint create_surf_ebo() {
    GLuint ebo;
    //Set up element array buffer (index buffer)
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    std::vector<unsigned int> indices;

    const int index = 0;
    for (int i = 0; i < N - 1; i++) { //strip
        for (int j = 0; j < N; j++) { //data of each strip
            indices.push_back(j+i*N);
            indices.push_back(j + N + i*N);
        }
        indices.push_back(PRIMI_RESTART);
    }

    glPrimitiveRestartIndex(PRIMI_RESTART);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*indices.size(), indices.data(), GL_STATIC_DRAW);
    return ebo;
}
GLuint create_surf_vbo()
{
    //Declare a vector to hold N*N vertices, each with 8 float (3 for position, 3 for norm, 2 for uv texture)
    std::vector<float> surf_verts;
    glm::vec3 pos = surf(0, 0);
    for (int i = 0 ; i < N ; i++) {
        for (int j = 0 ; j < N ; j++) {
            glm::vec3 pos = surf(i,j);
            glm::vec3 norm = normal(i, j);
            glm::vec2 tex = tex_coord(i,j);
            //define interleaved vbo
            //position
            surf_verts.push_back(pos.x);
            surf_verts.push_back(pos.y);
            surf_verts.push_back(pos.z);
            //normal
            surf_verts.push_back(norm.x);
            surf_verts.push_back(norm.y);
            surf_verts.push_back(norm.z);
            //uv texture
            surf_verts.push_back(tex.x);
            surf_verts.push_back(tex.y);
        }
    }

    GLuint vbo;
    glGenBuffers(1, &vbo); //Generate vbo to hold vertex attributes for triangle.

    glBindBuffer(GL_ARRAY_BUFFER, vbo); //Specify the buffer where vertex attribute data is stored.
                                        //Upload from main memory to gpu memory.
    glBufferData(GL_ARRAY_BUFFER, surf_verts.size()*sizeof(float), surf_verts.data(), GL_STATIC_DRAW);


    return vbo;
}
GLuint create_surf_vao()
{
    GLuint vao;

    //Generate vao id to hold the mapping from attrib variables in shader to memory locations in vbo
    glGenVertexArrays(1, &vao);

    //Binding vao means that bindbuffer, enablevertexattribarray and vertexattribpointer state will be remembered by vao
    glBindVertexArray(vao);

    GLuint ebo = create_surf_ebo();
    GLuint vbo = create_surf_vbo();

    const GLuint pos_loc = 0;
    const GLuint normal_loc = 1;
    const GLuint tex_loc = 2;
    glEnableVertexAttribArray(pos_loc); //Enable the position attribute.
    glEnableVertexAttribArray(normal_loc);
    glEnableVertexAttribArray(tex_loc);
    //Tell opengl how to get the attribute values out of the vbo (stride and offset).
    glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, sizeof(float) * 8, 0);
    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, false, sizeof(float) * 8, (void*)(sizeof(float) * 3));
    glVertexAttribPointer(tex_loc, 2, GL_FLOAT, false, sizeof(float) * 8, (void*)(sizeof(float) * 6));

    //bind the transform matrix vbo
    GLuint Mvbo = create_transM();
    const GLuint transM_loc = 3;
    //define the location of 4 vec3 of the matrix
    int m1 = 3;
    int m2 = 4;
    int m3 = 5;
    int m4 = 6;
    glEnableVertexAttribArray(m1);
    glEnableVertexAttribArray(m2);
    glEnableVertexAttribArray(m3);
    glEnableVertexAttribArray(m4);
    glVertexAttribPointer(m1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * 4, 0);
    glVertexAttribPointer(m2, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * 4, (void*)(sizeof(float) * 4));
    glVertexAttribPointer(m3, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * 4, (void*)(sizeof(float) * 8));
    glVertexAttribPointer(m4, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4 * 4, (void*)(sizeof(float) * 12));
    glVertexAttribDivisor(m1, 1);
    glVertexAttribDivisor(m2, 1);
    glVertexAttribDivisor(m3, 1);
    glVertexAttribDivisor(m4, 1);

    //bind the color matrix vbo
    GLuint Cvbo = create_color_vbo();
    const int color_loc = 7;
    glEnableVertexAttribArray(color_loc);
    glVertexAttribPointer(color_loc, 3, GL_FLOAT, false, 0, 0);
    glVertexAttribDivisor(color_loc, 1);


    glBindVertexArray(0); //unbind the vao

    return vao;
}

//Draw the set of points on the surface
void draw_surf(GLuint vao)
{
    glBindVertexArray(vao);
    glDrawElementsInstanced(GL_TRIANGLE_STRIP, N_Strips * NumIndicesPerStrip + N_Strips, GL_UNSIGNED_INT, 0, 9);
}
