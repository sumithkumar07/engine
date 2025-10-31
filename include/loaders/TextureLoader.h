#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace Engine {
namespace Loaders {

/**
 * @brief Texture data structure
 */
struct TextureData {
    uint8_t* pixels;        // Raw pixel data (RGBA format)
    int width;              // Image width
    int height;             // Image height
    int channels;           // Number of color channels (3=RGB, 4=RGBA)
    size_t dataSize;        // Total size in bytes
    
    TextureData() : pixels(nullptr), width(0), height(0), channels(0), dataSize(0) {}
    ~TextureData() {
        if (pixels) {
            delete[] pixels;
            pixels = nullptr;
        }
    }
    
    // Disable copy, enable move
    TextureData(const TextureData&) = delete;
    TextureData& operator=(const TextureData&) = delete;
    TextureData(TextureData&& other) noexcept
        : pixels(other.pixels), width(other.width), height(other.height), 
          channels(other.channels), dataSize(other.dataSize) {
        other.pixels = nullptr;
    }
    TextureData& operator=(TextureData&& other) noexcept {
        if (this != &other) {
            delete[] pixels;
            pixels = other.pixels;
            width = other.width;
            height = other.height;
            channels = other.channels;
            dataSize = other.dataSize;
            other.pixels = nullptr;
        }
        return *this;
    }
};

/**
 * @brief Texture loader for various image formats
 * 
 * Supports:
 * - PNG (Portable Network Graphics)
 * - JPG/JPEG (Joint Photographic Experts Group)
 * - BMP (Bitmap)
 * - TGA (Targa)
 * - And more via stb_image
 */
class TextureLoader {
public:
    TextureLoader();
    ~TextureLoader();

    /**
     * @brief Load texture from file (auto-detects format)
     * @param filePath Path to image file
     * @param textureData Output texture data
     * @return true if successful
     */
    bool LoadFromFile(const std::string& filePath, TextureData& textureData);

    /**
     * @brief Load texture using stb_image (header-only library)
     * @param filePath Path to image file
     * @param textureData Output texture data
     * @return true if successful
     */
    bool LoadWithStbImage(const std::string& filePath, TextureData& textureData);

    /**
     * @brief Get supported file extensions
     * @return Vector of supported extensions (e.g., ".png", ".jpg", ".bmp")
     */
    static std::vector<std::string> GetSupportedExtensions();

    /**
     * @brief Check if file format is supported
     * @param filePath File path to check
     * @return true if supported
     */
    static bool IsFormatSupported(const std::string& filePath);

private:
    std::string GetFileExtension(const std::string& filePath);
    std::string ToLower(const std::string& str);
};

} // namespace Loaders
} // namespace Engine

