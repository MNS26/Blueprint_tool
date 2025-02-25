#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>
#include <lz4.h>
#include <lz4frame.h>
#include <lz4frame_static.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "stb/stb_image_write.h"
#include "stb/stb_image.h"
#include "smaz.h"
#include "trailmakers.pb.h"
#include "blueprint-repacker.hpp"

extern "C"
{
#ifdef _WIN32
#pragma comment(lib, "Rpcrt4.lib")
#define _WIN32_WINNT 0x0A00
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#if !defined(_MAC) || defined(_WIN32NLS)
#include <winnls.h>
#endif
#include <rpc.h>
#include <rpcdce.h>
#else
#include <uuid/uuid.h>
#endif
}

using namespace google::protobuf;
using google::protobuf::util::TimeUtil;


blueprint_repacker::blueprint_repacker(/* args */ bool enableCustomSteamToken) {
  this->CustomSteamToken = enableCustomSteamToken;
}

blueprint_repacker::~blueprint_repacker() {
	binary.clear();
	lz4Data.clear();
	protobuf.clear();
	smaz.clear();
	Vehicle.clear();
	Uuid.clear();
	Title.clear();
	Description.clear();
  Tag.clear();
	Creator.clear();
  SteamToken.clear();
}

// filling up the initial header with a replica of a real file
bool blueprint_repacker::CreateFakeHeader() { 
	binary.resize(249);
	buffptr = binary.data();

	buffptr = (uint8_t*)memcpy(binary.data(),data1.begin(),data1.max_size());
	filledBytes = buffptr - binary.data();

	buffptr = (uint8_t*)memset(buffptr,SaveGameVersion,sizeof(SaveGameVersion));
	filledBytes = buffptr - binary.data();

	buffptr = (uint8_t*)memcpy(binary.data(),data2.begin(),data2.max_size());
	filledBytes = buffptr - binary.data();

	VehicleSizePtr = buffptr;
	buffptr = (uint8_t*)memset(buffptr,VehicleSize,sizeof(VehicleSize));
	filledBytes = buffptr - binary.data();

	UuidSizePtr = buffptr;
	buffptr = (uint8_t*)memset(buffptr,UuidSize,sizeof(UuidSize));
	filledBytes = buffptr - binary.data();

	SmazSizePtr = buffptr;
	buffptr = (uint8_t*)memset(buffptr,SmazSize,sizeof(SmazSize));
	filledBytes = buffptr - binary.data();

	buffptr = (uint8_t*)memcpy(binary.data(),data3.begin(),data3.max_size());
	filledBytes = buffptr - binary.data();
	
	return true;
}

bool blueprint_repacker::CompressToProto() { 
	std::string message;
	google::protobuf::util::JsonParseOptions options;
	options.ignore_unknown_fields = true; 
	auto status = google::protobuf::util::JsonStringToMessage(Vehicle, &sgsdp,options);
	size_t size = sgsdp.ByteSizeLong();
	protobuf.resize(size);
	bool result = sgsdp.SerializePartialToArray(&protobuf,protobuf.capacity());
	return (result == true && status.ok()) ? true : false;
}

void blueprint_repacker::CompressToLz4() { 
	LZ4F_compressionContext_t ctx;
  compressResult_t result = { 1, 0, 0 };  /* result for an error */
  long long count_in = 0, count_out, bytesToOffset = -1;

	size_t const ctxCreation = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
	size_t const outputBufCap	= LZ4F_compressBound(16*1024,&kPrefs);
	lz4Data.reserve(outputBufCap);
  if (!LZ4F_isError(ctxCreation)) {
    //result = blueprint_repacker::compress_internal(ctx, 16*1024);

    /* write frame header */
		size_t const headerSize = LZ4F_compressBegin(ctx, lz4Data.data(), lz4Data.capacity(), &kPrefs);
		if (LZ4F_isError(headerSize)) {
        printf("Failed to start compression: error %u \n", (unsigned)headerSize);
        return;
    }
	  count_out = headerSize;
    printf("Buffer size is %u bytes, header size %u bytes \n",
		       (unsigned)lz4Data.capacity(), (unsigned)headerSize);

		// since we walk along it we cant use them directly
		auto lz4Ptr = lz4Data.data();
		auto protoPtr = protobuf.data();

		/* stream buffer */
    for (;;) {
      size_t compressedSize;

      size_t const readSize = protobuf.size()-count_in;
      if (readSize == 0) break; /* nothing left to read from input file */
      count_in += readSize;
      compressedSize = LZ4F_compressUpdate(ctx,
	                                          lz4Ptr, lz4Data.size(),
                                            protoPtr, protobuf.size(),
                                            NULL);

      if (LZ4F_isError(compressedSize)) {
        printf("Compression failed: error %u \n", (unsigned)compressedSize);
        return;
      }

      printf("Writing %u bytes\n", (unsigned)compressedSize);
      //safe_fwrite(outBuff, 1, compressedSize, f_out);
      count_out += compressedSize;
    }

    /* flush whatever remains within internal buffers */
    size_t const compressedSize = LZ4F_compressEnd(ctx,
                                                lz4Ptr, lz4Data.size(),
                                                NULL);
    if (LZ4F_isError(compressedSize)) {
      printf("Failed to end compression: error %u \n", (unsigned)compressedSize);
      return;
    
    	printf("Writing %u bytes \n", (unsigned)compressedSize);
      //safe_fwrite(outBuff, 1, compressedSize, f_out);
      count_out += compressedSize;
    }

    result.size_in = count_in;
    result.size_out = count_out;
    result.error = 0;
	} else {
    printf("error : resource allocation failed \n");
	}
	LZ4F_freeCompressionContext(ctx);   /* supports free on NULL */

	/* resize with known output size */
	lz4Data.resize(result.size_out);

	/* set size of lz4 data */
	memset(VehicleSizePtr,(uint32_t)result.size_out,sizeof(VehicleSize));
}

