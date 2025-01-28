#define STB_IMAGE_IMPLEMENTATION
#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <lz4.h>
#include <lz4frame.h>
#include "stb/stb_image.h"
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
typedef struct __attribute__((packed)) {
    const char stuff[99];
    const char SaveGamePNG_STR[22];
    const char Version_STR[8];
    const char Creator_STR[8];
    const char StructureByteSize_STR[18];
    const char StructureIdentifierSize_STR[24];
    const char StructureMetaByteSize[21];
    const uint8_t Constant[23];
    const uint8_t filler[12];
} Header;

typedef struct __attribute__((packed)) {
    uint32_t LZ4_size;
    uint32_t UUID_length;
    uint32_t SMAZ_chunk_length;
    uint8_t filler[2];
} VehicleDecodeData;

int64_t decompress_data_internal(uint8_t* srcBuffer,size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize,size_t filled, size_t alreadyConsumed, LZ4F_dctx* dctx) {
    int firstChunk = 1;
    size_t ret = 1;
    while (ret != 0) {
        // Load more data
        size_t readSize = firstChunk ? filled : srcBufferSize - alreadyConsumed; ; firstChunk = 0;
        const void* srcPtr = (const char*)srcBuffer + alreadyConsumed; alreadyConsumed = 0;
        const void* srcEnd = (const char*)srcPtr+readSize;
        if (readSize == 0) {
            printf("Decompress: not enough data\n");
            return -EXIT_FAILURE;
        }

        /* Decompress:
         * Continue while there is more input to read (srcPtr != srcEnd)
         * and the frame isn't over (ret != 0)
         */
        while (srcPtr < srcEnd && ret != 0) {
            // Any data within dst has been flushed at this stage
            size_t dstSize = dstBufferSize;
            size_t srcSize = (const char*)srcEnd - (const char*) srcPtr;
            ret = LZ4F_decompress(dctx, dstBuffer,&dstSize, srcPtr,&srcSize,NULL);
            if (LZ4F_isError(ret)) {
                printf("Decompression error: %s\n", LZ4F_getErrorName(ret));
                return -EXIT_FAILURE;
            }
            // Update data
            srcPtr = (const char*)srcPtr + srcSize;
        }
        assert(srcPtr <= srcEnd);
        /* Ensure all input data has been consumed.
         * It is valid to have multiple frames in the same file,
         * but this example only supports one frame.
         */
        if (srcPtr < srcEnd) {
            printf("Decompress: Trailing data left in buffer after frame\n");
            return -EXIT_FAILURE;
        }
    }
    // Reverse over the buffer to find the last non-null character
    // its not a fool proof way but should work (hopefully)
    size_t filled_size = 0;
    for (size_t i = dstBufferSize; i > 0; i--) {
        if (dstBuffer[i - 1] != '\0') {
            filled_size = i;  // Found the end
            break;
        }
    }
    return  filled_size; // return how many bytes we wrote 
    //return 0;
}


size_t decompress_file_allocDst(uint8_t *srcBuffer, size_t srcBufferSize,uint8_t *dstBuffer, size_t dstBufferSize, LZ4F_dctx *dctx) {
    // Read Frame header
    assert(srcBufferSize >= LZ4F_HEADER_SIZE_MAX);  /* ensure LZ4F_getFrameInfo() can read enough data */
    size_t const readSize = srcBufferSize;
    if (readSize == 0 ) {
        printf("Decompress: not enough input data\n");
            return EXIT_FAILURE;
    }

    LZ4F_frameInfo_t info;
    size_t consumedSize = readSize;
    size_t const fires = LZ4F_getFrameInfo(dctx, &info, srcBuffer,&consumedSize);
    if (LZ4F_isError(fires)) {
        printf("LZ4F_getFrameInfo error: %s\n", LZ4F_getErrorName(fires));
            return EXIT_FAILURE;
    }

    int const decompressionResult = decompress_data_internal(srcBuffer, srcBufferSize, dstBuffer, dstBufferSize, readSize-consumedSize, consumedSize, dctx);
    return decompressionResult;
}

