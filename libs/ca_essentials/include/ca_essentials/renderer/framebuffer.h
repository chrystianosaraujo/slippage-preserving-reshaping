#pragma once

#include <glad/glad.h>

#include <vector>

namespace ca_essentials {
namespace renderer {

class Framebuffer {
public:
    enum class STATUS {
        FRAMEBUFFER_COMPLETE,
        SIMPLE_SAMPLING_FRAMEBUFFER_ERROR,
        MSAA_FRAMEBUFFER_ERROR,
    };

public:
    ~Framebuffer();

    // MSAA is disabled when msaa=0
    STATUS setup(int width, int height, int msaa=0);

    void bind() const;
    void unbind() const;

    GLuint id() const;
    GLuint texture_id() const;

    int width() const;
    int height() const;
    int num_comps() const;
    int msaa() const;
    bool is_msaa_enabled() const;

    void blit_to_single_sampling_fbo() const;

    std::vector<unsigned char> get_color_buffer() const;

private:
    bool is_complete(GLuint fbo) const;

    void setup_single_sampling_fbo();
    void setup_msaa_fbo();

    void delete_buffers();

    // Return a valid MSAA level. 
    //   - must be greater or equal to 0
    //   - must be less than or equal to GL_MAX_SAMPLES
    //   - must be multiple of 2
    GLint get_valid_msaa(GLint val) const;

private:
    int m_width = 0;
    int m_height = 0;
    int m_msaa = 0;

    // Simple sampling buffers info
    GLuint m_fbo = 0;
    GLuint m_tex = 0;
    GLuint m_depth_rbo = 0;

    // MSAA buffers info
    GLuint m_msaa_fbo= 0;
    GLuint m_msaa_color_rbo = 0;
    GLuint m_msaa_depth_rbo = 0;

    STATUS m_status = STATUS::FRAMEBUFFER_COMPLETE;
};

}
}
