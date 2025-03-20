
#define MAX_LZ4_DECOMPRESSED_SIZE (1024 * 1024 * 4)
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "google/protobuf/util/time_util.h"
#include "google/protobuf/util/json_util.h"
#include <lz4.h>
#include <lz4frame.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "smaz.h"
#include "blueprint-unpacker.hpp"
#ifdef _WIN32
#define EXPORT
#endif

#if defined __ELF__ 
  #define API __attribute((visibility("default")))
#elif defined EXPORT
  #define API __declspec(dllexport)
#else 
  #define API __declspec(dllimport)
#endif

using namespace google::protobuf;
using google::protobuf::util::TimeUtil;

blueprint_unpacker::blueprint_unpacker(/* args */bool enableSteamToken) // even though we have a option to export it as parameter
{this->enableSteamToken = enableSteamToken;}

blueprint_unpacker::~blueprint_unpacker() {
	binaryData.clear();
	Lz4Data.clear();
	protobufData.clear();
	uuidData.clear();
	smazData.clear();
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
    LZ4F_decompressOptions_t decomp = {0};
    decomp.skipChecksums=0;
  
		while (srcPtr < srcEnd && ret != 0) {
		  // Any data within dst has been flushed at this stage
			size_t dstSize = dstBufferSize;
			size_t srcSize = (const char*)srcEnd - (const char*) srcPtr;
			ret = LZ4F_decompress(dctx, dstBuffer, &dstSize, srcPtr,&srcSize,&decomp);
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


int blueprint_unpacker::decompress_lz4(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize) {
	// Read Frame header
	assert(srcBufferSize >= LZ4F_HEADER_SIZE_MAX);  /* ensure LZ4F_getFrameInfo() can read enough data */
	
  LZ4F_dctx* dctx;
	LZ4F_frameInfo_t info;
	
  size_t const dctxStatus = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
	if (LZ4F_isError(dctxStatus)) {
	  printf("LZ4F_dctx creation error: %s\n", LZ4F_getErrorName(dctxStatus));
	}
	
  size_t const readSize = srcBufferSize;
	if (readSize == 0 ) {
		printf("Decompress: not enough input data\n");
	  return EXIT_FAILURE;
	}

	size_t consumedSize = readSize;
	size_t const fires = LZ4F_getFrameInfo(dctx, &info, srcBuffer,&consumedSize);
	if (LZ4F_isError(fires)) {
		printf("LZ4F_getFrameInfo error: %s\n", LZ4F_getErrorName(fires));
		return EXIT_FAILURE;
	}

	int const result = !dctx ? 1 /* error */ : decompress_data_internal(srcBuffer, srcBufferSize, dstBuffer, dstBufferSize, readSize-consumedSize, consumedSize, dctx);//decompress_file_allocDst(srcBuffer,srcBufferSize,dstBuffer,dstBufferSize,dctx);
	LZ4F_freeDecompressionContext(dctx);
	return result;
}

bool blueprint_unpacker::decompress_protobuf() {
  sgsdp.ParseFromArray(protobufData.data(),protobufData.capacity());
	// Configure options for human-readable JSON
	google::protobuf::util::JsonPrintOptions options;
	options.add_whitespace = true; // Enable pretty printing
	//options.always_print_primitive_fields = true; // Optionally include unset fields

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

void blueprint_unpacker::extractSmaz() {
  smazData = std::vector<uint8_t>(binaryData.begin() + offset_header + VehicleLength+UUIDLength, binaryData.begin() + offset_header + VehicleLength + UUIDLength+smazLength);
}

void blueprint_unpacker::decompress_smaz() {

  uint8_t consumedBytes = 0;
  size_t LEB128 = 0;
  size_t offset = 0;
  
  std::vector<uint8_t> tempString;
  tempString.resize(smazLength*2);
  smaz_decompress((char*)smazData.data()+1,smazData.capacity(),(char*)tempString.data(),tempString.capacity());

  
  LEB128 = getSmazLEB(tempString.data(), consumedBytes, offset);
  Title.resize(LEB128);
  Title.assign((char*)(tempString.data()+offset+consumedBytes),LEB128);
  
  offset = get_index_of(tempString.data(), tempString.size(), DescriptionMarker, offset+Title.size());
  LEB128 = getSmazLEB(tempString.data()+1, consumedBytes, offset);
  Description.resize(LEB128);
  Description.assign((char*)(tempString.data()+offset+consumedBytes+1),LEB128);

  offset = get_index_of(tempString.data(), tempString.size(), TagMarker, offset+Description.size());
  auto a = (tempString.data()+offset+1);
  Tag.resize(*a);
  Tag.assign((char*)(tempString.data()+offset+1),*a);

  offset = get_index_of(tempString.data(), tempString.size(), CreatorMarker, offset+Tag.size());
  a = (tempString.data()+offset+1);
  Creator.resize(*a);
  Creator.assign((char*)(tempString.data()+offset+2),*a);

  offset = get_index_of(tempString.data(), tempString.size(), SteamTokenMarker, offset+Creator.size());
  a = (tempString.data()+offset+1);
  SteamToken.resize(*a);
  SteamToken.assign((char*)(tempString.data()+offset+2),*a);

  tempString.clear();
}
void blueprint_unpacker::extractUUID() {
  std::vector<uint8_t> uuidData = std::vector<uint8_t>(binaryData.data()+ offset_header + VehicleLength, binaryData.data()+ offset_header + VehicleLength + UUIDLength);

  // UUID data layout:
  // 0x08: starting marker
  // 0x02 or 0x05: 0x02 means actual UUID 0x05 means UUID placeholder(?)
  // 0x12: always the same
  // 0xXX size of UUID or number placrholder

  uint8_t uuid_len = 0;
  switch (uuidData[1])
  {
  case 0x02: // blueprint with proper UUID
    uuid_len = uuidData[3];
    UUID.resize(uuid_len);
    memcpy(UUID.data(), uuidData.data()+4,uuid_len);
    break;
  case 0x05: // legacy blueprint updated with UUID placeholder
    legacy_with_uuid = true;
    uuid_len = uuidData[3];
    UUID.resize(uuid_len);
    memcpy(UUID.data(), uuidData.data()+4,uuid_len);
    break;
  case 0x06: // No real UUID but has smaz and protobuf instead of lz4
    uuid_len = uuidData[3];
    UUID.resize(uuid_len);
    memcpy(UUID.data(), uuidData.data()+4,uuid_len);
    break;
  default: // not implemented version
//    fprintf(stderr, "unknown UUID version: %u. Check with creator!\n", uuidData[1]);
//    fprintf(stderr, "filling with dummy UUID!\n");
    UUID.resize(36);
    UUID = "00000000-0000-0000-0000-000000000000";
    break;
  }
}


void blueprint_unpacker::unpack() {
    // those are only possible if we get the raw bin file or the entire PNG
  if (has_header) {
      offset_header += binaryData[offset_header]+1;         // SaveGamePNG size + own byte (17+1)
      SaveGameVersion = (uint32_t)binaryData[offset_header];// get PNG Save version (>4 means we have lz4 )
      offset_header += sizeof(SaveGameVersion);         // Move forward 4 bytes (Version num)
      offset_header += binaryData[offset_header]+1;         // (version string + own byte)
      offset_header += binaryData[offset_header]+1;         // (creator string + own byte)
      offset_header += binaryData[offset_header]+1;         // (structuresize string + own byte)
      if( SaveGameVersion > 4) {
      offset_header += binaryData[offset_header]+1;         // (ident string + own byte)
      offset_header += binaryData[offset_header]+1;         // (metasize string + own byte)
      offset_header += 22;                              // skip 22 bytes of unknown data
      } else {
        offset_header += 18;                            // skip 18 bytes of unknown data
        leagacy_file = true;                            // Only true if we have a raw protobuf instead
      }
      offset_header += binaryData[offset_header]+1;          // (Creator name string size + own byte)
      if (!leagacy_file) {
        VehicleLength = *(uint32_t*)(binaryData.data()+offset_header); // get LZ4 size
        offset_header += sizeof(VehicleLength);
        UUIDLength = *(uint32_t*)(binaryData.data()+offset_header); // get uuid size
        offset_header += sizeof(UUIDLength);
        smazLength = *(uint32_t*)(binaryData.data()+offset_header); // get smaz size
        offset_header += sizeof(smazLength);
        offset_header += 2; // jump 2 forward 

      } else {
        VehicleLength = *(uint32_t*)(binaryData.data()+offset_header); // get legacy protobuf size
        offset_header += sizeof(VehicleLength); // shift over by 4
        offset_header += 3; // shift over by 3 (this lines up with protobuf @0xc0)
      }

      if(UUIDLength)
        extractUUID();
      if(smazLength){
        extractSmaz();
        decompress_smaz();
      }
    }
      Lz4Data = std::vector<uint8_t>(binaryData.begin() + offset_header, binaryData.begin() + offset_header + VehicleLength);

    if (toProtobuf){
      if (memcmp(Lz4Data.data(),Lz4MagicHeader.data(),Lz4MagicHeader.capacity())==0) {
        hasLz4MagicHeader = true;
        protobufData.resize(MAX_LZ4_DECOMPRESSED_SIZE);
        protobufData.resize(decompress_lz4(Lz4Data.data(), Lz4Data.size(), protobufData.data(), protobufData.size()));
      }else {
//        fprintf(stderr, "This Blueprint has no LZ4 magic header\n");
//        fprintf(stderr, "Asumming it has Protobuf instead\n");
        protobufData = std::vector<uint8_t>(Lz4Data);
      }
      decompress_protobuf();
    }
    if(legacy_with_uuid) {
//      fprintf(stderr, "Updated legacy Blueprint Detected.\n");
//      fprintf(stderr, "This file has no real UUID\n");
    }
  }

//void blueprint_unpacker::extractFromImageData(std::vector<uint8_t> data) {
//  binaryData = std::vector<uint8_t>(data);
//  has_header = true;
//}

void blueprint_unpacker::setBinary(std::vector<uint8_t> data) {
  binaryData = std::vector<uint8_t>(data);
  has_header = true;
}

void blueprint_unpacker::setlz4(std::vector<uint8_t> data) {
  Lz4Data = std::vector<uint8_t>(data);
  HasLz4 = true;
  HasProtobuf = true;
}

void blueprint_unpacker::setProtobuf(std::vector<uint8_t> data) {
  protobufData = std::vector<uint8_t>(data);
  HasProtobuf = true;
}