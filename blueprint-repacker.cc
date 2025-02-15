#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <google/protobuf/util/json_util.h>
#include "lz4.h"
#include "lz4frame.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "stb/stb_image_write.h"
#include "stb/stb_image.h"
#include "smaz.h"
#include "blueprint-repacker.hpp"
using namespace std;
//using namespace google::protobuf;
//using google::protobuf::util::TimeUtil;

blueprint_repacker::blueprint_repacker(/* args */)
{}

blueprint_repacker::~blueprint_repacker() {
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
