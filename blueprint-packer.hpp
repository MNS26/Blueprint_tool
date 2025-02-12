#include "trailmakers.pb.h"
using namespace std;

class blueprint_unpacker
{
public:
  uint8_t offset_header = 98;
  uint8_t CreatorNameSize = 0;
  uint32_t SaveGameVersion = 0;
  bool leagacy_file = false;
  uint32_t legacy_protobuf_size = 0;
  uint32_t lz4_size = 0;
  uint32_t uuid_size = 0;
  uint32_t smaz_size = 0;
  uint32_t uuid_id_size = 0;
  bool legacy_with_uuid = false;
  bool has_header = false;

  uint8_t TitleMarker = 0x31;
  uint8_t DescriptionMarker = 0x12;
  uint8_t TagMarker = 0x1A;
  uint8_t CreatorMarker = 0x22; // marker in test form (\")
  uint8_t SteamTokenMarker = 0x2A; // marker in test form (or)

private:

  std::vector<uint8_t> binary;
  std::vector<uint8_t> lz4Data;
  std::vector<uint8_t> protobuf;
  std::vector<uint8_t> smaz;
  std::string Vehicle;
  std::string UUID;
  std::string Title;
  std::string Description;
  std::string Tag;
  std::string Creator;
  std::string SteamToken;

  size_t binarySize = 0;
  size_t lz4DataLength = 0;
  size_t ProtobufLength = 0;
  size_t smazLength = 0;  
  size_t VehicleLength = 0;
  size_t UUIDLength = 0;
  uint8_t TitleLength = 0;
  size_t DescriptionLength = 0;
  size_t CreatorLength = 0;
  int ImgWidth = 0;
  int ImgHeight = 0;
  int ImgChannels = 0;
  StructureGraphSaveDataProto sgsdp;

  int64_t decompress_data_internal(uint8_t* srcBuffer,size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize,size_t filled, size_t alreadyConsumed, LZ4F_dctx* dctx);
  size_t decompress_file_allocDst(uint8_t *srcBuffer, size_t srcBufferSize,uint8_t *dstBuffer, size_t dstBufferSize, LZ4F_dctx *dctx);

  long fsize(FILE *fp);
  void writeFile(string path, string contents);
  void writeFile(string path, std::vector<uint8_t> contents);
  std::vector<uint8_t> readFile(string path);
  void extract_data_from_image(const unsigned char *image_data, int width, int height);

public:
  void parse();

  auto getBinary() const {return binary;}
  auto getLz4() const {return lz4Data;}
  auto getProtobuf() const {return protobuf;}
  auto getSmaz() const {return smaz;}
  auto getVehicle() const {return Vehicle;}
  auto getTitle() const {return Title;}
  auto getDescription() const {return Description;}
  auto getTag() const {return Tag;}
  auto getCreator() const {return Creator;}
  auto getSteamToken() const {return SteamToken;} // yes... it will be in the code but not in the help window
  bool isLegacyBlueprint() const {return leagacy_file;}

  bool extractFromImage(std::string filepath);
  bool extractFromBinary(std::string filepath);
  bool extractFromlz4(std::string filepath);
  bool extractFromProtobuf(std::string filepath);

  void extractLz4();
  int decompress_lz4(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize);
  void extractProtobuf();
  bool decompress_protobuf();
  int get_index_of(uint8_t *t, size_t cap, int val, uint offset);
  int get_index_of(char *t, size_t cap, int val, uint offset);
  void extractJson();
  void extractUUID();
  void extractSmaz();
  size_t getSmazLEB(uint8_t* buffptr, uint8_t &consumed, uint16_t offset, uint8_t maxFromOffset);
  size_t getSmazLEB(char* buffptr, uint8_t &consumed, uint16_t offset, uint8_t maxFromOffset);
  void decompress_smaz();

  blueprint_unpacker(/* args */);
  ~blueprint_unpacker();
};
