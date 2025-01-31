// needed data
//
// JSON
// UUID
// SMAZ

#define STB_IMAGE_IMPLEMENTATION
#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <lz4.h>
#include <lz4frame.h>
#include "stb/stb_image_write.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>
#include "trailmakers.pb.h"

using namespace std;
//using namespace google::protobuf;
//using google::protobuf::util::TimeUtil;
StructureGraphSaveDataProto sgsdp;

static int compress_data(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize) {
  LZ4F_compressionContext_t ctx;
  size_t const ctxCtreation = LZ4F_createCompressionContext(&ctx,LZ4F_VERSION);

  LZ4F_preferences_t ctxPrefs;
  ctxPrefs.autoFlush = true;
  ctxPrefs.compressionLevel = 7;
  size_t outputBufCapacity = LZ4F_compressBound(16*1024,&ctxPrefs);
  assert(dstBufferSize>outputBufCapacity);
  
  
}


long fsize(FILE *fp){
    long prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

// Function to extract binary data from image data
std::vector<uint8_t> extract_data_from_image(const unsigned char *image_data, int width, int height) {
    size_t offset = 0; // Track how much data has been written

    std::vector<uint8_t> data;
    data.resize(width * height * 3);
    uint8_t *output_data = data.data();

    // Extract binary data
    for (int y = height - 1; y >= 0; y--) { // Start from the bottom row and work upwards
        for (int x = 0; x < width; x++) { // Start from the left
            int index = (y * width + x) * 4; // RGBA has 4 bytes per pixel
            unsigned char alpha = image_data[index + 3];
            if (alpha == 0) { // Alpha channel fully transparent
                memcpy(&output_data[offset], &image_data[index], 3); // Copy RGB only
                offset += 3;
            }
        }
    }
    return data;
}

size_t decompress_protobuf(std::vector<uint8_t> protobuf, string *json_buffer) {

    assert(sgsdp.ParseFromArray(protobuf.data(), protobuf.size()));
    // Configure options for human-readable JSON
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true; // Enable pretty printing
    //options.always_print_primitive_fields = true; // Optionally include unset fields
    options.preserve_proto_field_names = true; // Use Protobuf field names (instead of camelCase)

    std::string message;
    google::protobuf::util::MessageToJsonString(sgsdp,&message, options);

    *json_buffer = message;
    return message.size();;

}

#ifndef __WIN32
void print_banner() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║╱╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━━━━━━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━╮┃┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ╭━━╮ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━╋╮ ╭╯╱╱╱┃ ╰━━╯ ┣━━━┳━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃ ━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┫ ┃╱╱╱╱┃ ╭━╮ ╭┫ ━━┫╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┫ ━━┫ ╭╯╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃ ━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ┃ ╰╮╰┫ ━━┫╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┫ ━━┫ ┃╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━╯  ╰━┻━━━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱Made by: Noah (MS26)╱╱╱║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");

}

void print_help() {
    fprintf(stdout, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║ This tool takes \"blueprint\" data and converts it in to a image file:                ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "╠═════════════════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stdout, "║ The following options are available                                                 ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "║ -E                                                                                  ║\n");
    fprintf(stdout, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}
#else
void print_banner() {
    fprintf(stdout, "Blueprint Repacker \n");
    fprintf(stdout, "Made by: Noah (MS26) \n");
    fprintf(stdout, "\n");
}
void print_help() {

    fprintf(stdout, "This tool takes \"blueprint\" data and converts it in to a image file:\n");
    fprintf(stdout, " \n");
    fprintf(stdout, " \n");
    fprintf(stdout, " \n");
    fprintf(stdout, " \n");
    fprintf(stdout, "\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
    fprintf(stdout, "-E\n");
  }
#endif


int main(int argc, char *argv[]) {
  print_banner();
  print_help();
  return 0;
}


