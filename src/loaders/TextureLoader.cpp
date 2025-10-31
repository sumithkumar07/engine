#include "loaders/TextureLoader.h"
#include "core/Logger.h"
#include <algorithm>

// stb_image - single header image loading library
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#include "external/stb_image.h"

namespace Engine {
namespace Loaders {

TextureLoader::TextureLoader() {
    Core::Logger::Instance().Debug("TextureLoader created");
}

TextureLoader::~TextureLoader() {
    Core::Logger::Instance().Debug("TextureLoader destroyed");
}

bool TextureLoader::LoadFromFile(const std::string& filePath, TextureData& textureData) {
    if (!IsFormatSupported(filePath)) {
        Core::Logger::Instance().Error("TextureLoader: Unsupported file format: " + filePath);
        return false;
    }

    Core::Logger::Instance().Info("TextureLoader: Loading texture from " + filePath);

    // Use stb_image for all formats
    return LoadWithStbImage(filePath, textureData);
}

bool TextureLoader::LoadWithStbImage(const std::string& filePath, TextureData& textureData) {
    // Load image using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

    if (!data) {
        const char* reason = stbi_failure_reason();
        Core::Logger::Instance().Error("TextureLoader: Failed to load image: " + filePath + " - " + std::string(reason));
        return false;
    }

    // Store texture data
    textureData.width = width;
    textureData.height = height;
    textureData.channels = channels;
    textureData.dataSize = static_cast<size_t>(width * height * channels);

    // Allocate and copy pixel data
    textureData.pixels = new uint8_t[textureData.dataSize];
    std::memcpy(textureData.pixels, data, textureData.dataSize);

    // Free stb_image data
    stbi_image_free(data);

    Core::Logger::Instance().Info("TextureLoader: Loaded texture - " + std::to_string(width) + "x" + std::to_string(height) + 
                                  " (" + std::to_string(channels) + " channels, " + std::to_string(textureData.dataSize) + " bytes)");
    return true;
}

std::vector<std::string> TextureLoader::GetSupportedExtensions() {
    return {
        ".png",
        ".jpg",
        ".jpeg",
        ".bmp",
        ".tga"
    };
}

bool TextureLoader::IsFormatSupported(const std::string& filePath) {
    std::string ext = TextureLoader().GetFileExtension(filePath);
    auto supported = GetSupportedExtensions();
    return std::find(supported.begin(), supported.end(), ext) != supported.end();
}

std::string TextureLoader::GetFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) return "";
    return ToLower(filePath.substr(dotPos));
}

std::string TextureLoader::ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

} // namespace Loaders
} // namespace Engine

