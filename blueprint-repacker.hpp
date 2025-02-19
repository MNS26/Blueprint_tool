#include "trailmakers.pb.h"
using namespace std;

class blueprint_repacker
{
public:
  uint8_t offset_header = 98;
  uint8_t CreatorNameSize = 0;
  uint32_t SaveGameVersion = 0;
  bool leagacy_file = false;
  uint32_t legacy_protobuf_size = 0;
  uint32_t lz4_size = 0;
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
  int ImgChannels = 4;
  StructureGraphSaveDataProto sgsdp;
  bool CreateFakeHeader();
  bool CompressToProto();
  bool CompressToLz4();
  bool GenerateUuid();
  bool GenerateSmaz();
  std::vector<uint8_t> readFile(string path);


public:
  bool GetImage(std::string filepath);
  bool GetJson(std::string filepath);
  bool GetTitleDescriptionTagCreatorToken(std::string filepath); // does the same as all the other calls but atomated from a text file
  bool GetTitle(std::string text);
  bool GetDescription(std::string text);
  bool GetTag(std::string text);
  bool GetCreator(std::string text);
  bool GetToken(std::string text);

  blueprint_repacker(/* args */);
  ~blueprint_repacker();
};
