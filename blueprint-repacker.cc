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

#include "smaz.h"
#include "trailmakers.pb.h"
#include "blueprint-repacker.hpp"

extern "C"
{
#ifdef _WIN32
#define EXPORT
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

#if defined __ELF__ 
  #define API __attribute((visibility("default")))
#elif defined EXPORT
  #define API __declspec(dllexport)
#else 
  #define API __declspec(dllimport)
#endif

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
  UuidStr.clear();
	Title.clear();
	Description.clear();
  Tag.clear();
	Creator.clear();
  SteamToken.clear();
}

std::size_t blueprint_repacker::WriteUleb128(std::vector<char>& dest, unsigned long val) {
  std::size_t count = 0;
  do {
    unsigned char byte = val & 0x7f;
    val >>= 7;

    if (val != 0)
      byte |= 0x80;  // mark this byte to show that more bytes will follow

    dest.push_back(byte);
    count++;
  } while (val != 0);
  return count;
}

// filling up the initial header with a replica of a real file
void blueprint_repacker::CreateFakeHeader() { 
  for (int i=0; i<data1.size();i++)
    Header.push_back(data1[i]);

  for (int i=0; i<sizeof(SaveGameVersion);i++)
    Header.push_back((uint8_t)(SaveGameVersion>>8*i)&0xff);

  for (int i=0; i<data2.size();i++)
    Header.push_back(data2[i]);
  
  for (int i=0; i<sizeof(uint32_t);i++)
    Header.push_back((uint8_t)((uint32_t)lz4Data.size()>>8*i)&0xff);
  
  for (int i=0; i<sizeof(uint32_t);i++)
    Header.push_back((uint8_t)((uint32_t)Uuid.size()>>8*i)&0xff);
  
  for (int i=0; i<sizeof(uint32_t);i++)
    Header.push_back((uint8_t)((uint32_t)smaz.size()>>8*i)&0xff);

  for (int i=0; i<data3.size();i++)
    Header.push_back(data3[i]);

}

  extern void writeFile(std::string, std::vector<uint8_t>);
void blueprint_repacker::CompressToProto() { 
	std::string message;
	google::protobuf::util::JsonParseOptions options;
  
	options.ignore_unknown_fields = true; 
	auto status = google::protobuf::util::JsonStringToMessage(Vehicle, &sgsdp,options);
	size_t size = sgsdp.ByteSizeLong();
	protobuf.resize(size);
	bool result = sgsdp.SerializeToArray(protobuf.data(),protobuf.capacity());
	//writeFile("dump.proto",protobuf);
	//return (result == true && status.ok()) ? true : false;
}

void blueprint_repacker::CompressToLz4() { 
	LZ4F_compressionContext_t ctx;
  compressResult_t result = { 1, 0, 0 };  /* result for an error */
  LZ4F_compressOptions_t opt = {0,0,0,0};
  opt.stableSrc = true;
  long long count_in = 0, count_out, bytesToOffset = -1;

	size_t const ctxCreation = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
	size_t const outputBufCap	= LZ4F_compressBound(protobuf.size(),&kPrefs);
	lz4Data.resize(outputBufCap);
  


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
		auto lz4Ptr = lz4Data.data()+headerSize;
		auto protoPtr = protobuf.data();

		/* stream buffer */
    for (;;) {
      size_t compressedSize;

      size_t const readSize = protobuf.size()-count_in;
      if (readSize == 0) break; /* nothing left to read from input file */
      count_in += readSize;
//       compressedSize = LZ4F_compressFrame(lz4Ptr, lz4Data.size()-headerSize,
//                                            protobuf.data(), protobuf.size(),
//                                            NULL);
      compressedSize = LZ4F_compressUpdate(ctx,
	                                          lz4Ptr, lz4Data.size()-headerSize,
                                            protobuf.data(), protobuf.size(),
                                            NULL);

      if (LZ4F_isError(compressedSize)) {
        printf("Compression failed: error %u \n", (unsigned)compressedSize);
        return;
      }

//      printf("Writing %u bytes\n", (unsigned)compressedSize);
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
    }
//    printf("Writing %u bytes \n", (unsigned)compressedSize);
    //safe_fwrite(outBuff, 1, compressedSize, f_out);
    count_out += compressedSize;
    

    result.size_in = count_in;
    result.size_out = count_out;
    result.error = 0;
	} else {
    printf("error : resource allocation failed \n");
	}
	LZ4F_freeCompressionContext(ctx);   /* supports free on NULL */


	lz4Data.resize(result.size_out);
}

