#include <iostream>
#include <vector>
#include <string>
#include <lz4.h>
#include <lz4frame.h>
#include <curl/curl.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include "smaz.h"
#include "trailmakers.pb.h"
#include "blueprint-unpacker.hpp"
#include "blueprint-repacker.hpp"
#include <dpp/dpp.h>

std::string BOT_TOKEN = "MTM0Nzk0NjQwODQxNzk1MTg4Ng.GS-zp4.M7vLeb6-gvEVbcH2ydH-ZYThzkViD4IsXgTYhs";

CURL *curl;
CURLcode res;

size_t curl_data_str(char* data, size_t size, size_t nmemb, void *clientp) {
  size_t realsize = size * nmemb;
  std::string *d = (std::string*)clientp;
  for (int i = 0; i<realsize; i++)
    d->push_back(data[i]);
  return realsize;
}

size_t curl_data_int(char* data, size_t size, size_t nmemb, void *clientp) {
  size_t realsize = size * nmemb;
  std::vector<uint8_t> *d = (std::vector<uint8_t>*)clientp;
  for (int i = 0; i<realsize; i++)
    d->push_back(data[i]);
  return realsize;
}

std::vector<uint8_t> makeImage(blueprint_repacker* repacker, std::vector<uint8_t>* Image) {
  int Width = 0;
  int Height = 0;
  int Comp = 0;
  size_t Size = 0;
  std::vector<uint8_t> _NewImage;
  uint8_t* _Image = nullptr;
  _Image = stbi_load_from_memory(Image->data(),Image->capacity(),&Width,&Height,&Comp,0);
  if (Comp <4)
    Size = Width*Height*4;
  else
    Size = Width*Height*Comp;

  // Fill new image with 0
  for (int i=0; i<Size; i++) {
    _NewImage.push_back(0);
  }

  // Copy the image into the canvas
  // Im sure this can be done better.., but
  for (int y = 0; y < Height; y++) {
    for (int x = 0; x < Width; x++) {
      int canvas_index = (y * Width + x) * 4;
      int img_index = (y * Width + x) * Comp;
      _NewImage[canvas_index++] = _Image[img_index++];
      _NewImage[canvas_index++] = _Image[img_index++];
      _NewImage[canvas_index++] = _Image[img_index++];
      _NewImage[canvas_index++] = Comp==3 ? 0xff : _Image[img_index++];
    }
  }

  auto data = repacker->getBlueprintData();
  uint8_t *dataPtr = data.data();
  auto dataSize = data.capacity();

  // Now calculate how many rows to add and round it up
  auto usedRows = std::max(ceil((Width*4)/dataSize),ceil(dataSize/(Width*4)))+1; // Added +1 as a buffer

  // Fill the bottom area (the extra rows) with a background color (RGB = 0) and
  // embed the custom data into the alpha channel in the order: bottom-to-top, left-to-right.
  int data_index = 0;
  for (int y = Height-1; y >= Height-1-usedRows; y--) { // start from the bottom row and move upward
    for (int x = 0; x < Width; x++) { // left to right in each row
      int pixel_index = (y * Width + x) * 4;
      // Set the RGB channels to a default value (here, 0; adjust if needed)
      if (data_index < dataSize) {
        _NewImage[pixel_index + 0] = dataPtr[data_index++]; // Red
        _NewImage[pixel_index + 1] = dataPtr[data_index++]; // Green
        _NewImage[pixel_index + 2] = dataPtr[data_index++]; // Blue
        _NewImage[pixel_index + 3] = 0x00; // Alpha
      }
    }
  }

  for (uint8_t i=0; i<sizeof(uint32_t); i++)
    _NewImage[((Width - 1) * 4) + i] = (repacker->getHeaderSize() >> (24 - i * 8)) & 0xFF;

  int len;
  auto ImgPtr = stbi_write_png_to_mem(_NewImage.data(),Width*4,Width,Height,4,&len);

  return std::vector<uint8_t>(ImgPtr,ImgPtr+len);

}

std::vector<uint8_t> extractBinaryData(uint8_t *img, int* imgWidth, int* imgHeight) {
  auto ImgHeight = *imgHeight;
  auto ImgWidth = *imgWidth;

  std::vector<uint8_t> temp;
  // should be done nicer but works i think
  if (!img)
    return temp;

  size_t offset = 0; // Track how much data has been written
  temp.resize((*imgWidth) * (*imgHeight) * 3);
  uint8_t *output_data = temp.data();

  // Extract binary data
  for (int y = ImgHeight - 1; y >= 0; y--) { // Start from the bottom row and work upwards
    for (int x = 0; x < ImgWidth; x++) { // Start from the left
      int index = (y * ImgWidth + x) * 4; // RGBA has 4 bytes per pixel
      unsigned char alpha = img[index + 3];
      if (alpha == 0) { // Alpha channel fully transparent
        memcpy(&output_data[offset], &img[index], 3); // Copy RGB only
        offset += 3;
      }
    }
  }
  return temp;
}

