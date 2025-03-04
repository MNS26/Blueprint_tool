#include "trailmakers.pb.h"
#include <lz4.h>
#include <lz4frame.h>
#include <lz4frame_static.h>

class blueprint_repacker
{
private:

  typedef struct {
    int error;
    unsigned long long size_in;
    unsigned long long size_out;
  } compressResult_t;

  LZ4F_preferences_t kPrefs = {
    LZ4F_default,
    LZ4F_blockLinked,
    LZ4F_contentChecksumEnabled,
    LZ4F_frame,
    0 /* unknown content size */,
    0 /* no dictID */ ,
    LZ4F_noBlockChecksum ,
    0,   /* compression level; 0 == default */
    0,   /* autoflush */
    0,   /* favor decompression speed */
    { 0, 0, 0 },  /* reserved, must be set to 0 */
  };

  StructureGraphSaveDataProto sgsdp;

  bool CustomTag = false;
  bool CustomSteamToken = false;
  uint8_t UuidMarker= 0x08;

  uint8_t TitleMarker = 0x31;
  uint8_t DescriptionMarker = 0x12;
  uint8_t TagMarker = 0x1A;
  uint8_t CreatorMarker = 0x22; // marker in test form (\")
  uint8_t SteamTokenMarker = 0x2A; // marker in test form (or)
  std::vector<const char*> Tags = {
    "", //Untagged
    "Airplane",
    "Airship",
    "Animal",
    "Boat",
    "Car",
    "Combat",
    "Fan Art",
    "Helicopter",
    "Hovercraft",
    "Immobile",
    "Motorcycle",
    "Space",
    "Submarine",
    "Transformer",
    "Walker",
    "Weekly Challenge"
  };
  /**
   * Header bullshit
   */
  std::array<uint8_t, 116> data1 = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x46, 0x41, 0x73, 0x73, 0x65, 0x6D, 0x62, 0x6C, 0x79, 0x2D, 
    0x43, 0x53, 0x68, 0x61, 0x72, 0x70, 0x2C, 0x20, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 
    0x30, 0x2E, 0x30, 0x2E, 0x30, 0x2E, 0x30, 0x2C, 0x20, 0x43, 0x75, 0x6C, 0x74, 0x75, 0x72, 0x65, 
    0x3D, 0x6E, 0x65, 0x75, 0x74, 0x72, 0x61, 0x6C, 0x2C, 0x20, 0x50, 0x75, 0x62, 0x6C, 0x69, 0x63, 
    0x4B, 0x65, 0x79, 0x54, 0x6F, 0x6B, 0x65, 0x6E, 0x3D, 0x6E, 0x75, 0x6C, 0x6C, 0x05, 0x01, 0x00, 
    0x00, 0x00, 0x11, 0x53, 0x61, 0x76, 0x65, 0x47, 0x61, 0x6D, 0x65, 0x49, 0x6E, 0x50, 0x4E, 0x47, 
    0x49, 0x6E, 0x66, 0x6F, 
  };
  uint32_t SaveGameVersion = 5;
  std::array<uint8_t, 115> data2 = {
    0x07, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x07, 0x63, 0x72, 0x65, 0x61, 0x74, 0x6F, 0x72, 
    0x11, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x75, 0x72, 0x65, 0x42, 0x79, 0x74, 0x65, 0x53, 0x69, 
    0x7A, 0x65, 0x17, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x75, 0x72, 0x65, 0x49, 0x64, 0x65, 0x6E, 
    0x74, 0x69, 0x66, 0x69, 0x65, 0x72, 0x53, 0x69, 0x7A, 0x65, 0x15, 0x73, 0x74, 0x72, 0x75, 0x63, 
    0x74, 0x75, 0x72, 0x65, 0x4D, 0x65, 0x74, 0x61, 0x42, 0x79, 0x74, 0x65, 0x53, 0x69, 0x7A, 0x65, 
    0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x08, 0x08, 0x08, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
    0x00, 0x06, 0x03, 0x00, 0x00, 0x00, 0x0C, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x20, 0x20, 
  };
  // idk why they have this... but its there
  std::string steamtokenText = "2\nsteamtoken";

  std::array<uint8_t, 2> data3 = {
    0x0B, 0x00, 
  };

  std::vector<uint8_t> Header;
  std::vector<uint8_t> protobuf;
  std::vector<uint8_t> lz4Data;
  std::vector<uint8_t> Uuid;
  std::vector<uint8_t> smaz;
  std::vector<uint8_t> binary;

  std::string Vehicle;
  std::string Title;
  std::string Description;
  std::string UuidStr;
  std::string Tag;
  std::string Creator;
  std::string SteamToken;
  std::string VehicleText;
  


  std::size_t WriteUleb128(std::vector<char>& dest, unsigned long val);

  void CreateFakeHeader();
  void CompressToProto();
  void CompressToLz4();
  void CreateUuid();
  void GenerateSmaz();
  void GenerateUuid();
  void GenerateBinary();

public:

  void UseCustomTags();
  void UseCustomToken();
  void setVehicleData(std::string data);
  void setVehicleTitle(std::string title);
  void setVehicleDescription(std::string description);
  void setVehicleTag(std::string tag);
  void setVehicleCreator(std::string creator);
  void setVehicleUuid(std::string uuid);
  void setVehicleSteamToken(std::string token);
  void GenerateBlueprintData();
  inline std::vector<uint8_t> getBlueprintData() const {return binary;};
  inline uint32_t getHeaderSize() const {return Header.size()-1;}
  void _TEST() {
    blueprint_repacker::setVehicleData("{{ 'StructureNodes': [  {   'SocketPosition': {},   'SocketRotation': {    'EulerY': 1   },   'parentSocket': -1,   'BlockSetup': {    'primaryColor': {     'm_commonColorIndexOffByOne': 17    },    'secondaryColor': {     'm_commonColorIndexOffByOne': 2    },    'SkinType': 200   },   'BlockMetadataGuid': {    'low': '4663452642151314132',    'high': '11595786232070705820'   },   'CloneNodeID': -1  } ], 'Bounds': {  'center': {   'x': 1.375,   'y': -0.5,   'z': 1.125  },  'size': {   'x': 3,   'y': 1.25,   'z': 2.5  } }, 'LowestPoint': -1.125, 'MidPointOffsetFromWeldGroupSpace': {  'x': 1.375,  'y': -0.5,  'z': 1.125 }, 'SaveDataVersion': 13, 'GameVersion': {  'GameVersion_Major': 1,  'GameVersion_Mid': 6,  'GameVersion_Minor': 2,  'GameVersion_ChangeSet': 46142 }, 'WeldingData': {  'm_weldgroupsDict': [   {    'Value': [     0    ]   }  ],  'WeldSettingsHash': 7143424 }}}");
    blueprint_repacker::setVehicleUuid("9bbb86ac-75a9-47c9-848b-618cc2f7a598");
    blueprint_repacker::setVehicleTitle("TEST");
    blueprint_repacker::setVehicleDescription("test ﷽");
    blueprint_repacker::setVehicleCreator("Noah");
    blueprint_repacker::setVehicleTag("DeezNuts");
    blueprint_repacker::setVehicleSteamToken("76561198202006434");
    blueprint_repacker::GenerateBlueprintData();
  };

  blueprint_repacker(/* args */bool enableCustomSteamToken = false);
  ~blueprint_repacker();
};
