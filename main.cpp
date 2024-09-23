#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdio.h>
#include <thread>
#include <chrono>

#include "Matrix.h"

typedef struct {
    unsigned char* image;
    int width;
    int height;
    int channels;
} Image;

void convolve(Image& input_image, Matrix<float>& conv_kernel, int passes) {
    // Convert input image to float
    std::vector<float> float_image(input_image.width * input_image.height * input_image.channels);
    for (int i = 0; i < input_image.width * input_image.height * input_image.channels; ++i) {
        float_image[i] = static_cast<float>(input_image.image[i]);
    }

    std::vector<float> output_image(float_image.size());

    int kernel_center_x = static_cast<int>(conv_kernel.columns) / 2;
    int kernel_center_y = static_cast<int>(conv_kernel.rows) / 2;

    // Normalize the kernel to ensure the sum is 1
    float kernel_sum = 0.0f;
    for (unsigned int i = 0; i < conv_kernel.rows; ++i) {
        for (unsigned int j = 0; j < conv_kernel.columns; ++j) {
            kernel_sum += *conv_kernel.get_element(i, j);
        }
    }
    conv_kernel *= (1.0f / kernel_sum);

    unsigned int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();

    auto process_chunk = [&](int start_y, int end_y) {
        for (int y = start_y; y < end_y; ++y) {
            for (int x = 0; x < input_image.width; ++x) {
                for (int c = 0; c < input_image.channels; ++c) {
                    float sum = 0.0f;
                    for (int ky = 0; ky < static_cast<int>(conv_kernel.rows); ++ky) {
                        for (int kx = 0; kx < static_cast<int>(conv_kernel.columns); ++kx) {
                            int img_x = std::clamp(x + kx - kernel_center_x, 0, input_image.width - 1);
                            int img_y = std::clamp(y + ky - kernel_center_y, 0, input_image.height - 1);
                            float kernel_val = *conv_kernel.get_element(ky, kx);
                            sum += kernel_val * float_image[(img_y * input_image.width + img_x) * input_image.channels + c];
                        }
                    }
                    output_image[(y * input_image.width + x) * input_image.channels + c] = sum;
                }
            }
        }
    };

    for (int pass = 0; pass < passes; ++pass) {
        threads.clear();
        int chunk_size = input_image.height / num_threads;
        for (unsigned int i = 0; i < num_threads; ++i) {
            int start_y = i * chunk_size;
            int end_y = (i == num_threads - 1) ? input_image.height : (start_y + chunk_size);
            threads.emplace_back(process_chunk, start_y, end_y);
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // Swap input and output for the next pass
        std::swap(float_image, output_image);
    }

    // End timing
    auto end_time = std::chrono::high_resolution_clock::now();

    // Compute total time taken
    std::chrono::duration<double> diff = end_time - start_time;
    double time_taken = diff.count();

    // Compute total floating point operations
    unsigned long long total_ops_convolution = static_cast<unsigned long long>(passes) * input_image.width * input_image.height * input_image.channels * 2ULL * conv_kernel.rows * conv_kernel.columns;
    unsigned long long total_ops_normalization = static_cast<unsigned long long>(input_image.width) * input_image.height * input_image.channels * 3ULL;
    unsigned long long total_ops = total_ops_convolution + total_ops_normalization;

    double flops = total_ops / time_taken;

    printf("Total time taken: %f seconds\n", time_taken);
    printf("Total floating point operations: %llu\n", total_ops);
    printf("FLOPS: %e\n", flops);
    printf("Performance: %f GFLOPS\n", flops / 1e9);

    // Normalize the image
    float min_val = *std::min_element(float_image.begin(), float_image.end());
    float max_val = *std::max_element(float_image.begin(), float_image.end());
    float range = max_val - min_val;

    // Convert back to unsigned char
    for (int i = 0; i < input_image.width * input_image.height * input_image.channels; ++i) {
        float normalized = (float_image[i] - min_val) / range * 255.0f;
        input_image.image[i] = static_cast<unsigned char>(std::clamp(normalized, 0.0f, 255.0f));
    }
}

int main() {
    int width, height, channels;
    unsigned char *img = stbi_load("image.jpg", &width, &height, &channels, 0);

    if (!img) {
        printf("Error in loading the image\n");
        return 1;
    }

    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

    // Create an Image struct for the loaded image
    Image input_image = {
        .image = img,
        .width = width,
        .height = height,
        .channels = channels
    };

    // conv kernel
    Matrix<float> conv_kernel(1.0/273, {
        {1,  4,  7,  4, 1},
        {4, 16, 26, 16, 4},
        {7, 26, 41, 26, 7},
        {4, 16, 26, 16, 4},
        {1,  4,  7,  4, 1}
    });

    convolve(input_image, conv_kernel, 150);

    // Save the convolved image
    stbi_write_jpg("image_blurred.jpg", input_image.width, input_image.height, input_image.channels, input_image.image, 100);

    // Free the image memory
    free(input_image.image);

    return 0;
}