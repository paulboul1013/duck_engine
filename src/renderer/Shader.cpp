#include "renderer/Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

namespace duck {

Shader::~Shader() {
    if (m_programID) glDeleteProgram(m_programID);
}

bool Shader::compile(const std::string& vertexSrc, const std::string& fragmentSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (!vs || !fs) return false;

    m_programID = glCreateProgram();
    glAttachShader(m_programID, vs);
    glAttachShader(m_programID, fs);
    glLinkProgram(m_programID);

    GLint success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(m_programID, 512, nullptr, log);
        std::printf("Shader link 失敗: %s\n", log);
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    std::printf("Shader 編譯成功 (ID=%u)\n", m_programID);
    return true;
}

void Shader::use() const {
    glUseProgram(m_programID);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& v) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(v));
}

void Shader::setVec4(const std::string& name, const glm::vec4& v) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(v));
}

void Shader::setMat4(const std::string& name, const glm::mat4& m) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        const char* typeName = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        std::printf("%s shader 編譯失敗: %s\n", typeName, log);
        return 0;
    }
    return shader;
}

GLint Shader::getUniformLocation(const std::string& name) const {
    return glGetUniformLocation(m_programID, name.c_str());
}

} // namespace duck
