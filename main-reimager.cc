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

#include <lz4.h>
#include <lz4frame.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

long fsize(FILE *fp){
    long prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

uint8_t* getImage(std::string filepath, int* imgWidth, int* imgHeight, int* imgChannels) {
  uint8_t* img = stbi_load(filepath.append(".png").c_str(), imgWidth, imgHeight, imgChannels, 0);
  if (!img) {
    fprintf(stderr, "Failed to load image: %s\n", filepath.c_str());
    return NULL;
  }
  printf("Loaded image: %s (Width: %d, Height: %d, Channels: %d)\n", filepath.c_str(), imgWidth, imgHeight, imgChannels);
  return img;
}

std::vector<uint8_t> extractBinaryData(uint8_t *img, int* imgWidth, int* imgHeight) {
  auto ImgHeight = *imgHeight;
  auto ImgWidth = *imgWidth;

  std::vector<uint8_t> temp;
  // should be done nicer but works i think
  if (!img)
    return temp;

  size_t offset = 0; // Track how much data has been written
  temp.resize((*imgWidth) * (*imgHeight) * 3);
  uint8_t *output_data = temp.data();

  // Extract binary data
  for (int y = ImgHeight - 1; y >= 0; y--) { // Start from the bottom row and work upwards
    for (int x = 0; x < ImgWidth; x++) { // Start from the left
      int index = (y * ImgWidth + x) * 4; // RGBA has 4 bytes per pixel
      unsigned char alpha = img[index + 3];
      if (alpha == 0) { // Alpha channel fully transparent
        memcpy(&output_data[offset], &img[index], 3); // Copy RGB only
        offset += 3;
      }
    }
  }
  stbi_image_free(img);
  return temp;
}


std::vector<uint8_t> readFile(std::string path) {
  std::vector<uint8_t> data;

  FILE *fp = fopen(path.c_str(), "rb");
  if (!fp) {
    fprintf(stderr, "Failed to open file\n");
    return data;
  }
  int size = fsize(fp);
  data.resize(size);

  int res = fread(data.data(), 1, size, fp);
  if (res != size) {
    fprintf(stderr, "unable to read entire file\n");
    data.resize(0);
    return data;
  }
  fclose(fp);
  return data;
}

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
    fprintf(stderr, "║╱╱╱┃╭━╮┃┃ ┃╱V1.1╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ╭━━╮ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
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
  int imgWidth = 0;
  int imgHeight = 0;
  int imgChannels = 0;

  std::vector<uint8_t> binary = extractBinaryData(
      getImage(
        imgPath,
        &imgWidth,
        &imgHeight,
        &imgChannels),
      &imgWidth,
      &imgHeight
  );
  // Reverse over the buffer to find the last non-null character
	// its not a fool proof way but should work (hopefully)
	size_t filled_size = 0;
	for (size_t i = binary.size(); i > 0; i--) {
		if (binary[i - 1] != '\0') {
			filled_size = i;  // Found the end
			break;
		}
	}
  binary.resize(filled_size);
  binary.shrink_to_fit();
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

  auto custom_data = binary.data();
  auto custom_data_len = binary.size();
  // Now calculate how many rows to add and round it up
  auto usedRows = std::max(ceil((newWidth*4)/custom_data_len),ceil(custom_data_len/(newWidth*4)));
//  auto usedRows = custom_data_len/(newWidth*4);

  // Fill the bottom area (the extra rows) with a background color (RGB = 0) and
  // embed the custom data into the alpha channel in the order: bottom-to-top, left-to-right.
  int data_index = 0;
  for (int y = newHeight-1; y >= newHeight-1-usedRows; y--) { // start from the bottom row and move upward
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
  for (uint8_t i=0; i<sizeof(uint32_t); i++)
    newImage[((newWidth - 1) * 4) + i] = image[((width - 1) * 4) + i];// (repacker.getHeaderSize() >> (24 - i * 8)) & 0xFF;

//  for (uint8_t i = 0; i < sizeof(uint32_t); i++) {
//    uint8_t a = image[((width*channels)-1)-i];
//    auto b = ((newWidth*4)-1)-i;
//    newImage[b] = a;
//  }
  printf("R: %i | G: %i | B: %i | A: %i\n",
        newImage[((newWidth-1)*4)+0],
        newImage[((newWidth-1)*4)+1],
        newImage[((newWidth-1)*4)+2],
        newImage[((newWidth-1)*4)+3]
      );
  stbi_write_png(newImgPath.append(".png").c_str(), newWidth, newHeight, 4, newImage.data(), newWidth*4);
  return 0;
}