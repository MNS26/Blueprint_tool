#define STB_IMAGE_IMPLEMENTATION
#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <lz4.h>
#include <lz4frame.h>
#include "stb/stb_image.h"
#include "smaz.h"

#include <cstdint>
#include <ctime>
#include <ostream>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "blueprint-packer.hpp"

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
  printf("res %d\n", res);
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
  printf("res %d\n", res);
  if (res != contents.size()) {
    fprintf(stderr, "cant write entire file\n");
  }
  fclose(fp);
}

#ifndef __WIN32
void print_banner() {
    fprintf(stdout, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━╮╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stdout, "║╱╱┃╭━╮┃┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ┃╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stdout, "║╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━╋╮ ╭╯╱╱╱┃ ┃╱╱┃ ┣━━━━━┳━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱║\n");
    fprintf(stdout, "║╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃ ━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┃ ┃╱╱╱╱┃ ┃╱╱┃ ┃ ╭━╮ ┃╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┃ ━━┫ ╭╯╱╱║\n");
    fprintf(stdout, "║╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃ ━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ╰━━╯ ┃ ┃ ┃ ┃╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┃ ━━┫ ┃╱╱╱║\n");
    fprintf(stdout, "║╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━━━━━━┻━╯ ╰━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱║\n");
    fprintf(stdout, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱Made by:┃ ┃╱Noah             (MS26)╱╱║\n");
    fprintf(stdout, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱Michael Bishop (Clever)╱╱║\n");
    fprintf(stdout, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");

}
void print_help() {
    fprintf(stdout, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║ This tool takes \"blueprint\" data and converts it to the following types:            ║\n");
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
    fprintf(stdout, "║ -o <path>                          path to outpout file                             ║\n");
    fprintf(stdout, "║ -O [binary, lz4, protobuf, json]   Output file type                                 ║\n");
    fprintf(stdout, "║ -j <path>                          path to outpout Json file                        ║\n");
    fprintf(stdout, "║ -b <path>                          path to outpout Binary file                      ║\n");
    fprintf(stdout, "║ -l <path lz4>                      path to output lz4 file                          ║\n");
    fprintf(stdout, "║ -P <path protobuf>                 path to output protobuf file                     ║\n");
    fprintf(stdout, "║ -j <path json>                     path to output json file                         ║\n");
    fprintf(stdout, "║ -U                                 dump UUID to text file                           ║\n");
    fprintf(stdout, "║ -D                                 dump Description to text file                    ║\n");
    fprintf(stdout, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}
#else
void print_banner() {
    fprintf(stdout, "Blueprint Unpacker\n");
    fprintf(stdout, "Made by: Noah             (MS26)\n");
    fprintf(stdout, "         Michael Bishop (Clever)\n");
    fprintf(stdout, "\n");
}
void print_help() {

    fprintf(stdout, "This tool takes \"blueprint\" images and converts it to the following types:\n");
    fprintf(stdout, "Raw binary                         (direct rip from the image)\n");
    fprintf(stdout, "lz4 commpressed + user data        (compressed vehicle data and user data)\n");
    fprintf(stdout, "protobuffer     + user data        (raw vehicle structure data and user data)\n");
    fprintf(stdout, "JSON            + user data        (parsed vehicle structure data and user data)\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "-H/-h                            shows this help screen\n");
    fprintf(stdout, "-p <path>                        path to png input file\n");
    fprintf(stdout, "-i <path>                        path to input file\n");
    fprintf(stdout, "-I [png, binary, lz4. protobuf]  input file type\n");
    fprintf(stdout, "-o <path>                        path to outpout file\n");
    fprintf(stdout, "-O [binary, lz4, protobuf, json] Output file type\n");
    fprintf(stdout, "-b <path>                        path to outpout Binary file\n");
    fprintf(stdout, "-l <path lz4>                    path to output lz4 file\n");
    fprintf(stdout, "-P <path protobuf>               path to output protobuf file\n");
    fprintf(stdout, "-j <path json>                   path to output json file\n");
    fprintf(stdout, "-U                               dump UUID to text file\n");
    fprintf(stdout, "-D                               dump Description to text file\n");
  }
#endif


int main(int argc, char *argv[]) {
  print_banner();

  int opt;
  string in_path;
  string in_type;

  string out_path;
  string out_type;

  bool dump_UUID = false;
  bool dump_Description = false;
  bool print_info = false;

  while ((opt = getopt(argc, argv, "p:b:l:j:P:o:O:i:UDfI:hH")) != -1) {
    switch (opt) {
    case 'p':
      in_path = optarg;
      in_type = "png";
      break;
    case 'b':
      out_path = optarg;
      out_type = "binary";
      break;
    case 'l':
      out_path = optarg;
      out_type = "lz4";
      break;
    case 'j':
      out_path = optarg;
      out_type = "json";
      break;
    case 'P':
      out_path = optarg;
      out_type = "protobuf";
      break;
    case 'o':
      out_path = optarg;
      break;
    case 'O':
      out_type = optarg;
      break;
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
    // different options

    uint8_t offset_header = 98;
    uint8_t CreatorNameSize = 0;
    uint32_t SaveGameVersion = 0;
    bool leagacy_file = false;
    uint32_t legacy_protobuf_size = 0;
    uint32_t lz4_size = 0;
    uint32_t uuid_size = 0;
    uint32_t smaz_size = 0;
    auto uuid_id_size = 0;
    uint8_t legacy_with_uuid[] ={0x08,0x05,0x12}; // UUID from legacy blueprint that got updated

    uint8_t prefix_uuid[] ={0x08,0x02,0x12}; // UUID prefix
    uint8_t prefix_smaz[] ={0x31,0xff}; // SMAZ prefix

    std::vector<char> CreatorName;
    std::vector<uint8_t> uuid;
    std::vector<uint8_t> smaz;
    std::string description;
    std::string title;
    

    std::vector<uint8_t> binary_data; // raw image pixels
    std::vector<uint8_t> lz4_data; // raw lz4 compressed data
    std::vector<uint8_t> protobuf_data;


    bool has_header = false;

    if (in_type == "png") {
      unpacker.extractFromImage(in_path);
//        uint8_t *image_data = NULL;
//        int width, height, channels;
//        image_data = stbi_load(in_path.c_str(), &width, &height, &channels, 4); // Force RGBA (4 channels)
//        if (!image_data) {
//            fprintf(stderr, "Failed to load image: %s\n", in_path.c_str());
//            return EXIT_FAILURE;
//        }
//
//        printf("Loaded image: %s (Width: %d, Height: %d, Channels: %d)\n", in_path.c_str(), width, height, channels);
//
//        // Call the extraction function
//        // This fills the buffer and return the size of the extracted data (yes... we can use it to trim the buffer... "shut up")
//        binary_data = unpacker.extract_data_from_image(image_data, width, height);
//        has_header = true;
//        unpacker.has_header = true;
//        stbi_image_free(image_data);
    }
    if (in_type == "binary") {
      unpacker.extractFromBinary(in_path);
//        binary_data = readFile(in_path);
//        if (binary_data.empty()) return EXIT_FAILURE;
//        has_header = true;
//        unpacker.has_header = true;
    }
    if (in_type == "lz4") {
      unpacker.extractFromlz4(in_path);
//      lz4_data = readFile(in_path);
//      if (lz4_data.empty()) return EXIT_FAILURE;
    }

    if (in_type == "protobuf") {
      unpacker.extractFromProtobuf(in_path);
    }
    unpacker.parse();
    if (unpacker.isLegacyBlueprint()) {
      fprintf(stdout,"Legacy Blueprint detected!\n");
      fprintf(stdout,"This file has no description or UUID\n");
      fprintf(stdout,"Available outputs are: Binary, Protobuf, Json\n");
      if (out_type == "lz4") {
        fprintf(stdout,"Unable to output: %s. Using default: Json.\n", out_type.c_str());
        out_type = "json";
        auto last_point = out_path.find_last_of(".");
        out_path.replace(last_point+1,last_point+5,out_type); // replacing extention with out_type
      }
    }
    
    if (out_type == "lz4") {
      writeFile(out_path, unpacker.getLz4());
    }

    if (!lz4_data.empty() && !unpacker.isLegacyBlueprint()) {
      protobuf_data.resize(MAX_LZ4_DECOMPRESSED_SIZXE);
      auto protobuf_size = unpacker.decompress_lz4(lz4_data.data(), lz4_data.size(), protobuf_data.data(), protobuf_data.size());
      protobuf_data.resize(protobuf_size);
    }

    if (leagacy_file) {
      protobuf_data.resize(legacy_protobuf_size);
      protobuf_data.assign(binary_data.begin()+offset_header, binary_data.begin()+offset_header+legacy_protobuf_size);
    }

    if (out_type == "binary") {
      writeFile(out_path, unpacker.getBinary());
    }

    if (out_type == "protobuf") {
      writeFile(out_path, unpacker.getProtobuf());
    }

    if (out_type == "json") {
//      string json_data;
//      unpacker.decompress_protobuf(protobuf_data);
      writeFile(out_path, unpacker.getVehicle());
    }
    if (dump_UUID) {
      auto old_out_path = out_path;
      auto last_point = out_path.find_last_of(".");
      out_path.replace(last_point,last_point+out_path.length()," uuid.txt"); // changing file name on the fly
      writeFile(out_path,uuid);
      out_path = old_out_path;
    }
    if (dump_Description) {
      auto old_out_path = out_path;
      auto last_point = out_path.find_last_of(".");
      out_path.replace(last_point,last_point+out_path.length()," description.txt"); // changing file name on the fly
      writeFile(out_path,(std::string)title.data());
      writeFile(out_path,(std::string)title + "\r\n"+description);
      out_path = old_out_path;
    }
    if (out_type == "info") {

    }

    // Cleanup

    return 0;
}


