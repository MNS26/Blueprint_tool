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

int blueprint_unpacker::get_index_of(uint8_t* t, size_t cap, int val, uint32_t offset=0) {
  for(uint32_t i = offset; i < cap; i++) {
    if (t[i] == val)
    return i;
  }
  return 0;
}
int blueprint_unpacker::get_index_of(char* t, size_t cap, int val, uint32_t offset=0) {
  for(uint32_t i = offset; i < cap; i++) {
    if (t[i] == val)
    return i;
  }
  return 0;
}
size_t blueprint_unpacker::getSmazLEB(uint8_t* buffptr, uint8_t &consumed, uint16_t offset, uint8_t maxFromOffset=8) {
  size_t LEB=0;
  for(int i = 0; i < maxFromOffset; i++){
    LEB |= (buffptr[offset + i] & 0x7f) << (i * 7);
    consumed = 1+i;// it will always consume at least one byte... since if will act as a normal value otherwise
    if (!(buffptr[offset+i] & 0x80))
    break;
  }
  return LEB;
}
size_t blueprint_unpacker::getSmazLEB(char* buffptr, uint8_t &consumed, uint16_t offset, uint8_t maxFromOffset=8) {
  size_t LEB=0;
  for(int i = 0; i < maxFromOffset; i++){
    LEB |= (buffptr[offset + 1 + i] & 0x7f) << (i * 7);
    consumed = i;
    if (!(buffptr[offset+1+i] & 0x80))
    break;
  }
  return LEB;
}

void blueprint_unpacker::decompress_smaz() {

  // Text layout after smaz decode
  // 0x31 for title marker (can be skipped)
  // LEB128 length

  // 0x12 for description marker
  // LEB128 length

  // 0x1A for tag marker
  // LEB128 length

  // 0x65 for creator marker
  // LEB128 length

  // no marker for steam token
  // token +2 bytes after creator

  // 0x31 after token string (becomes \n why?)
  // followed by string "steamtoken"

  // layous is:
  // Title marker|Copy bytes|N bytes (input for copy)|Title len|Title data (if any)|Desciption marker|Description len|Description data (if any)|Tag marker|Tag len|Tag data|


  uint8_t consumedBytes = 0;
  size_t LEB128 = 0;
  size_t offset = 0;
  
  // splitting up all smaz data into its induvidual parts
  smaz = std::vector<uint8_t>((binary.data()+ offset_header + lz4_size+ uuid_size), binary.data()+ offset_header + lz4_size+ uuid_size + smaz_size);
  std::vector<uint8_t> tempString;
  tempString.resize(1024);
  tempString.resize(smaz_decompress((char*)smaz.data()+1,smaz.capacity(),(char*)tempString.data(),tempString.capacity()));

  
  LEB128 = getSmazLEB(tempString.data(), consumedBytes, offset);
  Title.resize(LEB128);
  Title.assign((char*)(tempString.data()+offset+consumedBytes),LEB128);
  
  offset = get_index_of(tempString.data(), tempString.size(), DescriptionMarker, offset+Title.size());
  LEB128 = getSmazLEB(tempString.data()+1, consumedBytes, offset);
  Description.resize(LEB128);
  Description.assign((char*)(tempString.data()+offset+consumedBytes+1),LEB128);


  offset = get_index_of(tempString.data(), tempString.size(), TagMarker, offset+Description.size());
  LEB128 = getSmazLEB(tempString.data()+1, consumedBytes, offset);
  Tag.resize(LEB128);
  Tag.assign((char*)(tempString.data()+offset+consumedBytes+1),LEB128);

  offset = get_index_of(tempString.data(), tempString.size(), CreatorMarker, offset+Tag.size());
  LEB128 = getSmazLEB(tempString.data()+1, consumedBytes, offset);
  Creator.resize(LEB128);
  Creator.assign((char*)(tempString.data()+offset+consumedBytes+1),LEB128);

  offset = get_index_of(tempString.data(), tempString.size(), SteamTokenMarker, offset+Creator.size());
  LEB128 = getSmazLEB(tempString.data()+1, consumedBytes, offset);
  SteamToken.resize(LEB128);
  SteamToken.assign((char*)(tempString.data()+offset+consumedBytes+1),LEB128);



}
void blueprint_unpacker::extractUUID() {
  // UUID data layout:
  // 0x08: starting marker
  // 0x02 or 0x05: 0x02 means actual UUID 0x05 means UUID placeholder(?)
  // 0x12: always the same
  // 0xXX size of UUID or number placrholder

  std::vector<uint8_t> temp = std::vector<uint8_t>(binary.data()+ offset_header + lz4_size, binary.data()+ offset_header + lz4_size + uuid_size);
  uint8_t uuid_len = 0;
  switch (temp[1])
  {
  case 0x02: // blueprint with proper UUID
    uuid_len = temp[3];
    UUID.resize(uuid_len);
    memcpy(UUID.data(), temp.data()+3,uuid_len);
    break;
  case 0x05: // legacy blueprint updated with UUID placeholder
    legacy_with_uuid = true;
    uuid_len = temp[3];
    UUID.resize(uuid_len);
    memcpy(UUID.data(), temp.data()+3,uuid_len);
    break;
  default: // not implemented version
    fprintf(stderr, "unknown UUID version: %u. Check with creator!\n", temp[1]);
    fprintf(stderr, "filling with dummy UUID!\n");
    UUID.resize(36);
    UUID = "00000000-0000-0000-0000-000000000000";
    break;
  }
  temp.clear();
}

