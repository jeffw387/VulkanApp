#pragma once
#include <stb_image.h>
#include <memory>
#include <vector>

namespace vka {
struct Bitmap {
  std::vector<unsigned char> data;
  uint32_t width, height;
  int32_t left, top;
  size_t byteLength;

  Bitmap(stbi_uc* pixelData, uint32_t width, uint32_t height, int32_t left,
         int32_t top, size_t byteLength)
      : data(pixelData, pixelData + byteLength),
        width(width),
        height(height),
        left(left),
        top(top),
        byteLength(byteLength) {}
};

inline Bitmap loadImageFromFile(std::string path) {
  // Load the texture into an array of pixel data
  int channels_i, width_i, height_i;
  auto pixels =
      stbi_load(path.c_str(), &width_i, &height_i, &channels_i, STBI_rgb_alpha);
  uint32_t width, height;
  width = static_cast<uint32_t>(width_i);
  height = static_cast<uint32_t>(height_i);
  Bitmap image = Bitmap(
      pixels, width, height, static_cast<int32_t>(std::ceil(width * -0.5f)),
      static_cast<int32_t>(std::ceil(height * 0.5f)), width * height * 4U);

  return image;
}
}  // namespace vka