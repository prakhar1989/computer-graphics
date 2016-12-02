#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "linmath.h"
#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include "utils.h"
#include <iostream>
#include "parser.h"

using namespace std;

// source provided for function to load and compile shaders
GLuint InitShader(const char* vertexShaderFile, const char* fragmentShaderFile);

/** Global location config **/
float theta = 0.0;   // mouse rotation around the Y (up) axis
double posx = 0.0;   // translation along X
double posy = 0.0;   // translation along Y

/** Setting up Light and Material Properties **/
const light_properties light = {
        .position   = {0.0, 0.0, -10.0f, 1.0},
        .ambient    = {0.2, 0.2, 0.2, 1.0},
        .diffuse    = {0.8, 0.8, 0.8, 1.0},
        .specular   = {.5, .5, .5, 1.0}
};

const material_properties material = {
        .ambient    = {1.0, 0.0, 1.0, 1.0},
        .diffuse    = {1.0, 0.8, 0.0, 1.0},
        .specular   = {1.0, 0.8, 0.0, 1.0},
        .shininess  = 100.0
};

/** Set the viewer's location **/
const point4 viewer = {0.0, 0.0, -10.0f, 1.0};

const float deg_to_rad = (3.1415926f / 180.0f);

/** values that are sent to shader repeatedly **/
GLint mvp_location, rotation_mat_location;

