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
  uint8_t legacy_with_uuid[3] ={0x08,0x05,0x12}; // UUID from legacy blueprint that got updated
  bool has_header = false;
private:

  std::vector<uint8_t> binary;
  std::vector<uint8_t> lz4Data;
  std::vector<uint8_t> protobuf;
  std::vector<uint8_t> smaz;
  std::string Vehicle;
  std::vector<uint8_t> UUID;
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

public:
  void parse();

  std::vector<uint8_t> getBinary() const {return binary;}
  std::vector<uint8_t> getLz4() const {return lz4Data;}
  std::vector<uint8_t> getProtobuf() const {return protobuf;}
  std::vector<uint8_t> getSmaz() const {return smaz;}
  std::string getVehicle() const {return Vehicle;}
  std::string getTitle() const {return Title;}
  std::string getDescription() const {return Description;}
  std::string getCreator() const {return Creator;}
  std::string getSteamToken() const {return SteamToken;}
  bool isLegacyBlueprint() const {return leagacy_file;}

  bool extractFromImage(std::string filepath);
  bool extractFromBinary(std::string filepath);
  bool extractFromlz4(std::string filepath);
  bool extractFromProtobuf(std::string filepath);

  void extractUUID(bool enable);
  void extractTitle(bool enable);
  void extractDescription(bool enable);
  void extractTag(bool enable);
  void extractCreator(bool enable);
  void extractSteamToken(bool enable); // yes... it will be in the code but not in the help window

  void extract_data_from_image(const unsigned char *image_data, int width, int height);
  
  int decompress_lz4(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize);
  bool decompress_protobuf();
  void decompress_smaz();

  blueprint_unpacker(/* args */);
  ~blueprint_unpacker();
};

