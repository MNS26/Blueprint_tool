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

    #include <stdio.h>
    #include <stdint.h>
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
    fprintf(stderr, "║╱╱╱┃╭━╮┃┃ ┃╱V0.0╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ╭━━╮ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━┻╮ ╭╯╱╱╱┃ ╰━━╯ ┣━━━┳━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃ ━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┫ ┃╱╱╱╱┃ ╭━╮ ╭┫ ━━┫╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┫ ━━┫ ╭╯╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃ ━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ┃ ╰╮╰┫ ━━┫╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┫ ━━┫ ┃╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━╯  ╰━┻━━━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃ Made by:  Noah  Clever ╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
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
    fprintf(stderr, "║ -f <path>          path to Text file for Title, Description, Tag, Creator, UUID ... ║\n");
    fprintf(stderr, "║ -t <Title>         Title of the creation (single line)                   [optional] ║\n");
    fprintf(stderr, "║ -d <Description>   Description of the creation (single line)             [optional] ║\n");
    fprintf(stderr, "║ -T <Tag>                                                                 [optional] ║\n");
    fprintf(stderr, "║ -c <Creator>                                                             [optional] ║\n");
    fprintf(stderr, "║ -u <UUID>                                                                [optional] ║\n");
//    fprintf(stderr, "║ -s <SteamToken>                                                                     ║\n");
    fprintf(stderr, "║ -o <path>          Path to the png output file                                      ║\n");
    fprintf(stderr, "║ -e                 Enable Embeddded mode.                        [MUST BE 1ST FLAG] ║\n");
    fprintf(stderr, "║                    This mode outputs the data to stdout.                            ║\n");
    fprintf(stderr, "║                    This mode is meant for use in other programs.                    ║\n");
    fprintf(stderr, "║                    -j Now takes a  json string instead of a file                    ║\n");
//    fprintf(stderr, "║                    -f Now takes a string instead of a file that hold the data       ║\n");
    fprintf(stderr, "║ -C                 Enable Custom Tags                  [!! USE AT YOUT OWN RISK !!] ║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}

bool readFromTextFile(std::string path, blueprint_repacker& repacker) {}

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
  
  while ((opt = getopt(argc, argv, "p:j:f:t:d:CT:c:u:Ss:o:ehH")) != -1) {
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
    case 't':
      repacker.setVehicleTitle(optarg);
      break;
    case 'd':
      repacker.setVehicleDescription(optarg);
      break;
    case 'C':
      repacker.UseCustomTags();
      break;
    case 'T':
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

  // Now calculate how many rows to add and round it up
  auto extraRows = ceil(repacker.getBlueprintData().size()/width);

  int newHeight = height+extraRows;
  // png may not have alpha, so we will add it
  size_t newImageSize = width*newHeight*4;
  std::vector<uint8_t> newImgData;
  // Copy the original image into the new buffer (top part)
  for (int i = 0; i <(width*height*channels);(channels==3) ? i+=3:i+=4) {
    newImgData.push_back(image[i]);
    newImgData.push_back(image[i+1]);
    newImgData.push_back(image[i+2]);
    if (channels==3)
      newImgData.push_back(0xFF);
    else
      newImgData.push_back(image[i+3]);
  }
  newImgData.resize(newImageSize);

  auto data = repacker.getBlueprintData();
  auto custom_data = data.data();
  auto custom_data_len = repacker.getBlueprintData().size();
  // Fill the bottom area (the extra rows) with a background color (RGB = 0) and
  // embed the custom data into the alpha channel in the order: bottom-to-top, left-to-right.
  int data_index = 0;
  for (int y = newHeight - 1; y >= height; y--) { // start from the bottom row and move upward
    for (int x = 0; x < width; x++) { // left to right in each row
      int pixel_index = (y * width + x) * channels;
      // Set the RGB channels to a default value (here, 0; adjust if needed)
      if (data_index < custom_data_len) {
        newImgData[pixel_index + 0] = custom_data[data_index++]; // Red
        newImgData[pixel_index + 1] = custom_data[data_index++]; // Green
        newImgData[pixel_index + 2] = custom_data[data_index++]; // Blue
        newImgData[pixel_index + 3] = 0; // Alpha
      }
    }
  }

  stbi_write_png(out_path.c_str(), width, newHeight, 4, newImgData.data(), width*4);
  return 0;
}


