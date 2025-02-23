#include "trailmakers.pb.h"

class blueprint_unpacker
{
private:
  bool enableSteamToken = false;
  bool has_header = false;
  bool HasLz4 = false;
  bool HasProtobuf = false;
  bool hasLz4MagicHeader = false;
  bool leagacy_file = false;
  bool legacy_with_uuid = false;
  uint8_t offset_header = 98;
  uint32_t SaveGameVersion = 0;
  uint32_t legacy_protobuf_size = 0;
  uint8_t TitleMarker = 0x31;
  uint8_t DescriptionMarker = 0x12;
  uint8_t TagMarker = 0x1A;
  uint8_t CreatorMarker = 0x22; // marker in test form (\")
  uint8_t SteamTokenMarker = 0x2A; // marker in test form (or)

  std::vector<uint8_t> Lz4MagicHeader = {0x04, 0x22, 0x4D, 0x18};
  std::vector<uint8_t> binaryData;
  std::vector<uint8_t> VehicleData;
  std::vector<uint8_t> protobufData;
  std::vector<uint8_t> uuidData;
  std::vector<uint8_t> smazData;
  std::string Vehicle;
  std::string UUID;
  std::string Title;
  std::string Description;
  std::string Tag;
  std::string Creator;
  std::string SteamToken;

  uint32_t binarySize = 0;
  uint32_t lz4DataLength = 0;
  uint32_t ProtobufLength = 0;
  uint32_t smazLength = 0;  
  uint32_t VehicleLength = 0;
  uint32_t UUIDLength = 0;
  uint8_t TitleLength = 0;
  size_t DescriptionLength = 0;
  size_t CreatorLength = 0;
  int ImgWidth = 0;
  int ImgHeight = 0;
  int ImgChannels = 0;
  StructureGraphSaveDataProto sgsdp;

  int64_t decompress_data_internal(uint8_t* srcBuffer,size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize,size_t filled, size_t alreadyConsumed, LZ4F_dctx* dctx);

  long fsize(FILE *fp);
  std::vector<uint8_t> readFile(std::string path);
  void extractFromImage(const unsigned char *image_data, int width, int height);

  void extractVehicle();
  int decompress_lz4(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize);
  bool decompress_protobuf();
  int get_index_of(uint8_t* t, size_t cap, int val, uint32_t offset);
  int get_index_of(char* t, size_t cap, int val, uint32_t offset);
  void extractUUID();
  void extractSmaz();
  size_t getSmazLEB(uint8_t* buffptr, uint8_t &consumed, uint16_t offset, uint8_t maxFromOffset);
  size_t getSmazLEB(char* buffptr, uint8_t &consumed, uint16_t offset, uint8_t maxFromOffset);
  void decompress_smaz();

public:
  void parse();
  inline void exportSteamToken() {enableSteamToken = true;};
  inline bool getExportSteamTokenEnabled() const {return enableSteamToken;};
  inline bool isLegacyBlueprint() const {return leagacy_file;}
  inline std::vector<uint8_t> getBinary() const {return binaryData;}
  inline std::vector<uint8_t> getLz4() const {return VehicleData;}
  inline std::vector<uint8_t> getProtobuf() const {return protobufData;}
  inline std::vector<uint8_t> getSmaz() const {return smazData;}
  inline std::string getVehicle() {return Vehicle;}
  inline std::string getTitle() {return Title;}
  inline std::string getDescription() {return Description;}
  inline std::string getTag() {return Tag;}
  inline std::string getCreator() {return Creator;}
  inline std::string getUuid() {return UUID;}
  inline std::string getSteamToken() const {return enableSteamToken ? SteamToken : "-----------------";} // yes... it will be in the code but not in the help window
  
  bool extractFromImage(std::string filepath);
  bool extractFromBinary(std::string filepath);
  bool extractFromlz4(std::string filepath);
  bool extractFromProtobuf(std::string filepath);


  blueprint_unpacker(/* args */bool enableSteamToken = false);
  ~blueprint_unpacker();
};
