#include <ca_essentials/renderer/framebuffer.h>

#include <algorithm>

namespace ca_essentials {
namespace renderer {

Framebuffer::~Framebuffer() {

}

Framebuffer::STATUS Framebuffer::setup(int width, int height, int msaa) {
    m_width = width;
    m_height = height;
    m_msaa = get_valid_msaa(msaa);

    // It starts with a complete status and is changed
    // in case an error occurs
    m_status = STATUS::FRAMEBUFFER_COMPLETE;

    // Delete existing buffers
    delete_buffers();

    setup_single_sampling_fbo();
    if(m_status != STATUS::FRAMEBUFFER_COMPLETE) {
        m_status = STATUS::SIMPLE_SAMPLING_FRAMEBUFFER_ERROR;
        delete_buffers();
        return m_status;
    }

    if(is_msaa_enabled()) {
        setup_msaa_fbo();

        if(m_status != STATUS::FRAMEBUFFER_COMPLETE) {
            m_status = STATUS::MSAA_FRAMEBUFFER_ERROR;
            delete_buffers();
            return m_status;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return m_status;
}

void Framebuffer::bind() const {
    if(is_msaa_enabled())
        glBindFramebuffer(GL_FRAMEBUFFER, m_msaa_fbo);
    else
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Framebuffer::id() const {
    if(is_msaa_enabled())
        return m_msaa_fbo;
    else
        return m_fbo;
}

GLuint Framebuffer::texture_id() const {
    return m_tex;
}

int Framebuffer::width() const {
    return m_width;
}

int Framebuffer::height() const {
    return m_height;
}

int Framebuffer::num_comps() const {
    return 4;
}

int Framebuffer::msaa() const {
    return m_msaa;
}

bool Framebuffer::is_msaa_enabled() const {
    return m_msaa > 0;
}

void Framebuffer::blit_to_single_sampling_fbo() const {
    if(!is_msaa_enabled())
        return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaa_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
    glBlitFramebuffer(0, 0, m_width, m_height,
                      0, 0, m_width, m_height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

std::vector<unsigned char> Framebuffer::get_color_buffer() const {
    blit_to_single_sampling_fbo();

    std::vector<unsigned char> buffer(m_width * m_height * 4);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return buffer;
}

bool Framebuffer::is_complete(GLuint fbo) const {
    GLint bound_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)bound_fbo);

    return ok;
}

void Framebuffer::setup_single_sampling_fbo() {
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Setup texture for color attachment
    {
        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Setup renderbuffer for depth attachment
    {
        glGenRenderbuffers(1, &m_depth_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depth_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // FBO attachments
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if(!is_complete(m_fbo))
        m_status = STATUS::SIMPLE_SAMPLING_FRAMEBUFFER_ERROR;
}

void Framebuffer::setup_msaa_fbo() {
    // Setup renderbuffer for color attachment
    {
        glGenRenderbuffers(1, &m_msaa_color_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_msaa_color_rbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_msaa, GL_RGBA8, m_width, m_height);

    }

    // Setup renderbuffer for depth
    {
        glGenRenderbuffers(1, &m_msaa_depth_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_msaa_depth_rbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_msaa, GL_DEPTH_COMPONENT, m_width, m_height);
    }

    // Generate multisampling FBO
    glGenFramebuffers(1, &m_msaa_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_msaa_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_msaa_color_rbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_msaa_depth_rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if(!is_complete(m_msaa_fbo))
        m_status = STATUS::MSAA_FRAMEBUFFER_ERROR;
}

void Framebuffer::delete_buffers() {
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteFramebuffers(1, &m_msaa_fbo);

    glDeleteRenderbuffers(1, &m_depth_rbo);
    glDeleteRenderbuffers(1, &m_msaa_color_rbo);
    glDeleteRenderbuffers(1, &m_msaa_depth_rbo);

    glDeleteTextures(1, &m_tex);

    m_fbo = 0;
    m_tex = 0;
    m_depth_rbo = 0;

    m_msaa_fbo = 0;
    m_msaa_color_rbo = 0;
    m_msaa_depth_rbo = 0;
}

GLint Framebuffer::get_valid_msaa(GLint val) const {
    GLint max_msaa;
    glGetIntegerv(GL_MAX_SAMPLES, &max_msaa);

    val = std::max(0, val);
    val = std::min(val, max_msaa);
    if(val % 2 != 0)
        val--;

    return val;
}

}
}
