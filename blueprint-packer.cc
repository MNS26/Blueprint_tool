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
//#include "stb/stb_image.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>
#include "trailmakers.pb.h"
#include "smaz.h"
#include "blueprint-packer.hpp"
using namespace std;
//using namespace google::protobuf;
//using google::protobuf::util::TimeUtil;
StructureGraphSaveDataProto sgsdp;

blueprint_unpacker::blueprint_unpacker(/* args */)
{
}

blueprint_unpacker::~blueprint_unpacker() {}

int64_t blueprint_unpacker::decompress_data_internal(uint8_t* srcBuffer,size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize,size_t filled, size_t alreadyConsumed, LZ4F_dctx* dctx) {
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


size_t blueprint_unpacker::decompress_file_allocDst(uint8_t *srcBuffer, size_t srcBufferSize,uint8_t *dstBuffer, size_t dstBufferSize, LZ4F_dctx *dctx) {
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

int blueprint_unpacker::decompress_data(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize) {
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


// Function to extract binary data from image data
std::vector<uint8_t> blueprint_unpacker::extract_data_from_image(const unsigned char *image_data, int width, int height) {
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

size_t blueprint_unpacker::decompress_protobuf(std::vector<uint8_t> protobuf, string *json_buffer) {

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
