#include <string>
#include <iostream>

#include <Magick++.h>

#include "MagickImageSink.hpp"

MagickImageSink::MagickImageSink(std::string path, unsigned int width) : ImageSink(width), path(path) { }

void MagickImageSink::append(const std::vector<uint32_t> &pixels) {
    imagePixels.insert(imagePixels.end(), pixels.begin(), pixels.end());
}

void MagickImageSink::write() {
    Magick::Image image(width, imagePixels.size()/width, "BGRA", Magick::CharPixel, imagePixels.data());
    image.quality(100);
    image.opacity(0);
    image.rotate(-90);
    image.write(path);
}

