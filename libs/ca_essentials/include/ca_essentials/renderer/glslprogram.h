/*
* This file is a modification of the glslprogram.h file available in codebase of the
* book "OpenGL 4 Shading Language Cookbook - Third Edition" by David Wolff.
*
* Author (modified version): Chrystiano Araujo (araujoc@.cs.ubc.ca)
*
* MIT License
* 
* Copyright (c) 2017 Packt
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#pragma once

#pragma warning( disable : 4290 )

#include <glad/glad.h>

#include <string>
#include <map>
#include <glm/glm.hpp>
#include <stdexcept>

namespace ca_essentials {
namespace renderer {

class GLSLProgramException : public std::runtime_error {
public:
    GLSLProgramException(const std::string &msg) :
            std::runtime_error(msg) {}
};

namespace GLSLShader {
    enum GLSLShaderType {
        VERTEX = GL_VERTEX_SHADER,
        FRAGMENT = GL_FRAGMENT_SHADER,
        GEOMETRY = GL_GEOMETRY_SHADER,
        TESS_CONTROL = GL_TESS_CONTROL_SHADER,
        TESS_EVALUATION = GL_TESS_EVALUATION_SHADER,
        COMPUTE = GL_COMPUTE_SHADER
    };
};

class GLSLProgram {
private:
    GLuint handle;
    bool linked;
    std::map<std::string, int> uniformLocations;

    inline GLint getUniformLocation(const char *name);
	void detachAndDeleteShaderObjects();
    bool fileExists(const std::string &fileName);
    std::string getExtension(const char *fileName);

public:
    GLSLProgram();
	~GLSLProgram();

	// Make it non-copyable.
	GLSLProgram(const GLSLProgram &) = delete;
	GLSLProgram & operator=(const GLSLProgram &) = delete;

	void compileShader(const char *fileName);
    void compileShader(const char *fileName, GLSLShader::GLSLShaderType type);
    void compileShader(const std::string &source, GLSLShader::GLSLShaderType type,
                       const char *fileName = NULL);

    void link();
    void validate();
    void use() const;

    int getHandle();
    bool isLinked();

    void bindAttribLocation(GLuint location, const char *name);
    void bindFragDataLocation(GLuint location, const char *name);

    void setUniform(const char *name, float x, float y, float z);
    void setUniform(const char *name, const glm::vec2 &v);
    void setUniform(const char *name, const glm::vec3 &v);
    void setUniform(const char *name, const glm::vec4 &v);
    void setUniform(const char *name, const glm::mat4 &m);
    void setUniform(const char *name, const glm::mat3 &m);
    void setUniform(const char *name, float val);
    void setUniform(const char *name, int val);
    void setUniform(const char *name, bool val);
    void setUniform(const char *name, GLuint val);

    void findUniformLocations();
    void printActiveUniforms();
    void printActiveUniformBlocks();
    void printActiveAttribs();

    const char *getTypeString(GLenum type);
};

int GLSLProgram::getUniformLocation(const char *name) {
	auto pos = uniformLocations.find(name);

	if (pos == uniformLocations.end()) {
		GLint loc = glGetUniformLocation(handle, name);
		uniformLocations[name] = loc;
		return loc;
	}

	return pos->second;
}

}
}

