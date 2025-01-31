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
#ifndef __WIN32
void print_banner() {
    fprintf(stdout, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━╮╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stdout, "║╱╱┃╭━╮┃┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ┃╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stdout, "║╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━╋╮ ╭╯╱╱╱┃ ┃╱╱┃ ┣━━━━━╮━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱║\n");
    fprintf(stdout, "║╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃┃━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┫ ┃╱╱╱╱┃ ┃╱╱┃ ┃ ╭━╮ ┫╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┫┃━━┫ ╭╯╱╱║\n");
    fprintf(stdout, "║╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃┃━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ╰━━╯ ┃ ┃ ┃ ┃╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┫┃━━┫ ┃╱╱╱║\n");
    fprintf(stdout, "║╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━━━━━━┻━╯ ╰━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱║\n");
    fprintf(stdout, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stdout, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱Made by: Noah (MS26)╱╱╱║\n");
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
    fprintf(stdout, "Blueprint Unpacker \n");
    fprintf(stdout, "Made by: Noah (MS26) \n");
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
  bool dump_UUID = false;
  bool dump_SMAZ = false;

  while ((opt = getopt(argc, argv, "p:j:o:O:i:I:b:hH")) != -1) {
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
      dump_SMAZ = true;
      break;
    case 'H':
    case 'h':
      print_help();
      return EXIT_SUCCESS;
    }
  }

  if (in_path.empty() || out_path.empty()) {
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
    std::vector<char> CreatorName;


    bool UUID_out = false; // are only if we have a bin or png in
    bool SMAZ_out = false; // are only if we have a bin or png in

    std::vector<uint8_t> binary_data; // raw image pixels
    std::vector<uint8_t> lz4_data; // raw lz4 compressed data
    std::vector<uint8_t> protobuf_data;

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

        offset_header += binary_data[offset_header]+1;         // SaveGamePNG size + own byte (17+1)
        SaveGameVersion = (uint32_t)binary_data[offset_header];// get PNG Save version (>4 means we have lz4 )
        offset_header += sizeof(SaveGameVersion);              // Move forward 4 bytes (Version num)
        offset_header += binary_data[offset_header]+1;         // (version string + own byte)
        offset_header += binary_data[offset_header]+1;         // (creator string + own byte)
        offset_header += binary_data[offset_header]+1;         // (structuresize string + own byte)
        if( SaveGameVersion > 4) {
        offset_header += binary_data[offset_header]+1;         // (ident string + own byte)
        offset_header += binary_data[offset_header]+1;         // (metasize string + own byte)
        offset_header += 22;                                   // skip 22 bytes of unknown data
        } else {
          offset_header += 18;                                 // skip 18 bytes of unknown data
          leagacy_file = true;                                 // Only true if we have a raw protobuf instead
        }
        CreatorNameSize = binary_data[offset_header];          // (Creator name string size)
        CreatorName.reserve(CreatorNameSize);
        offset_header++;
        CreatorName.assign(binary_data.begin() + offset_header, binary_data.begin() + offset_header + CreatorNameSize);
        offset_header += CreatorNameSize;
        if (!leagacy_file) {
          lz4_size = *(uint32_t*)(binary_data.data()+offset_header); // get LZ4 size
          offset_header += sizeof(lz4_size);
          uuid_size = *(uint32_t*)(binary_data.data()+offset_header); // get uuid size
          offset_header += sizeof(uuid_size);
          smaz_size = *(uint32_t*)(binary_data.data()+offset_header); // get smaz size
          offset_header += sizeof(smaz_size);
          offset_header += 2; // jump 2 forward 
                  
          printf("LZ4 size %d\n", lz4_size);
          printf("UUID size %d\n", uuid_size);
          printf("SMAZ size %d\n", smaz_size);

          // Pointer to the player UUID
          UUID = binary_data.data() + offset_header + lz4_size;

          // Pointer to vehicle SMAZ description
          SMAZChunk = binary_data.data() + offset_header + lz4_size + uuid_size;

          auto start = binary_data.begin() + offset_header;

          auto end = start + lz4_size;
          lz4_data = std::vector<uint8_t>(start, end);

        } else {
          legacy_protobuf_size = *(uint32_t*)(binary_data.data()+offset_header); // get legacy protobuf size
          offset_header += sizeof(legacy_protobuf_size); // shift over by 4
          offset_header += 3; // shift over by 3 (this lines up with protobuf @0xc0)

        }

    }

    if (leagacy_file) {
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

    if (in_type == "lz4") {
      lz4_data = readFile(in_path);
      if (lz4_data.empty()) return EXIT_FAILURE;
    }

    if (out_type == "lz4") {
      writeFile(out_path, lz4_data);
    }

    if (!lz4_data.empty() && !leagacy_file) {
      protobuf_data.resize(MAX_LZ4_DECOMPRESSED_SIZXE);
      auto protobuf_size = decompress_data(lz4_data.data(), lz4_data.size(), protobuf_data.data(), protobuf_data.size());
      protobuf_data.resize(protobuf_size);
    }

    if (leagacy_file) {
      protobuf_data.resize(legacy_protobuf_size);
      protobuf_data.assign(binary_data.begin()+offset_header, binary_data.begin()+offset_header+legacy_protobuf_size);
    }

    if (in_type == "protobuf") {
      protobuf_data = readFile(in_path);
    }

    if (out_type == "protobuf") {
      writeFile(out_path, protobuf_data);
    }

    if (out_type == "json") {
      string json_data;
      decompress_protobuf(protobuf_data, &json_data);
      writeFile(out_path, json_data);
    }

    // Cleanup

    return 0;
}


