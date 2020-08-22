#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>

std::uint8_t *load_raw_image(char const *path, int& width, int& height)
{
    int fd = ::open(path, O_RDONLY);
    if (fd < 0)
        throw std::runtime_error("Couldn\'t open file");

    std::uint32_t header[4];
    ::read(fd, header, sizeof(header));

    if (header[2] != 0xffff2233 || header[3] != 0)
    {
        ::close(fd);
        throw std::runtime_error("Bad header");
    }

    width = header[0];
    height = header[1];
    std::size_t size = width * height * 4;
    
    std::uint8_t *data = new std::uint8_t[size];
    ::read(fd, data, size);
    ::close(fd);

    return data;
}

void load_opencv_image(cv::Mat& image, std::uint8_t *__data, int width, int height)
{
    std::uint32_t *data = reinterpret_cast<std::uint32_t*>(__data);
    for (int i = 0; i < height; i++)
    {
        std::uint32_t *ptr = data + i * width;
        for (int j = 0; j < width; j++)
        {
            cv::Vec3b& pot = image.at<cv::Vec3b>(i, j);

            pot[0] = ptr[j] & 0xFF;
            pot[1] = (ptr[j] & 0xFF00) >> 8;
            pot[2] = (ptr[j] & 0xFF0000) >> 16;
        }
    }
}

int main(int argc, char const *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        const char *path = argv[i];
        int width;
        int height;

        std::uint8_t *raw_image = load_raw_image(path, width, height);
        cv::Mat image(height, width, CV_8UC3);

        load_opencv_image(image, raw_image, width, height);
        cv::imshow(path, image);

        delete[] raw_image;
        cv::waitKey();
    }
    return 0;
}
