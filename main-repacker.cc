// needed data
//
// JSON
// UUID
// SMAZ
#ifdef _WIN32
#  define _WIN32_WINNT 0x0A00
    #define WIN32_LEAN_AND_MEAN
    // Minimal Windows.h
    #include <sdkddkver.h>
    #include <windef.h>
    #include <winbase.h>
    #include <winuser.h>
    // WinCon needs GDI, which uses Escape. Satisfy deps
    #define NOGDI
    #if !defined(_MAC) || defined(_WIN32NLS)
    #include <winnls.h>
    #endif
    #ifndef _MAC
    #include <wincon.h>
    #include <winver.h>
    #endif

    #include <stdio.h>
    #include <stdint.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#endif
#define MAX_LZ4_DECOMPRESSED_SIZXE (1024 * 1024 * 4)
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "google/protobuf/util/time_util.h"
#include "google/protobuf/util/json_util.h"
#include "trailmakers.pb.h"
#include "blueprint-repacker.hpp"
using namespace std;
//using namespace google::protobuf;
//using google::protobuf::util::TimeUtil;
StructureGraphSaveDataProto sgsdp;


long fsize(FILE *fp){
    long prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    long sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

#ifdef _WIN32
void enable_cmd_vt() {
  HANDLE h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(h_stdout, &mode);
  mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(h_stdout, mode);
}
void change_charset_utf8() {
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);
}
#endif

//#ifndef __WIN32
void print_banner() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║╱╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━━━━━━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━╮┃┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ╭━━╮ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━╋╮ ╭╯╱╱╱┃ ╰━━╯ ┣━━━┳━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃ ━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┫ ┃╱╱╱╱┃ ╭━╮ ╭┫ ━━┫╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┫ ━━┫ ╭╯╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃ ━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ┃ ╰╮╰┫ ━━┫╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┫ ━━┫ ┃╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━╯  ╰━┻━━━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱Made by: Noah (MS26)╱╱╱║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");

}

void print_help() {
    fprintf(stdout, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║ This tool takes \"blueprint\" data and converts it in to a image file:                ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "╠═════════════════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stdout, "║ The following options are available                                                 ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "║ -                                                                                   ║\n");
    fprintf(stdout, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}
//#else
//void print_banner() {
//    fprintf(stdout, "Blueprint Repacker \n");
//    fprintf(stdout, "Made by: Noah (MS26) \n");
//    fprintf(stdout, "\n");
//}
//void print_help() {
//
//    fprintf(stdout, "This tool takes \"blueprint\" data and converts it in to a image file:\n");
//    fprintf(stdout, " \n");
//    fprintf(stdout, " \n");
//    fprintf(stdout, " \n");
//    fprintf(stdout, " \n");
//    fprintf(stdout, " \n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//    fprintf(stdout, "-\n");
//  }
//#endif
//


int main(int argc, char *argv[]) {
#ifdef _WIN32
  enable_cmd_vt();
  change_charset_utf8();
#endif
  print_banner();

  int opt;
  string in_path;
  string in_type;

  string out_path;
  string out_type;

  while ((opt = getopt(argc, argv, "b:l:j:P:i:I:o:u:U:d:D:hH")) != -1) {
    switch (opt) {
    case 'b':
      in_path = optarg;
      in_type = "binary";
      break;
    case 'l':
      in_path = optarg;
      in_type = "lz4";
      break;
    case 'j':
      in_path = optarg;
      in_type = "json";
      break;
    case 'P':
      in_path = optarg;
      in_type = "protobuf";
      break;
    case 'i':
      in_path = optarg;
      break;
    case 'I':
      in_type = optarg;
      break;
    case 'o':
      out_path = optarg;
      break;
    case 'u': // UUID string
      break;
    case 'U': // path to a UUID text file
      break;
    case 'd': // description string
      break;
    case 'D': // path to a description text file
      break;
    case 'H':
    case 'h':
      print_help();
      return EXIT_SUCCESS;
    }
  }

  if (in_path.empty() || out_path.empty()) {
    print_help();
    return EXIT_SUCCESS;
  }

  return 0;
}