blueprint_repacker::compressResult_t blueprint_repacker::compress_internal(LZ4F_compressionContext_t ctx,int chunk) {
    compressResult_t result = { 1, 0, 0 };  /* result for an error */
    long long count_in = 0, count_out, bytesToOffset = -1;

    /* write frame header */
    {   size_t const headerSize = LZ4F_compressBegin(ctx, lz4Data.data(), lz4Data.capacity(), &kPrefs);
        if (LZ4F_isError(headerSize)) {
            printf("Failed to start compression: error %u \n", (unsigned)headerSize);
            return result;
        }
        count_out = headerSize;
        printf("Buffer size is %u bytes, header size %u bytes \n",
                (unsigned)lz4Data.capacity(), (unsigned)headerSize);
        //fwrite(outBuff, 1, headerSize, f_out);
    }
		// since we walk along it we cant really use them directly
		auto lz4Ptr = lz4Data.data();
		auto protoPtr = protobuf.data();
    /* stream file */
    for (;;) {
      size_t compressedSize;

      size_t const readSize = protobuf.size()-count_in;
      if (readSize == 0) break; /* nothing left to read from input file */
      count_in += readSize;
      compressedSize = LZ4F_compressUpdate(ctx,
	                                          lz4Ptr, lz4Data.size(),
                                            protoPtr, protobuf.size(),
                                            NULL);


      if (LZ4F_isError(compressedSize)) {
        printf("Compression failed: error %u \n", (unsigned)compressedSize);
        return result;
      }

      printf("Writing %u bytes\n", (unsigned)compressedSize);
      //safe_fwrite(outBuff, 1, compressedSize, f_out);
      count_out += compressedSize;
    }

    /* flush whatever remains within internal buffers */
    {   size_t const compressedSize = LZ4F_compressEnd(ctx,
                                                lz4Ptr, lz4Data.size(),
                                                NULL);
        if (LZ4F_isError(compressedSize)) {
            printf("Failed to end compression: error %u \n", (unsigned)compressedSize);
            return result;
        }

        printf("Writing %u bytes \n", (unsigned)compressedSize);
        //safe_fwrite(outBuff, 1, compressedSize, f_out);
        count_out += compressedSize;
    }

    result.size_in = count_in;
    result.size_out = count_out;
    result.error = 0;
    return result;
}

void blueprint_repacker::GenerateUuid() {
// IDK OF ANY WINDOWS LIB TO DO VALID Uuid GEN
// OR HOW TO ADD IT TO NIX


  Uuid.reserve(40);
  Uuid[0]= UuidMarker;
  Uuid[1] = 0x02; // marker for using Uuid
  Uuid[2] = 0x12;
  Uuid[3] = 0x24; // Uuid length is 36
#ifdef _WIN32
  UUID uuid;
  RPC_CSTR uuidString = NULL;
  UuidCreate(&uuid);
  UuidToStringA(&uuid, &uuidString);
  memcpy(Uuid.data()+4,&uuidString,Uuid[3]);
//  Uuid.reserve(15);
//  Uuid[0]= UuidMarker;
//  Uuid[1] = 0x05; // marker for using legacy option
//  Uuid[2] = 0x12;
//  Uuid[3] = 0x0B; // Rand of 11 large (10 excluding the "-")
//  Uuid[4] = 0x2D;
//  for (int i = 0; i < (Uuid[3]-1);i++)
//    Uuid[i+4] = (uint8_t)(rand()%0xFF);
//  memset(UuidSizePtr,(uint32_t)Uuid.capacity(),sizeof(uint32_t));
#else
  uuid_generate(Uuid.data()+4);
//  memset(UuidSizePtr,(uint32_t)Uuid.capacity(),sizeof(uint32_t));
#endif
  memset(UuidSizePtr,(uint32_t)Uuid.capacity(),sizeof(uint32_t));
}

void blueprint_repacker::GenerateSmaz() {}

bool blueprint_repacker::GenerateBlueprint() { 
	
	CreateFakeHeader();	
	CompressToProto();
	CompressToLz4();
	GenerateUuid();
	GenerateSmaz();
	return false;
}