static int decompress_data(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize) {
    LZ4F_dctx* dctx;
    {
        size_t const dctxStatus = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
        if (LZ4F_isError(dctxStatus)) {
            printf("LZ4F_dctx creation error: %s\n", LZ4F_getErrorName(dctxStatus));
        }
    }
    int const result = !dctx ? 1 /* error */ : decompress_file_allocDst(srcBuffer,srcBufferSize,dstBuffer,dstBufferSize,dctx);
    LZ4F_freeDecompressionContext(dctx);
    return result;

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

void print_banner() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━╮╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱┃╭━╮┃┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ┃╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━╋╮ ╭╯╱╱╱┃ ┃╱╱┃ ┣━━━━━╮━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱║\n");
    fprintf(stderr, "║╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃┃━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┫ ┃╱╱╱╱┃ ┃╱╱┃ ┃ ╭━╮ ┫╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┫┃━━┫ ╭╯╱╱║\n");
    fprintf(stderr, "║╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃┃━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ╰━━╯ ┃ ┃ ┃ ┃╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┫┃━━┫ ┃╱╱╱║\n");
    fprintf(stderr, "║╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━━━━━━┻━╯ ╰━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱Made by: Noah (MS26)╱╱╱║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");

}
void print_help() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║ This tool takes blueprint data and converts it to the following types:              ║\n");
    fprintf(stderr, "║ Raw binary (direct rip from the PNG)                                                ║\n");
    fprintf(stderr, "║ lz4 commpressed + user data text (compressed vehicle data and user data)            ║\n");
    fprintf(stderr, "║ protobuffer + user data text (raw vehicle structure data and user data)             ║\n");
    fprintf(stderr, "║ JSON + user data text (parsed vehicle structure data and user data)                 ║\n");
    fprintf(stderr, "╠═════════════════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stderr, "║ The following combinations can be used                                              ║\n");
    fprintf(stderr, "║ Usage:               <input_png> <output_pro>   (output png to protobuf) (default)  ║\n");
    fprintf(stderr, "║ Usage: -H to show this help screen                                                  ║\n");
    fprintf(stderr, "║ Usage: -I png -O bin <input_png> <output_bin>   (output png to bin)            (TBD)║\n");
    fprintf(stderr, "║ Usage: -I png -O lz4 <input_png> <output_lz4>   (output png to lz4)            (TBD)║\n");
    fprintf(stderr, "║ Usage: -I png -O pro <input_png> <output_proto> (output png to protobuf)            ║\n");
    fprintf(stderr, "║ Usage: -I png -O jso <input_png> <output_json>  (output png to Json)           (TBD)║\n");
    fprintf(stderr, "║ Usage: -I bin -O lz4 <input_bin> <output_lz4>   (output bin to lz4)            (TBD)║\n");
    fprintf(stderr, "║ Usage: -I bin -O pro <input_bin> <output_proto> (output bin to protobuf)       (TBD)║\n");
    fprintf(stderr, "║ Usage: -I bin -O jso <input_bin> <output_json>  (output bin to json)           (TBD)║\n");
    fprintf(stderr, "║ Usage: -I lz4 -O pro <input_lz4> <output_proto> (output lz4 to protobuf)       (TBD)║\n");
    fprintf(stderr, "║ Usage: -I lz4 -O jso <input_lz4> <output_json>  (output lz4 to json)           (TBD)║\n");
    fprintf(stderr, "║ Usage: -I pro -O jso <input_proto> <output_json> (output lz4 to json)          (TBD)║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
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

std::vector<uint8_t> readFile(string path) {
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

int main(int argc, char *argv[]) {
    print_banner();

  int opt;
  string in_path;
  string in_type;

  string out_path;
  string out_type;

  while ((opt = getopt(argc, argv, "p:j:o:O:i:I:")) != -1) {
    switch (opt) {
    case 'p':
      in_path = optarg;
      in_type = "png";
      break;
    case 'j':
      out_path = optarg;
      out_type = "json";
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
    }
  }

/*
    if (argc <3 || (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-H") == 0)) {
        print_help();
        return EXIT_FAILURE;
    }

    const char *in = argv[1];
    const char *mode_in = argv[2];
    const char *out = argv[3];
    const char *mode_out = argv[4];
    const char *input_file = argv[5];
    const char *output_file = argv[6];
*/

    // different options

    bool UUID_out = false; // are only if we have a bin or png in
    bool SMAZ_out = false; // are only if we have a bin or png in

    std::vector<uint8_t> binary_data; // raw image pixels
    std::vector<uint8_t> lz4_data; // raw lz4 compressed data

    std::vector<uint8_t> protobuf_data;

    Header* header = NULL;
    VehicleDecodeData* vehicle_decode_data = NULL;
    //uint8_t* Vehicle_Data = NULL;
    uint8_t* UUID = NULL;
    uint8_t* SMAZChunk = NULL;

    bool has_header = false;

    if (in_type == "png") {
        uint8_t *image_data = NULL;
        int width, height, channels;
        image_data = stbi_load(in_path.c_str(), &width, &height, &channels, 4); // Force RGBA (4 channels)
        if (!image_data) {
            fprintf(stderr, "Failed to load image: %s\n", in_path.c_str());
            return EXIT_FAILURE;
        }

        printf("Loaded image: %s (Width: %d, Height: %d, Channels: %d)\n", in_path.c_str(), width, height, channels);

        // Call the extraction function
        // This fills the buffer and return the size of the extracted data (yes... we can use it to trim the buffer... "shut up")
        binary_data = extract_data_from_image(image_data, width, height);
        has_header = true;
        stbi_image_free(image_data);
    }

    if (in_type == "binary") {
        binary_data = readFile(in_path);
        if (binary_data.empty()) return EXIT_FAILURE;
        has_header = true;
    }

    if (out_type == "binary") {
      writeFile(out_path, binary_data);
    }

    // those are only possible if we get the raw bin file or the entire PNG
    if (has_header) {
        // Overlay fixed header
        // This is just a struct to make sure we offset far enough into the buffer (this data is not needed)
        header = (Header*)binary_data.data();

        // Overlay data needed to get extraction values
        // This lines up with the needed UUID length, LZ4 size, and the SMAZ size (WHAT  THE  FUCK  WERE  THEY  ON  FOR  USING  SMAZ!?)
        vehicle_decode_data = (VehicleDecodeData*)(binary_data.data() + sizeof(Header));
        printf("LZ4 size %d\n", vehicle_decode_data->LZ4_size);

        // Pointer to the player UUID
        UUID = binary_data.data() + sizeof(Header) + sizeof(VehicleDecodeData) + vehicle_decode_data->LZ4_size;

        // Pointer to vehicle SMAZ description
        SMAZChunk = binary_data.data() + sizeof(Header) + sizeof(VehicleDecodeData) + vehicle_decode_data->LZ4_size + vehicle_decode_data->UUID_length;

        auto start = binary_data.begin() + sizeof(Header) + sizeof(VehicleDecodeData);
        auto end = start + vehicle_decode_data->LZ4_size;
        lz4_data = std::vector<uint8_t>(start, end);
    }

    if (in_type == "lz4") {
        lz4_data = readFile(in_path);
        if (lz4_data.empty()) return EXIT_FAILURE;
    }

    if (out_type == "lz4") {
      writeFile(out_path, lz4_data);
    }

    //###################
    //# POINTER FUCKERY #
    //#   INCOMMING!!   #
    //###################

    // Pointer to the LZ4 vehicle data
    // if we have either of them we can use the header as offset otherwise its 0

    //###########################
    //# LZ4 DECOMPRESSION HELL #
    //#     AAAAHHHHHHH!!!     #
    //##########################

    if (!lz4_data.empty()) {
      protobuf_data.resize(MAX_LZ4_DECOMPRESSED_SIZXE);
      auto protobuf_size = decompress_data(lz4_data.data(), lz4_data.size(), protobuf_data.data(), protobuf_data.size());
      protobuf_data.resize(protobuf_size);
    }

    if (in_type == "protobuf") {
      protobuf_data = readFile(in_path);
    }

    if (out_type == "protobuf") {
      writeFile(out_path, protobuf_data);
    }

    string json_data;
    decompress_protobuf(protobuf_data, &json_data);

    if (out_type == "json") {
      writeFile(out_path, json_data);
    }

    // Cleanup

    return 0;
}