void blueprint_unpacker::extractLz4() {
  lz4Data = std::vector<uint8_t>(binary.begin() + offset_header, binary.begin() + offset_header+lz4_size);
}
void blueprint_unpacker::extractProtobuf() {}
void blueprint_unpacker::extractJson() {}
void blueprint_unpacker::extractSmaz() {
  size_t totalOffset = offset_header + lz4_size+ uuid_size;
  smaz.assign(smazLength,*(binary.data()+totalOffset));

}
void blueprint_unpacker::parse() {

    // those are only possible if we get the raw bin file or the entire PNG
    if (has_header) {
        offset_header += binary[offset_header]+1;         // SaveGamePNG size + own byte (17+1)
        SaveGameVersion = (uint32_t)binary[offset_header];// get PNG Save version (>4 means we have lz4 )
        offset_header += sizeof(SaveGameVersion);         // Move forward 4 bytes (Version num)
        offset_header += binary[offset_header]+1;         // (version string + own byte)
        offset_header += binary[offset_header]+1;         // (creator string + own byte)
        offset_header += binary[offset_header]+1;         // (structuresize string + own byte)
        if( SaveGameVersion > 4) {
        offset_header += binary[offset_header]+1;         // (ident string + own byte)
        offset_header += binary[offset_header]+1;         // (metasize string + own byte)
        offset_header += 22;                              // skip 22 bytes of unknown data
        } else {
          offset_header += 18;                            // skip 18 bytes of unknown data
          leagacy_file = true;                            // Only true if we have a raw protobuf instead
        }
        offset_header += binary[offset_header]+1;          // (Creator name string size + own byte)
        if (!leagacy_file) {
          lz4_size = *(uint32_t*)(binary.data()+offset_header); // get LZ4 size
          offset_header += sizeof(lz4_size);
          uuid_size = *(uint32_t*)(binary.data()+offset_header); // get uuid size
          offset_header += sizeof(uuid_size);
          smaz_size = *(uint32_t*)(binary.data()+offset_header); // get smaz size
          offset_header += sizeof(smaz_size);
          offset_header += 2; // jump 2 forward 

          extractUUID();
          if(legacy_with_uuid) {
            fprintf(stdout, "Updated legacy Blueprint Detected.\n");
            fprintf(stdout, "This file has no real UUID\n");
          }
          extractLz4();
          extractSmaz();
          decompress_smaz();

        } else {
          legacy_protobuf_size = *(uint32_t*)(binary.data()+offset_header); // get legacy protobuf size
          offset_header += sizeof(legacy_protobuf_size); // shift over by 4
          offset_header += 3; // shift over by 3 (this lines up with protobuf @0xc0)
        }

    }
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