int main() {
  
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();

  dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_unverified_default_intents );
  bot.on_log(dpp::utility::cout_logger());
  bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
    dpp::command_interaction cmd_data = event.command.get_command_interaction();
    dpp::message msg;
    msg.channel_id = event.command.channel_id;
    std::string ImageName;
    std::vector<uint8_t> Image;
    std::vector<uint8_t> NewImage;
    std::vector<uint8_t> Binary; 

    // Check if the command "blueprint" is used
    if (event.command.get_command_name() == "blueprint") {

      // Get sub command
      dpp::command_data_option subcommand = cmd_data.options[0];

      if (subcommand.name == "info") {
        dpp::embed embed;
        blueprint_unpacker unpacker(true);
        dpp::command_data_option image_file_param = subcommand.options[0];
        dpp::snowflake file_id;
        dpp::attachment att = {0};
        if (!image_file_param.empty()) {
          file_id = std::get<dpp::snowflake>(event.get_parameter("blueprint"));
          att = event.command.get_resolved_attachment(file_id);
          ImageName = att.filename;
          if(ImageName.find(".", 0) == ImageName.npos) {
            ImageName.append(".png");
          }
          curl_easy_setopt(curl, CURLOPT_URL, att.url.c_str());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_data_int);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&Image);
          curl_easy_perform(curl);
        }

        int Width = 0;
        int Height = 0;
        int Comp = 0;
        unpacker.setBinary(
          extractBinaryData(
            stbi_load_from_memory(
              Image.data(),
              Image.capacity(),
              &Width,
              &Height,
              &Comp,
              0),
            &Width,
            &Height
          )
        );
        unpacker.unpack();
        srand(time(0));
        embed.set_colour(rand() % 0xFFFFFF);
        embed.set_author(unpacker.getCreator(),"https://steamcommunity.com/profiles/"+unpacker.getSteamToken(),"");
        embed.set_title(unpacker.getTitle());
        embed.set_url(att.url);
        embed.set_thumbnail(att.url);
        embed.add_field("Description:",unpacker.getDescription());
        embed.add_field("Tag:",unpacker.getTag()=="" ? "Untagged":unpacker.getTag());
        embed.add_field("Creator:",unpacker.getCreator());
        embed.add_field("UUID:",unpacker.getUuid());
        embed.add_field("Steam Token:",unpacker.getSteamToken());

