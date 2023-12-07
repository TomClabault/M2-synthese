#ifndef LINE_H
#define LINE_H

#include "GL/glew.h"

#include "mat.h"
#include "program.h"
#include "vec.h"

#include <vector>

//class Lines
//{
//public:
//    Lines() {}
//    Lines(std::vector<Point> lines_points);
//    ~Lines();

//    void draw(const Transform& mvp_matrix);

//private:
//    GLuint m_shader_program;
//    GLuint m_vao, m_vbo;

//    int m_points_count;
//};

class Lines {
    int shaderProgram;
    unsigned int VBO, VAO;
    std::vector<float> vertices;
    vec3 startPoint;
    vec3 endPoint;
    vec3 lineColor;
public:
    Lines() {}
    Lines(std::vector<Point> lines_points) {

        GLuint test_program = read_program("test.glsl");
        program_print_errors(test_program);

        lines_points = {Point(-10, 0, 0), Point(10, 0, 0)};
        vec3 start = lines_points[0];
        vec3 end = lines_points[1];

        startPoint = start;
        endPoint = end;
        lineColor = vec3(1,1,1);

        const char *vertexShaderSource = "#version 430\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform mat4 MVP;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = MVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "}\0";
        const char *fragmentShaderSource = "#version 430\n"
            "out vec4 FragColor;\n"
            "uniform vec3 color;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(color, 1.0f);\n"
            "}\n\0";

        // vertex shader
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // check for shader compile errors

        // fragment shader
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // check for shader compile errors

        // link shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // check for linking errors

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        vertices = {
             start.x, start.y, start.z,
             end.x, end.y, end.z,

        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    int draw(const Transform& mvp_matrix) {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, mvp_matrix.data());
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &lineColor.x);

        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, 2);

        return 1;
    }

    ~Lines() {

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
};

#endif
