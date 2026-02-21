#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace duck {

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool compile(const std::string& vertexSrc, const std::string& fragmentSrc);
    void use() const;

    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& v) const;
    void setVec4(const std::string& name, const glm::vec4& v) const;
    void setMat4(const std::string& name, const glm::mat4& m) const;

    GLuint getID() const { return m_programID; }

private:
    GLuint m_programID = 0;
    GLuint compileShader(GLenum type, const std::string& source);
    GLint getUniformLocation(const std::string& name) const;
};

} // namespace duck
