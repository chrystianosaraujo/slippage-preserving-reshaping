#include <ca_essentials/renderer/texture_provider.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


namespace ca_essentials {
namespace renderer {

std::unordered_map<std::string, GLuint> TextureProvider::s_textures;

std::pair<TextureProvider::ERROR_CODE, GLuint>
TextureProvider::add_texture(const std::string& fn, const std::string& id) {
    namespace fs = std::filesystem;

    if(!fs::is_regular_file(fn))
        return {TextureProvider::ERROR_CODE::INVALID_FILE, 0};

    int w = 0;
    int h = 0;
    int c = 0;
    unsigned char *image = stbi_load(fn.c_str(), &w, &h, &c, STBI_rgb_alpha);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if(c == 3)      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB , w, h, 0, GL_RGB , GL_UNSIGNED_BYTE, image);
    else if(c == 4) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    else {
        glDeleteTextures(1, &tex);
        return {TextureProvider::ERROR_CODE::INVALID_IMAGE, 0};
    }

    TextureProvider::s_textures.insert({id, tex});

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(image);

    return {TextureProvider::ERROR_CODE::SUCC, tex};
}

std::pair<bool, GLuint>
TextureProvider::texture_id(const std::string& id) {
    auto itr = TextureProvider::s_textures.find(id);
    if(itr == TextureProvider::s_textures.end())
        return {false, 0};
    else
        return {true, itr->second};
}

void TextureProvider::clean() {
    for(const auto& [id, tex] : TextureProvider::s_textures)
        glDeleteTextures(1, &tex);
}

} // namespace renderer
} // namespace ca_essentials
