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
#  define _WIN32_WINNT 0x0A00
    #endif
//    #include <stdio.h>
//    #include <stdint.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#endif
#define STB_IMAGE_IMPLEMENTATION

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <cstdint>
#include <ctime>
#include <ostream>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <lz4.h>
#include <lz4frame.h>
#include "smaz.h"
#include "stb/stb_image.h"
#include "blueprint-unpacker.hpp"



using namespace std;

blueprint_unpacker unpacker;


long fsize(FILE *fp){
    long prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

void writeFile(string path, string contents) {
  FILE *fp = fopen(path.c_str(), "wb");
  if (!fp) {
    fprintf(stderr, "Failed to open output file: %s\n", path.c_str());
    return;
  }
  int res = fwrite(contents.c_str(), 1, contents.size(), fp);
//  printf("res %d\n", res);
  if (res != contents.size()) {
    fprintf(stderr, "cant write entire file\n");
  }
  fclose(fp);
}

void writeFile(string path, std::vector<uint8_t> contents) {
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


uint8_t* getImage(std::string filepath, int* imgWidth, int* imgHeight, int* imgChannels) {
  uint8_t* img = stbi_load(filepath.c_str(), imgWidth, imgHeight, imgChannels, 0);
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
    fprintf(stdout, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━╮╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stdout, "║╱╱┃╭━╮┃┃ ┃╱V1.5╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ┃╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stdout, "║╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━┻╮ ╭╯╱╱╱┃ ┃╱╱┃ ┣━━━━━┳━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱║\n");
    fprintf(stdout, "║╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃ ━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┃ ┃╱╱╱╱┃ ┃╱╱┃ ┃ ╭━╮ ┃╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┃ ━━┫ ╭╯╱╱║\n");
    fprintf(stdout, "║╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃ ━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ╰━━╯ ┃ ┃ ┃ ┃╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┃ ━━┫ ┃╱╱╱║\n");
    fprintf(stdout, "║╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━━━━━━┻━╯ ╰━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱║\n");
    fprintf(stdout, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃ Made by: Noah F277f4 ╱╱╱╱║\n");
    fprintf(stdout, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯ ╱╱╱╱╱╱╱╱ Vali Clever ╱╱╱╱║\n");
    fprintf(stdout, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");

}

void print_help() {
    fprintf(stdout, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║ This tool takes a \"blueprint\" and converts it to the following types:               ║\n");
    fprintf(stdout, "║ Raw binary                         (direct rip from the image)                      ║\n");
    fprintf(stdout, "║ lz4 commpressed + user data        (compressed vehicle data and user data)          ║\n");
    fprintf(stdout, "║ protobuffer + user data            (raw vehicle structure data and user data)       ║\n");
    fprintf(stdout, "║ JSON + user data                   (parsed vehicle structure data and user data)    ║\n");
    fprintf(stdout, "╠═════════════════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stdout, "║ The following options are available                                                 ║\n");
    fprintf(stdout, "║ -H/-h                              shows this help screen                           ║\n");
    fprintf(stdout, "║ -p <path>                          path to png input file                           ║\n");
    fprintf(stdout, "║ -i <path>                          path to input file                               ║\n");
    fprintf(stdout, "║ -I [png, binary, lz4. protobuf]    input file type                                  ║\n");
    fprintf(stdout, "║ -o <path>                          path and name to output file                     ║\n");
//    fprintf(stdout, "║ -O [binary, lz4, protobuf, json]   Output file type                                 ║\n");
    fprintf(stdout, "║ -j <path>                          Enable json output                               ║\n");
    fprintf(stdout, "║ -b <path>                          Enable binary output                             ║\n");
    fprintf(stdout, "║ -l <path lz4>                      Enable lz4 output                                ║\n");
    fprintf(stdout, "║ -P <path protobuf>                 Enable protobuf output                           ║\n");
    fprintf(stdout, "║ -U                                 dump UUID to text file                           ║\n");
    fprintf(stdout, "║ -D                                 dump User Data to text file                      ║\n");
    fprintf(stdout, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
  enable_cmd_vt();
  change_charset_utf8();
#endif
  print_banner();

  int opt;
  string in_path;
  string in_type;

  string out_path;
  string out_type;

  bool dump_UUID = false;
  bool dump_Description = false;
  bool print_info = false;

  while ((opt = getopt(argc, argv, "p:bljPo:O:i:UDTfI:hH")) != -1) {
    switch (opt) {
    case 'p':
      in_path = optarg;
      in_type = "png";
      break;
    case 'b':
      unpacker.EnableBinaryOut();
      break;
    case 'l':
      unpacker.EnableLz4Out();
      break;
    case 'j':
      unpacker.EnableJsonOut();
      break;
    case 'P':
      unpacker.EnableProtobufOut();
      break;
    case 'o':
      out_path = optarg;
      break;
//    case 'O':
//      out_type = optarg;
//      break;
    case 'i':
      in_path = optarg;
      break;
    case 'I':
      in_type = optarg;
      break;
    case 'U':
      dump_UUID = true;
      break;
    case 'D':
      dump_Description = true;
    case 'T':
      unpacker.exportSteamToken();
      break;
    case 'f':
      print_info = true;
      out_type = "info";
      break;
    case 'H':
    case 'h':
      print_help();
      return EXIT_SUCCESS;
    }
  }

  if ((in_path.empty() || out_path.empty())&& (!print_info || !dump_Description || !dump_UUID)) {
    print_help();
    return EXIT_SUCCESS;
  }
  std::vector<char> CreatorName;
  std::vector<uint8_t> uuid;
  std::vector<uint8_t> smaz;
  std::string description;
  std::string title;

  std::vector<uint8_t> binary_data; // raw image pixels
  std::vector<uint8_t> lz4_data; // raw lz4 compressed data
  std::vector<uint8_t> protobuf_data;
  int imgWidth = 0;
  int imgHeight = 0;
  int imgChannels = 0;
  if (in_type == "png") {
    unpacker.setBinary(
      extractBinaryData(
        getImage(
          in_path,
          &imgWidth,
          &imgHeight,
          &imgChannels),
        &imgWidth,
        &imgHeight
    ));
  }
  if (in_type == "binary") {
    unpacker.setBinary(readFile(in_path));
    
  }
  if (in_type == "lz4") {
    unpacker.setlz4(readFile(in_path));
  }
  if (in_type == "protobuf") {
    unpacker.setProtobuf(readFile(in_path));
  }
  writeFile(out_path+".bin", unpacker.getBinary());

  unpacker.unpack();

//    if (unpacker.isLegacyBlueprint()) {
//      fprintf(stdout,"Legacy Blueprint detected!\n");
//      fprintf(stdout,"This file has no description or UUID\n");
//      fprintf(stdout,"Available outputs are: Binary, Protobuf, Json\n");
//      if (out_type == "lz4") {
//        fprintf(stdout,"Unable to output: %s. Using default: Json.\n", out_type.c_str());
//        out_type = "json";
//        auto last_point = out_path.find_last_of(".");
//        out_path.replace(last_point+1,last_point+5,out_type); // replacing extention with out_type
//      }
//    }

  if (unpacker.getBinaryOut()) {
    writeFile(out_path+".bin", unpacker.getBinary());
  }
  if (!unpacker.isLegacyBlueprint())
  {
    if (unpacker.getLz4Out()) {
      writeFile(out_path+".lz4", unpacker.getLz4());
    }
  } else {fprintf(stderr, "Legacy file detected! No LZ4 in this file.\n");}

  if (unpacker.getProtobufOut()) {
    writeFile(out_path+".protobuf", unpacker.getProtobuf());
  }

  if (unpacker.getJsonOut()) {
    writeFile(out_path+".json", unpacker.getVehicle());
  }

  if (dump_Description) {
    std::string temp("Title:\n");
    temp.append(unpacker.getTitle());
    temp.append("\n\rDescription:\n");
    temp.append(unpacker.getDescription());
    temp.append("\n\rTag:\n");
    temp.append(unpacker.getTag());
    temp.append("\n\rCreator:\n");
    temp.append(unpacker.getCreator());
    if (dump_UUID) {
      temp.append("\n\rUUID:\n");
      temp.append(unpacker.getUuid());
    }
    temp.append("\n\rSteamToken\n");
    temp.append(unpacker.getSteamToken());
    temp.append("\n");
    writeFile(out_path+" User Data.txt",temp);
  }
  if (out_type == "info") {
  }

    // Cleanup

  return 0;
}


