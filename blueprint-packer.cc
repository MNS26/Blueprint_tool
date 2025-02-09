#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>
#include <lz4.h>
#include <lz4frame.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "stb/stb_image.h"
#include "smaz.h"
#include "blueprint-packer.hpp"
using namespace std;
//using namespace google::protobuf;
//using google::protobuf::util::TimeUtil;

blueprint_unpacker::blueprint_unpacker(/* args */)
{}

blueprint_unpacker::~blueprint_unpacker() {
	binary.clear();
	lz4Data.clear();
	protobuf.clear();
	smaz.clear();
	Vehicle.clear();
	UUID.clear();
	Title.clear();
	Description.clear();
  Tag.clear();
	Creator.clear();
  SteamToken.clear();
}

int64_t blueprint_unpacker::decompress_data_internal(uint8_t* srcBuffer,size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize,size_t filled, size_t alreadyConsumed, LZ4F_dctx* dctx) {
	int firstChunk = 1;
	size_t ret = 1;
	while (ret != 0) {
		// Load more data
		size_t readSize = firstChunk ? filled : srcBufferSize - alreadyConsumed; 
    firstChunk = 0;
		const void* srcPtr = (const char*)srcBuffer + alreadyConsumed;
    alreadyConsumed = 0;
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
			ret = LZ4F_decompress(dctx, dstBuffer, &dstSize, srcPtr,&srcSize,NULL);
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

int blueprint_unpacker::decompress_lz4(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize) {
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

bool blueprint_unpacker::decompress_protobuf() {
	assert(sgsdp.ParseFromArray(protobuf.data(), protobuf.size()));
	// Configure options for human-readable JSON
	google::protobuf::util::JsonPrintOptions options;
	options.add_whitespace = true; // Enable pretty printing
	//options.always_print_primitive_fields = true; // Optionally include unset fields
	options.preserve_proto_field_names = true; // Use Protobuf field names (instead of camelCase)

	auto status = google::protobuf::util::MessageToJsonString(sgsdp,&Vehicle, options);
	return status.ok();
}
void blueprint_unpacker::parse() {

    // those are only possible if we get the raw bin file or the entire PNG
    if (has_header) {

        offset_header += binary[offset_header]+1;         // SaveGamePNG size + own byte (17+1)
        SaveGameVersion = (uint32_t)binary[offset_header];// get PNG Save version (>4 means we have lz4 )
        offset_header += sizeof(SaveGameVersion);              // Move forward 4 bytes (Version num)
        offset_header += binary[offset_header]+1;         // (version string + own byte)
        offset_header += binary[offset_header]+1;         // (creator string + own byte)
        offset_header += binary[offset_header]+1;         // (structuresize string + own byte)
        if( SaveGameVersion > 4) {
        offset_header += binary[offset_header]+1;         // (ident string + own byte)
        offset_header += binary[offset_header]+1;         // (metasize string + own byte)
        offset_header += 22;                                   // skip 22 bytes of unknown data
        } else {
          offset_header += 18;                                 // skip 18 bytes of unknown data
          leagacy_file = true;                                 // Only true if we have a raw protobuf instead
        }
        CreatorNameSize = binary[offset_header];          // (Creator name string size)
        Creator.reserve(CreatorNameSize);
        offset_header++;
        Creator.assign(binary.begin() + offset_header, binary.begin() + offset_header + CreatorNameSize);
        offset_header += CreatorNameSize;
        if (!leagacy_file) {
          lz4_size = *(uint32_t*)(binary.data()+offset_header); // get LZ4 size
          offset_header += sizeof(lz4_size);
          uuid_size = *(uint32_t*)(binary.data()+offset_header); // get uuid size
          offset_header += sizeof(uuid_size);
          smaz_size = *(uint32_t*)(binary.data()+offset_header); // get smaz size
          offset_header += sizeof(smaz_size);
          offset_header += 2; // jump 2 forward 

          if(0 == memcmp(legacy_with_uuid, (uint8_t*)(binary.data()+ offset_header + lz4_size), 3)) {
            fprintf(stdout, "Updated legacy Blueprint Detected.\n");
            fprintf(stdout, "This file has no real UUID\n");
          }

          lz4Data = std::vector<uint8_t>(binary.begin() + offset_header, binary.begin() + offset_header+lz4_size);

          UUID = std::vector<uint8_t>(binary.data()+ offset_header + lz4_size + 4, binary.data()+ offset_header + lz4_size + uuid_size); // skip 4 initial bytes since they arent usefull

          smaz = std::vector<uint8_t>((binary.data()+ offset_header + lz4_size+ uuid_size)+1, binary.data()+ offset_header + lz4_size+ uuid_size + smaz_size); // skip first 1 byte

          Title.resize(      *(smaz.data()+ 2) );
          auto a = Title.size();
          Description.resize(*(smaz.begin()+ 2 + Title.size() + 2));
          Tag.resize(        *(smaz.begin()+ 2 + Title.size() + 2 + Description.size() + 2));
          Creator.resize(    *(smaz.begin()+ 2 + Title.size() + 2 + Description.size() + 2 + Tag.size() + 1));
          SteamToken.resize( *(smaz.begin()+ 2 + Title.size() + 2 + Description.size() + 2 + Tag.size() + 2 + Creator.size() + 2));

          std::vector<char> temp;
          temp.resize(2048);
          size_t size = smaz_decompress((char*)smaz.data(),smaz.capacity(),temp.data(),temp.capacity());
          
          int offset =2;// = *(uint8_t*)(smaz.data()+1)-1; // get offset of first character
          if(Title.size())
            Title       = std::string(temp.begin() + 1, temp.begin() + 1 + Title.size());
          if(Description.size())
            Description = std::string(temp.begin() + offset + Title.size() + 2, temp.begin() + 1 + Title.size() + 2 + Description.size());
          if(Tag.size())
            Tag         = std::string(temp.begin() + offset + Title.size() + 2 + Description.size() + 2, temp.begin() + offset + Title.size() + 2 + Description.size() + 2 + Tag.size());
          if(Creator.size())
            Creator     = std::string(temp.begin() + offset + Title.size() + 2 + Description.size() + 2 + Tag.size() + 2, temp.begin() + offset + Title.size() + 2 + Description.size() + 2 + Tag.size() + 2 + Creator.size());
          if(SteamToken.size())
            SteamToken  = std::string(temp.begin() + offset + Title.size() + 2 + Description.size() + 2 + Tag.size() + 2 + Creator.size() + 2, temp.begin() + offset + Title.size() + 2 + Description.size() + 2 + Tag.size() + 2 + Creator.size() + 2 + SteamToken.size());
          
        } else {
          legacy_protobuf_size = *(uint32_t*)(binary.data()+offset_header); // get legacy protobuf size
          offset_header += sizeof(legacy_protobuf_size); // shift over by 4
          offset_header += 3; // shift over by 3 (this lines up with protobuf @0xc0)
        }

    }

//    if (leagacy_file) {
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

}

bool blueprint_unpacker::extractFromImage(std::string filepath) {
	auto imgData = stbi_load(filepath.c_str(), &ImgWidth, &ImgHeight, &ImgChannels, 4);
  if (!imgData) {
    fprintf(stderr, "Failed to load image: %s\n", filepath.c_str());
    return false;
  }
  printf("Loaded image: %s (Width: %d, Height: %d, Channels: %d)\n", filepath.c_str(), ImgWidth, ImgHeight, ImgChannels);

  // Call the extraction function
  // This fills the buffer and return the size of the extracted data (yes... we can use it to trim the buffer... "shut up")
  extract_data_from_image(imgData, ImgWidth, ImgHeight);
  has_header = true;
  stbi_image_free(imgData);
  return true;
}

bool blueprint_unpacker::extractFromBinary(std::string filepath) {
  binary = readFile(filepath);
  if (binary.empty()) return false;
  has_header = true;
  return true;
}

bool blueprint_unpacker::extractFromlz4(std::string filepath) {
  lz4Data = readFile(filepath);
  return !lz4Data.empty();
}

bool blueprint_unpacker::extractFromProtobuf(std::string filepath) {
  protobuf = readFile(filepath);
  return !protobuf.empty();
}

// Function to extract binary data from image data
void blueprint_unpacker::extract_data_from_image(const unsigned char *image_data, int width, int height) {
	size_t offset = 0; // Track how much data has been written

	binary.resize(width * height * 3);
	uint8_t *output_data = binary.data();

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
}

long blueprint_unpacker::fsize(FILE *fp){
    long prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

void blueprint_unpacker::writeFile(string path, string contents) {
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

void blueprint_unpacker::writeFile(string path, std::vector<uint8_t> contents) {
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

std::vector<uint8_t> blueprint_unpacker::readFile(string path) {
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
