#pragma once

#include <glad/glad.h>

#include <string>
#include <unordered_map>
#include <filesystem>

namespace ca_essentials {
namespace renderer {

class TextureProvider {
public:
    enum class ERROR_CODE {
        SUCC,
        INVALID_FILE,
        INVALID_IMAGE,
        TEXTURE_ERROR,
    };
public:
    // Returns the texture id for the given image url and id
    static std::pair<ERROR_CODE, GLuint> add_texture(const std::string& fn, const std::string& id);

    // Returns texture id given its id
    static std::pair<bool, GLuint> texture_id(const std::string& id);

    // Destroys all textures
    static void clean();

private:
    static std::unordered_map<std::string, GLuint> s_textures;
};

} // namespace renderer
} // namespace ca_essentials