void update_normals (
    point4 vertices[], point4 points[], vec4 norms[],
    mat4x4& rotation_mat, unsigned long n_vertices
)
{
    for (unsigned int i = 0; i < n_vertices / 3; i++) {
        mat4x4_mul_vec4(points[3*i], rotation_mat, vertices[3*i]);
        mat4x4_mul_vec4(points[3*i+1], rotation_mat, vertices[3*i+1]);
        mat4x4_mul_vec4(points[3*i+2], rotation_mat, vertices[3*i+2]);

        // compute the triangle norm
        vec4 e1, e2, n;
        vec4_sub(e1, points[3*i+1], points[3*i]);
        vec4_sub(e2, points[3*i+2], points[3*i]);
        vec4_mul_cross(n, e1, e2);
        n[3] = 0.f;
        vec4_norm(n, n);

        vecset(norms[3*i], n);
        vecset(norms[3*i+1], n);
        vecset(norms[3*i+2], n);
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void init (int n_vertices)
{
    GLuint vertex_buffer, program;

    GLint vpos_location, vnorm_location; // attributes

    GLint light_diffuse_location, eye_location, // uniforms
          light_specular_location, light_ambient_location,
          light_position_location, material_diffuse_location, material_specular_location,
          material_ambient_location, material_shininess_location;

    // set up vertex buffer object - this will be memory on the GPU where
    // we are going to store our vertex data (that is currently in the "points"
    // array)
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

    // specify that its part of a VAO, what its size is, and where the
    // data is located, and finally a "hint" about how we are going to use
    // the data (the driver will put it in a good memory location, hopefully)
    glBufferData(GL_ARRAY_BUFFER, 2 * n_vertices * sizeof(vec4), NULL, GL_DYNAMIC_DRAW);

    program = InitShader("vshader_passthrough_lit.glsl", "fshader_passthrough_lit.glsl");
    glUseProgram(program);

    // get access to the various things we will be sending to the shaders:
    mvp_location  = glGetUniformLocation(program, "MVP");
    light_diffuse_location = glGetUniformLocation(program, "light_diffuse");
    light_specular_location = glGetUniformLocation(program, "light_specular");
    light_ambient_location = glGetUniformLocation(program, "light_ambient");
    light_position_location = glGetUniformLocation(program, "light_position");
    material_diffuse_location = glGetUniformLocation(program, "material_diffuse");
    material_specular_location = glGetUniformLocation(program, "material_specular");
    material_ambient_location = glGetUniformLocation(program, "material_ambient");
    material_shininess_location = glGetUniformLocation(program, "material_shininess");
    eye_location = glGetUniformLocation(program, "eye");

    glUniform4fv(light_diffuse_location, 1, (const GLfloat *) light.diffuse);
    glUniform4fv(light_specular_location, 1, (const GLfloat *) light.specular);
    glUniform4fv(light_ambient_location, 1, (const GLfloat *) light.ambient);
    glUniform4fv(light_position_location, 1, (const GLfloat *) light.position);
    glUniform4fv(material_diffuse_location, 1, (const GLfloat *) material.diffuse);
    glUniform4fv(material_specular_location, 1, (const GLfloat *) material.specular);
    glUniform4fv(material_ambient_location, 1, (const GLfloat *) material.ambient);
    glUniform1f(material_shininess_location, material.shininess);
    glUniform4fv(eye_location, 1, (const GLfloat *) viewer);

    vpos_location = glGetAttribLocation(program, "vPos");
    vnorm_location = glGetAttribLocation(program, "vNorm");

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 4, GL_FLOAT, GL_FALSE, 0, (void*) (0));

    glEnableVertexAttribArray(vnorm_location);
    glVertexAttribPointer(vnorm_location, 4, GL_FLOAT, GL_FALSE, 0, (void*) (n_vertices * sizeof(vec4)));
}

// use this motionfunc to demonstrate rotation - it adjusts "theta" based
// on how the mouse has moved.
static void mouse_move_rotate (GLFWwindow* window, double x, double y)
{
    static double last_x = 0;// keep track of where the mouse was last:
    double delta = x - last_x;
    if (delta != 0) {
        theta += delta;
        if (theta > 360.0 ) theta -= 360.0;
        if (theta < 0.0 ) theta += 360.0;
        last_x = x;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "USAGE: glrender <obj_file>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string obj_file {argv[1]};

    if (!does_file_exist(obj_file)) {
        std::cerr << "Obj file not found" << std::endl;
        exit(EXIT_FAILURE);
    }

    // read in the object file
    std::vector<float> verts;
    const auto n_vertices = Parser::parse_obj(obj_file, verts);

    // collection of vertices, points and colors
    // as OpenGL expects them
    point4 vertices[n_vertices];
    point4 points[n_vertices];
    vec4 norms[n_vertices];

    for (auto i = 0; i < n_vertices; i++) {
        vertices[i][0] = verts[3*i];
        vertices[i][1] = verts[3*i+1];
        vertices[i][2] = verts[3*i+2];
        vertices[i][3] = 1;
    }

    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(640, 480, "GLRender v0.1", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "Failed to initialize glfw window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(error_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_move_rotate);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    /** Open GL Init **/
    init(n_vertices);

    /** Enable Z Buffering for depth */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // a transformation matrix, for the rotation,
    // which we will apply to every vertex
    mat4x4 rotation_mat;

    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        mat4x4 projection;
        mat4x4 view;

        // update with window viewport size
        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // make up a transform that rotates around screen "Z" with time:
        mat4x4_identity(rotation_mat);
        mat4x4_rotate_Y(rotation_mat, rotation_mat, theta * deg_to_rad);
        glUniformMatrix4fv(rotation_mat_location, 1, GL_FALSE, (const GLfloat*) rotation_mat);

        update_normals(&vertices[0], &points[0], &norms[0], rotation_mat, n_vertices);

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(norms), norms);

        // perspective projection
        vec3 up = {0, 1.f, 0};
        vec3 center = {0, 0, 0};
        vec3 cameraPos = {viewer[0], viewer[1], viewer[2]};

        mat4x4_look_at(view, cameraPos, center, up);
        mat4x4_perspective(projection, 30 * deg_to_rad, ratio, 0.1f, 100.f);
        mat4x4_mul(projection, projection, view);

        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) projection);
        glDrawArrays(GL_TRIANGLES, 0, n_vertices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
