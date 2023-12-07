//#include "line.h"

//#include "program.h"

//Lines::Lines(std::vector<Point> lines_points) : m_points_count(2)//lines_points.size())
//{
//    lines_points = { Point(-10, 0, 0), Point(10, 0, 0) };

//    const char *vertex_shader_code = "#version 330 core\n"
//                "layout (location = 0) in vec3 aPos;\n"
//                "uniform mat4 u_mvp_matrix;\n"
//                "void main()\n"
//                "{\n"
//                "   gl_Position = u_mvp_matrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
//                "}\0";
//            const char *fragment_shader_code = "#version 330 core\n"
//                "out vec4 FragColor;\n"
//                "void main()\n"
//                "{\n"
//                "   FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n"
//                "}\n\0";

////    const char* vertex_shader_code = "#version 430\n"
////                                     "layout (location = 0) in vec3 a_pos;\n"
////                                     "uniform mat4 u_mvp_matrix;\n"
////                                     "void main() { gl_Position = u_mvp_matrix * vec4(a_pos, 1.0f); }";

////    const char* fragment_shader_code = "#version 430\n"
////                                       "out vec4 FragColor;"
////                                       "void main() { FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f); }";

//    int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vertex_shader, 1, &vertex_shader_code, NULL);
//    glCompileShader(vertex_shader);

//    int fragment_shader = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(fragment_shader, 1, &fragment_shader_code, NULL);
//    glCompileShader(fragment_shader);

//    m_shader_program = glCreateProgram();
//    glAttachShader(m_shader_program, vertex_shader);
//    glAttachShader(m_shader_program, fragment_shader);
//    glLinkProgram(m_shader_program);

//    glDeleteShader(vertex_shader);
//    glDeleteShader(fragment_shader);

//    glGenBuffers(1, &m_vbo);
//    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
//    glBufferData(GL_ARRAY_BUFFER, lines_points.size(), lines_points.data(), GL_STATIC_DRAW);

//    glGenVertexArrays(1, &m_vao);
//    glBindVertexArray(m_vao);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, NULL);

//    //Cleanup
//    glBindVertexArray(0);
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
//}

//Lines::~Lines()
//{
//    glDeleteVertexArrays(1, &m_vao);
//    glDeleteBuffers(1, &m_vbo);
//    glDeleteProgram(m_shader_program);
//}

//void Lines::draw(const Transform& mvp_matrix)
//{
//    glUseProgram(m_shader_program);

//    GLuint mvp_matrix_uniform_location = glGetUniformLocation(m_shader_program, "u_mvp_matrix");
//    glUniformMatrix4fv(mvp_matrix_uniform_location, 1, GL_TRUE, mvp_matrix.data());

//    glBindVertexArray(m_vao);
//    glDrawArrays(GL_LINES, 0, m_points_count);
//}