void blueprint_repacker::CreateUuid() {
  char* uuidStr = (char*)malloc(37); // gotta store it somewhere for a moment
#ifdef _WIN32
  UUID uuid;
  UuidCreate(&uuid);
  UuidToStringA(&uuid, (uint8_t**)&uuidStr);
#else
  uuid_t binuuid;
  uuid_generate_random(binuuid);
  uuid_unparse(binuuid,uuidStr);
#endif
  // skipping the null termination
  // Trailmakers also does this in their image
  for (int i = 0; i<36;i++)
    UuidStr.push_back(uuidStr[i]);
  free(uuidStr);
}

void blueprint_repacker::GenerateUuid() {
  Uuid.push_back(UuidMarker);// UUID marker
  Uuid.push_back(0x02);      // Marker to use reall UUID's
  Uuid.push_back(0x12);      // IDK... Trailmakers also has 0x12 here so im just doing the same
  Uuid.push_back(0x24);      // UUID length of 36 (skipping null end byte)
    for (int i = 0; i<36;i++)
    Uuid.push_back(UuidStr[i]);
}

void blueprint_repacker::GenerateSmaz() {
  uint8_t smazBuffOffset = 0;
  std::vector<char> GiantAssString;
  int GiantAssStringOffset = 0;

  GiantAssString.push_back(TitleMarker);
  GiantAssStringOffset += WriteUleb128(GiantAssString, Title.length());
  for(int i=0; i<Title.length();i++)
    GiantAssString.push_back(Title[i]);

  GiantAssString.push_back(DescriptionMarker);
  GiantAssStringOffset += WriteUleb128(GiantAssString, Description.length());
  for(int i=0; i<Description.length();i++)
    GiantAssString.push_back(Description[i]);

  GiantAssString.push_back(TagMarker);
  GiantAssStringOffset += WriteUleb128(GiantAssString, Tag.length());
  for(int i=0; i<Tag.length();i++)
    GiantAssString.push_back(Tag[i]);

  GiantAssString.push_back(CreatorMarker);
  GiantAssStringOffset += WriteUleb128(GiantAssString, Creator.length());
  for(int i=0; i<Creator.length();i++)
    GiantAssString.push_back(Creator[i]);

  GiantAssString.push_back(SteamTokenMarker);
  GiantAssStringOffset += WriteUleb128(GiantAssString, SteamToken.length());
  for(int i=0; i<SteamToken.length();i++)
    GiantAssString.push_back(SteamToken[i]);

  for (int i = 0;i<steamtokenText.length();i++)
    GiantAssString.push_back(steamtokenText[i]);

  smaz.push_back(0x31);
  cpp_smaz_compress(GiantAssString,smaz);


}

void blueprint_repacker::GenerateBinary() {
  for (int i=0; i<Header.size(); i++)
    binary.push_back(Header[i]);

  for (int i=0; i<lz4Data.size(); i++)
    binary.push_back(lz4Data[i]);

  for (int i=0; i<Uuid.size(); i++)
    binary.push_back(Uuid[i]);

  for (int i=0; i<smaz.size(); i++)
    binary.push_back(smaz[i]);
}

void blueprint_repacker::UseCustomTags() {
  CustomTag = true;
}

void blueprint_repacker::UseCustomToken() {
  CustomSteamToken = true;
}

void blueprint_repacker::setVehicleData(std::string data) {
  Vehicle.assign(data);
}

void blueprint_repacker::setVehicleTitle(std::string title) {
  Title.assign(title);
}

void blueprint_repacker::setVehicleDescription(std::string description) {
  Description.assign(description);
}

void blueprint_repacker::setVehicleTag(std::string tag) {
  bool UsingValidTag = false;
  for (int i = 0; i<Tag.size();i++){
    if (strncmp(tag.data(),Tags[i],strlen(Tags[i]))==0){
      UsingValidTag = true;
    }
  }
  if (CustomTag)
    Tag.assign(tag); // USE AT OWN RISK
  else if (UsingValidTag)
    Tag.assign(tag); // we have a valid tag
  else 
    Tag.assign(Tags[0]); // default is ""
}
void blueprint_repacker::setVehicleCreator(std::string creator) {
  Creator.assign(creator);
}

void blueprint_repacker::setVehicleUuid(std::string uuid) {
  UuidStr.assign(uuid);
}

void blueprint_repacker::setVehicleSteamToken(std::string token) {
  CustomSteamToken ? SteamToken.assign(token) : SteamToken.assign("00000000000000000") ;
}

void blueprint_repacker::pack() { 
	
  // make UUID if we dont get one
  if (UuidStr.empty())
  	CreateUuid();

  // Converting Json to Protbuf
	CompressToProto();

  // Generating the parts for the binary  
	CompressToLz4();
	GenerateUuid();
  GenerateSmaz();

  // Header relies on Vehicle, UUID, SMAZ sizes 
  CreateFakeHeader();	

  // Assembling the binary
  GenerateBinary();
}
