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
#include "blueprint-repacker.hpp"

blueprint_repacker repacker;

long fsize(FILE *fp){
    long prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
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

std::string readFileStr(std::string path) {
  std::string data;

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




void print_banner() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║╱╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━━━━━━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━╮┃┃ ┃╱V1.0╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ╭━━╮ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━┻╮ ╭╯╱╱╱┃ ╰━━╯ ┣━━━┳━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃ ━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┫ ┃╱╱╱╱┃ ╭━╮ ╭┫ ━━┫╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┫ ━━┫ ╭╯╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃ ━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ┃ ╰╮╰┫ ━━┫╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┫ ━━┫ ┃╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━╯  ╰━┻━━━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃ Made by:  Noah  Clever ╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱ Vali ╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}

void print_help() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║ This tool takes raw \"blueprint\" data and converts it in to a image file:            ║\n");
    fprintf(stderr, "║ When using a text file make sure its formatted the same as the export tool!         ║\n");
    fprintf(stderr, "║ Surround strings with (\") when not using a text file and they have spaces in them   ║\n");
    fprintf(stderr, "║ If any of the fields are left empty they will be filled with default values         ║\n");
    fprintf(stderr, "╠═════════════════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stderr, "║ The following options are available                                                 ║\n");
    fprintf(stderr, "║                                                                                     ║\n");
    fprintf(stderr, "║ -h/-H              shows this help screen                                           ║\n");
    fprintf(stderr, "║ -p <path>          path to png input file                                           ║\n");
    fprintf(stderr, "║ -j <path>          path to Json file                                                ║\n");
//    fprintf(stderr, "║ -f <path>          path to Text file for Title, Description, Tag, Creator, UUID ... ║\n");
    fprintf(stderr, "║ -T <Title>         Title of the creation (single line)                   [optional] ║\n");
    fprintf(stderr, "║ -d <Description>   Description of the creation (single line)             [optional] ║\n");
    fprintf(stderr, "║ -t <Tag>                                                                 [optional] ║\n");
    fprintf(stderr, "║ -c <Creator>                                                             [optional] ║\n");
    fprintf(stderr, "║ -u <UUID>                                                                [optional] ║\n");
//    fprintf(stderr, "║ -s <SteamToken>                                                                     ║\n");
    fprintf(stderr, "║ -o <path>          Path to the png output file                                      ║\n");
//    fprintf(stderr, "║ -e                 Enable Embeddded mode.                        [MUST BE 1ST FLAG] ║\n");
//    fprintf(stderr, "║                    This mode outputs the data to stdout.                            ║\n");
//    fprintf(stderr, "║                    This mode is meant for use in other programs.                    ║\n");
//    fprintf(stderr, "║                    -j Now takes a  json string instead of a file                    ║\n");
//    fprintf(stderr, "║                    -f Now takes a string instead of a file that hold the data       ║\n");
    fprintf(stderr, "║ -C                 Enable Custom Tags                  [!! USE AT YOUT OWN RISK !!] ║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}

bool readFromTextFile(std::string path, blueprint_repacker& repacker) {}


void writeFile(std::string path, std::vector<uint8_t> contents) {
  FILE *fp = fopen(path.c_str(), "wb");
  if (!fp) {
    fprintf(stderr, "Failed to open output file: %s\n", path.c_str());
    return;
  }
  int res = fwrite(contents.data(), 1, contents.size(), fp);
//  printf("res %d\n", res);
  if (res != contents.size()) {
    fprintf(stderr, "cant write entire file\n");
  }
  fclose(fp);
}


int main(int argc, char *argv[]) {
#ifdef _WIN32
  enable_cmd_vt();
  change_charset_utf8();
#endif
  print_banner();

  int opt;
  std::string inPathPng;
  std::string inPathJson;
  std::string inPathText;

  std::string out_path;

  bool useTextFile = false;
  bool useEmbeddedMode = false;
//  repacker._TEST();
  
  while ((opt = getopt(argc, argv, "p:j:f:T:d:Ct:c:u:Ss:o:ehH")) != -1) {
    switch (opt) {
    case 'p':
      inPathPng = optarg;
      break;
    case 'e':
      useEmbeddedMode = true;
      break;
    case 'j':
      inPathJson = optarg;
      break;
    case 'f':
      inPathText = optarg;
      useTextFile = true;
      break;
    case 'T':
      repacker.setVehicleTitle(optarg);
      break;
    case 'd':
      repacker.setVehicleDescription(optarg);
      break;
    case 'C':
      repacker.UseCustomTags();
      break;
    case 't':
      repacker.setVehicleTag(optarg);
      break;
    case 'c':
      repacker.setVehicleCreator(optarg);
      break;
    case 'u':
      repacker.setVehicleUuid(optarg);
      break;
    case 'S':
      repacker.UseCustomToken();
      break;
    case 's':
      repacker.setVehicleSteamToken(optarg);
      break;
    case 'o':
      out_path = optarg;
      break;
    case 'H':
    case 'h':
      print_help();
      return EXIT_SUCCESS;
    }
  }

  if ((inPathPng.empty()&&!useEmbeddedMode) || (inPathJson.empty()&&!useEmbeddedMode) || (out_path.empty()&&!useEmbeddedMode)) {
    print_help();
    return EXIT_SUCCESS;
  }
  if (!useEmbeddedMode)
    repacker.setVehicleData(readFileStr(inPathJson));
  else
    repacker.setVehicleData(inPathJson);

//  if (useEmbeddedMode)
//    readFromTextFile(inPathJson,repacker);
//  else
  if (useTextFile)
    readFromTextFile(inPathJson,repacker);
  
  repacker.GenerateBlueprintData();

  int width = 0;
  int height = 0;
  int channels = 0;
  auto image =  stbi_load(inPathPng.c_str(),&width,&height,&channels,0);

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

  auto data = repacker.getBlueprintData();
  auto custom_data = data.data();
  auto custom_data_len = repacker.getBlueprintData().capacity();
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
    uint8_t a = (uint8_t)(repacker.getHeaderSize() >> (8*i));
    auto b = ((newWidth*4)-1)-i;
    newImage[b] = a;
  }
  stbi_write_png(out_path.append(".png").c_str(), newWidth, newHeight, 4, newImage.data(), newWidth*4);
  return 0;
}