//        embed.set_title();
        //embed.set_author(unpacker.getCreator(), "https://steamcommunity.com/profiles/"+unpacker.getSteamToken(), "https://steamcommunity.com/profiles/"+unpacker.getSteamToken());
        // TODO CREATE INFO CODE
        msg.add_embed(embed);
      } else if (subcommand.name == "unpack") {
        blueprint_unpacker unpacker(true);
        unpacker.EnableJsonOut();
        dpp::command_data_option image_file_param = subcommand.options[0];
        if (!image_file_param.empty()) {
          dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("blueprint"));
          dpp::attachment att = event.command.get_resolved_attachment(file_id);
          ImageName = att.filename;
          if(ImageName.find(".", 0) == ImageName.npos) {
            ImageName.append(".png");
          }
          curl_easy_setopt(curl, CURLOPT_URL, att.url.c_str());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_data_int);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&Image);
          curl_easy_perform(curl);
        }

        int Width = 0;
        int Height = 0;
        int Comp = 0;
        unpacker.setBinary(
          extractBinaryData(
            stbi_load_from_memory(
              Image.data(),
              Image.capacity(),
              &Width,
              &Height,
              &Comp,
              0),
            &Width,
            &Height
          )
        );
        unpacker.unpack();

        msg.add_file(ImageName.replace(ImageName.length()-3,4,"json"), unpacker.getVehicle());
        //msg.content.append("```\n");
        //msg.content.append("Title:       "+ unpacker.getTitle()+"\n");
        //msg.content.append("Description: "+ unpacker.getDescription()+"\n");
        //msg.content.append("Tag:         "+ unpacker.getTag()+"\n");
        //msg.content.append("Creator:     "+ unpacker.getCreator()+"\n");
        //msg.content.append("UUID:        "+ unpacker.getUuid()+"\n");
        //msg.content.append("```\n");
        //msg.content.append("Steam profile: ");
        //msg.content.append("https://steamcommunity.com/profiles/");
        //msg.content.append(unpacker.getSteamToken());
        

      } else if (subcommand.name == "repack"){
        blueprint_repacker repacker;
        std::string Json;
        int cmd_count= 0;
        dpp::command_data_option image_file_param  = subcommand.options.capacity() > cmd_count ? subcommand.options[ cmd_count++ ] : dpp::command_data_option();
        dpp::command_data_option json_file_param   = subcommand.options.capacity() > cmd_count ? subcommand.options[ cmd_count++ ] : dpp::command_data_option();
        dpp::command_data_option Title_param       = subcommand.options.capacity() > cmd_count ? subcommand.options[ cmd_count++ ] : dpp::command_data_option();
        dpp::command_data_option Description_param = subcommand.options.capacity() > cmd_count ? subcommand.options[ cmd_count++ ] : dpp::command_data_option();
        dpp::command_data_option Tag_param         = subcommand.options.capacity() > cmd_count ? subcommand.options[ cmd_count++ ] : dpp::command_data_option();
        dpp::command_data_option Uuid_param        = subcommand.options.capacity() > cmd_count ? subcommand.options[ cmd_count++ ] : dpp::command_data_option();
        dpp::command_data_option Creator_param     = subcommand.options.capacity() > cmd_count ? subcommand.options[ cmd_count++ ] : dpp::command_data_option();
        dpp::command_data_option SteamToken_param  = subcommand.options.capacity() > cmd_count ? subcommand.options[ cmd_count++ ] : dpp::command_data_option();
        
        if (!image_file_param.empty()) {
          dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("image"));
          dpp::attachment att = event.command.get_resolved_attachment(file_id);
          ImageName = att.filename;
          if(ImageName.find(".", 0) == ImageName.npos) {
            ImageName.append(".png");
          }
          curl_easy_setopt(curl, CURLOPT_URL, att.url.c_str());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_data_int);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&Image);
          curl_easy_perform(curl);
        }

        if (!json_file_param.empty()) {
          dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("json"));
          dpp::attachment att = event.command.get_resolved_attachment(file_id);

          curl_easy_setopt(curl, CURLOPT_URL, att.url.c_str());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_data_str);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&Json);
          curl_easy_perform(curl);
        }
        repacker.setVehicleData(Json);
          
        if (!Title_param.empty()) 
          repacker.setVehicleTitle(std::get<std::string>(Title_param.value));
        if (!Description_param.empty())
          repacker.setVehicleDescription(std::get<std::string>(Description_param.value));
        if (!Tag_param.empty())
          repacker.setVehicleTag(std::get<std::string>(Tag_param.value));
        if (!Uuid_param.empty())
          repacker.setVehicleUuid(std::get<std::string>(Uuid_param.value));
        if (!Creator_param.empty())
          repacker.setVehicleCreator(std::get<std::string>(Creator_param.value));
        if (!SteamToken_param.empty())
          repacker.setVehicleSteamToken(std::get<std::string>(SteamToken_param.value));
        repacker.pack();

        // Assemble image and blueprint data
        NewImage = makeImage(&repacker, &Image);
        msg.add_file(ImageName,std::string(NewImage.begin(),NewImage.end()));

      } else if (subcommand.name == "reimage") {
        blueprint_unpacker unpacker(true);
        blueprint_repacker repacker;
        unpacker.EnableJsonOut();
        dpp::command_data_option image1_file_param  = subcommand.options[0];
        dpp::command_data_option image2_file_param  = subcommand.options[1];
        
        if (!image1_file_param.empty()) {
          dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("blueprint-old"));
          dpp::attachment att = event.command.get_resolved_attachment(file_id);
          curl_easy_setopt(curl, CURLOPT_URL, att.url.c_str());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_data_int);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&Image);
          curl_easy_perform(curl);
        }
        if (!image2_file_param.empty()) {
          dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("blueprint-new"));
          dpp::attachment att = event.command.get_resolved_attachment(file_id);
          ImageName = att.filename;
          if(ImageName.find(".", 0) == ImageName.npos) {
            ImageName.append(".png");
          }
          curl_easy_setopt(curl, CURLOPT_URL, att.url.c_str());
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,curl_data_int);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&NewImage);
          curl_easy_perform(curl);
        }

        uint8_t* img = nullptr;
        int Width = 0;
        int Height = 0;
        int Comp = 0;
        Binary = extractBinaryData(
          stbi_load_from_memory(
            Image.data(),
            Image.capacity(),
            &Width,
            &Height,
            &Comp,
            0),
          &Width,
          &Height
        );
        unpacker.setBinary(Binary);
        unpacker.unpack();
        repacker.setVehicleData(unpacker.getVehicle());
        repacker.setVehicleTitle(unpacker.getTitle());
        repacker.setVehicleDescription(unpacker.getDescription());
        repacker.setVehicleTag(unpacker.getTag());
        repacker.setVehicleCreator(unpacker.getCreator());
        repacker.setVehicleUuid(unpacker.getUuid());
        repacker.setVehicleSteamToken(unpacker.getSteamToken());
        repacker.pack();

        // Assemble image and blueprint data
        std::vector<uint8_t> _NewImage = makeImage(&repacker, &NewImage);
        msg.add_file(ImageName,std::string(_NewImage.begin(),_NewImage.end()));

      } else if (subcommand.name == "ping"){
        msg.content = event.command.member.get_nickname()+" pong!";
      }
      event.reply(msg);

    }
  });

  bot.on_ready([&](const dpp::ready_t& event) {
    if (dpp::run_once<struct register_bot_commands>()) {

	    dpp::slashcommand blueprint("blueprint","Send a TrailMakers Blueprint",bot.me.id);

      // setup sub command for showing blueprint info
      blueprint.add_option(
        dpp::command_option(dpp::co_sub_command, "info", "Show information about a TrailMakers blueprint")
        .add_option(dpp::command_option(dpp::co_attachment, "blueprint","TrailMakers blueprint",true))
      );

      // setup sub command for unpacking bluepint
      blueprint.add_option(
        dpp::command_option(dpp::co_sub_command, "unpack", "Get data from a TrailMakers blueprint")
        .add_option(dpp::command_option(dpp::co_attachment, "blueprint","TrailMakers blueprint",true))
      );

      // setup sub command for repacking blueprint
      blueprint.add_option(
        dpp::command_option(dpp::co_sub_command, "repack", "Generate a TrailMakers blueprint")
        .add_option(dpp::command_option(dpp::co_attachment, "image","PNG file to embed the blueprint in",true))
        .add_option(dpp::command_option(dpp::co_attachment, "json","JSON file to embed the blueprint in",true))
        .add_option(dpp::command_option(dpp::co_string, "title","Blueprint Title", false))
          .set_max_length(80) // technically it can be biggier but then it breaks the in game UI
        .add_option(dpp::command_option(dpp::co_string, "description","Blueprint description",false))
        .add_option(dpp::command_option(dpp::co_string, "tag","Vehicle tag",false)
          .add_choice(dpp::command_option_choice("Untagged"        ,""               ))
          .add_choice(dpp::command_option_choice("Airplane"        ,"Airplane"        ))
          .add_choice(dpp::command_option_choice("Airship"         ,"Airship"         ))
          .add_choice(dpp::command_option_choice("Animal"          ,"Animal"          ))
          .add_choice(dpp::command_option_choice("Boat"            ,"Boat"            ))
          .add_choice(dpp::command_option_choice("Car"             ,"Car"             ))
          .add_choice(dpp::command_option_choice("Combat"          ,"Combat"          ))
          .add_choice(dpp::command_option_choice("Fan Art"         ,"Fan Art"         ))
          .add_choice(dpp::command_option_choice("Helicopter"      ,"Helicopter"      ))
          .add_choice(dpp::command_option_choice("Hovercraft"      ,"Hovercraft"      ))
          .add_choice(dpp::command_option_choice("Immobile"        ,"Immobile"        ))
          .add_choice(dpp::command_option_choice("Motorcycle"      ,"Motorcycle"      ))
          .add_choice(dpp::command_option_choice("Space"           ,"Space"           ))
          .add_choice(dpp::command_option_choice("Submarine"       ,"Submarine"       ))
          .add_choice(dpp::command_option_choice("Transformer"     ,"Transformer"     ))
          .add_choice(dpp::command_option_choice("Walker"          ,"Walker"          ))
          .add_choice(dpp::command_option_choice("Weekly Challenge","Weekly Challenge"))
        )
        .add_option(dpp::command_option(dpp::co_string, "uuid","Transformation UUID",false))
        .add_option(dpp::command_option(dpp::co_string, "creator","Blueprint creator",false))
        .add_option(dpp::command_option(dpp::co_string, "steamtoken","Blueprint steamtoken",false))
          .set_max_length(18) // I dont know if it can be larger or not...

      );

      // setup sub command for reimaging blueprint
      blueprint.add_option(
        dpp::command_option(dpp::co_sub_command, "reimage", "Change TrailMakers blueprint image")
        .add_option(dpp::command_option(dpp::co_attachment, "blueprint-old","TrailMakers blueprint",true))
        .add_option(dpp::command_option(dpp::co_attachment, "blueprint-new","new TrailMakers blueprint",true))
      );      // setup sub command for reimaging blueprint
      blueprint.add_option(dpp::command_option(dpp::co_sub_command,"ping","check bot connection"));

      blueprint.set_interaction_contexts({dpp::itc_guild});
      bot.global_bulk_command_create({ blueprint });
    }
  });

  bot.start(dpp::st_wait);
  return 0;
}