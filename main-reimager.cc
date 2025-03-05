#ifdef _WIN32
#  define _WIN32_WINNT 0x0A00
    #define WIN32_LEAN_AND_MEAN
    // Minimal Windows.h
    #include <sdkddkver.h>
    #include <windef.h>
    #include <winbase.h>
    #include <winuser.h>
    // WinCon needs GDI, which uses Escape. Satisfy deps
    #define NOGDI
    #if !defined(_MAC) || defined(_WIN32NLS)
    #include <winnls.h>
    #endif
    #ifndef _MAC
    #include <wincon.h>
    #include <winver.h>
    #endif
//    #include <stdio.h>
//    #include <stdint.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#endif

#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "blueprint-unpacker.hpp"

blueprint_unpacker unpacker(true);


#ifdef _WIN32
void enable_cmd_vt() {
  HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(h_stdout, &mode);
  mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(h_stdout, mode);
}
void change_charset_utf8() {
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);
}
#endif

void print_banner() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║╱╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━━━━━━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━╮┃┃ ┃╱V1.0╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ╭━━╮ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━┻╮ ╭╯╱╱╱┃ ╰━━╯ ┣━━━┳━┳━━━━━┳━━━━━┳━━━━━┳━━━┳━━╮╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃ ━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┃ ┃╱╱╱╱┃ ╭━╮ ╭┃ ━━╋━┫ ╻ ╻ ┃ ╭━╮ ┃ ╭━╮ ┃ ━━┫ ╭╯╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃ ━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ┃ ╰╮╰┫ ━━┫ ┃ ┃ ┃ ┃ ╭━╮ ┃ ╰━╯ ┃ ━━┫ ┃╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━╯  ╰━┻━━━┻━┻━┻━┻━┻━╯ ╰━┻━━━╮ ┣━━━┻━╯╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱ Made by: Noah Clever ╭━━━╯ ┃╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱ Vali        ╰━━━━━╯╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}

void print_help() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║ This tool takes a \"blueprint\" image and replacement image as input                  ║\n");
    fprintf(stderr, "║ and generates a new image with the blueprint data                                   ║\n");
    fprintf(stderr, "╠═════════════════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stderr, "║ The following options are available                                                 ║\n");
    fprintf(stderr, "║ -H/-h                              shows this help screen                           ║\n");
    fprintf(stderr, "║ -p <path>                          path to the blueprint file                       ║\n");
    fprintf(stderr, "║ -n <path>                          path to the new image file                       ║\n");
    fprintf(stderr, "║ -o <path>                          path and name to output image                    ║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
  enable_cmd_vt();
  change_charset_utf8();
#endif
  print_banner();

  int opt;
  std::string imgPath;
  std::string newImgPath;

  std::string outImgPath;


  while ((opt = getopt(argc, argv, "p:n:o:hH")) != -1) {
    switch (opt) {
    case 'p':
      imgPath = optarg;
      break;
    case 'n':
      newImgPath = optarg;
      break;
    case 'o':
      outImgPath = optarg;
      break;
    case 'H':
    case 'h':
      print_help();
      return EXIT_SUCCESS;
    }
  }

  if (imgPath.empty() || outImgPath.empty() || newImgPath.empty()) {
    print_help();
    return EXIT_SUCCESS;
  }
  unpacker.EnableBinaryOut();
  unpacker.extractFromImageData(imgPath);
  
  int width = 0;
  int height = 0;
  int channels = 0;
  auto image =  stbi_load(newImgPath.c_str(),&width,&height,&channels,0);

//  int newWidth = 512;
//  int newHeight = 512;
  int newWidth = width;
  int newHeight = height;
  size_t newSize = newWidth*newHeight*4;
  std::vector<uint8_t> newImage;

  newImage.resize(newSize);

  // Fill the canvas with a background color.
  // Here, we fill it with a transparent background (0,0,0,0).
  for (int i = 0; i < newWidth * newHeight; i++) {
    newImage[i * 4 + 0] = 0; // Red
    newImage[i * 4 + 1] = 0; // Green
    newImage[i * 4 + 2] = 0; // Blue
    newImage[i * 4 + 3] = 0; // Alpha (transparent)
  }

  // Compute offsets to center the small image on the canvas.
  int offset_x = (newWidth - width) / 2;
  int offset_y = (newHeight - height) / 2;


  // Copy the small image into the canvas at the computed offsets.
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int canvas_x = offset_x + x;
      int canvas_y = offset_y + y;
      // Check bounds just in case.
      if (canvas_x < 0 || canvas_x >= newWidth || canvas_y < 0 || canvas_y >= newHeight)
        continue;
            
      int canvas_index = (canvas_y * newWidth + canvas_x) * 4;
      int img_index = (y * width + x) * channels;
            
      newImage[canvas_index++] = image[img_index++];
      newImage[canvas_index++] = image[img_index++];
      newImage[canvas_index++] = image[img_index++];
      newImage[canvas_index++] = channels==3 ? 0xff : image[img_index++];
    }
  }

  auto data = unpacker.getBinary();
  auto custom_data = data.data();
  auto custom_data_len = data.size();
  // Now calculate how many rows to add and round it up
  auto usedRows = std::max(ceil(newWidth/custom_data_len),ceil(custom_data_len/newWidth));

  // Fill the bottom area (the extra rows) with a background color (RGB = 0) and
  // embed the custom data into the alpha channel in the order: bottom-to-top, left-to-right.
  int data_index = 0;
  for (int y = newHeight-1; y >= newHeight-usedRows; y--) { // start from the bottom row and move upward
    for (int x = 0; x < newWidth; x++) { // left to right in each row
      int pixel_index = (y * newWidth + x) * 4;
      // Set the RGB channels to a default value (here, 0; adjust if needed)
      if (data_index < custom_data_len) {
        newImage[pixel_index + 0] = custom_data[data_index++]; // Red
        newImage[pixel_index + 1] = custom_data[data_index++]; // Green
        newImage[pixel_index + 2] = custom_data[data_index++]; // Blue
        newImage[pixel_index + 3] = 0x00; // Alpha
      }
    }
  }
  for (uint8_t i = 0; i < sizeof(uint32_t); i++) {
    uint8_t a = image[((width*3)-1)-i];
    auto b = ((newWidth*4)-1)-i;
    newImage[b] = a;
  }
  stbi_write_png(newImgPath.append(".png").c_str(), newWidth, newHeight, 4, newImage.data(), newWidth*4);
  return 0;
}