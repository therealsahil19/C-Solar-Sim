#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "glad.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace SolarSim {

/**
 * @brief Utility class for loading, compiling, and using GLSL shaders.
 */
class ShaderProgram {
private:
    unsigned int programID = 0;
    bool valid = false;

    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open shader file: " << path << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    unsigned int compileShader(const std::string& source, GLenum type) {
        unsigned int shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed: " << infoLog << std::endl;
            return 0;
        }
        return shader;
    }

public:
    ShaderProgram() = default;

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexSource = readFile(vertexPath);
        std::string fragmentSource = readFile(fragmentPath);

        if (vertexSource.empty() || fragmentSource.empty()) {
            return false;
        }

        unsigned int vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
        unsigned int fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

        if (!vertexShader || !fragmentShader) {
            return false;
        }

        programID = glCreateProgram();
        glAttachShader(programID, vertexShader);
        glAttachShader(programID, fragmentShader);
        glLinkProgram(programID);

        int success;
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(programID, 512, nullptr, infoLog);
            std::cerr << "Shader linking failed: " << infoLog << std::endl;
            return false;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        valid = true;
        return true;
    }

    void use() const {
        if (valid) glUseProgram(programID);
    }

    unsigned int getID() const { return programID; }
    bool isValid() const { return valid; }

    // Uniform setters
    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, glm::value_ptr(value));
    }

    void setMat3(const std::string& name, const glm::mat3& mat) const {
        glUniformMatrix3fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    ~ShaderProgram() {
        if (programID) glDeleteProgram(programID);
    }
};

} // namespace SolarSim
