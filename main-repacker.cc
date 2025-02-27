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
#include "blueprint-repacker.hpp"

blueprint_repacker repacker;

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

void print_banner() {
    fprintf(stderr, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║╱╱╱╭━━━╮╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╭━━━━━━╮╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭━╮╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━╮┃┃ ┃╱V0.0╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╭╯ ╰╮╱╱╱┃ ╭━━╮ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━╯╰┫ ┃╭━╮ ╭━┳━━━┳━━━┳━━━┳━┳━━━━┻╮ ╭╯╱╱╱┃ ╰━━╯ ┣━━━┳━━━┳━━━━━┳━━━┫ ┃ ╭┳━━━┳━━╮╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╭━━╮┃ ┃┃ ┃ ┃ ┃ ━━┫╭━╮┃ ╭━╋━┫ ╭━╮ ┫ ┃╱╱╱╱┃ ╭━╮ ╭┫ ━━┫╭━╮┃ ╭━╮ ┃╭━━┫ ╰━╯┫ ━━┫ ╭╯╱╱╱║\n");
    fprintf(stderr, "║╱╱╱┃╰━━╯┃ ╰┫ ╰━╯ ┃ ━━┫╰━╯┃ ┃ ┃ ┃ ┃ ┃ ┃ ╰╮╱╱╱┃ ┃ ╰╮╰┫ ━━┫╰━╯┃ ╭━╮ ┃╰━━┫ ╭━╮┫ ━━┫ ┃╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╰━━━━┻━━┻━━━━━┻━━━┫ ╭━┻━╯ ╰━┻━╯ ╰━┻━━╯╱╱╱╰━╯  ╰━┻━━━┫ ╭━┻━╯ ╰━┻━━━┻━╯ ╰┻━━━┻━╯╱╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱┃ ┃ Made by:  Noah  Clever ╱╱╱║\n");
    fprintf(stderr, "║╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╰━╯╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱╱║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");

}

void print_help() {
    fprintf(stdout, "╔═════════════════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║ This tool takes raw \"blueprint\" data and converts it in to a image file:            ║\n");
    fprintf(stdout, "║ When using a text file make sure its formatted the same as the export tool          ║\n");
    fprintf(stdout, "║ Surround strings with (\") when not using a text file and have spaces in them        ║\n");
    fprintf(stdout, "║ If any of the fields are left empty they will be filled with default values         ║\n");
    fprintf(stdout, "╠═════════════════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stdout, "║ The following options are available                                                 ║\n");
    fprintf(stdout, "║                                                                                     ║\n");
    fprintf(stdout, "║ -h/-H              shows this help screen                                           ║\n");
    fprintf(stdout, "║ -p <path>          path to png input file                                           ║\n");
    fprintf(stdout, "║ -j <path>          path to Json file                                                ║\n");
    fprintf(stdout, "║ -f <path>          path to Text file for Title, Description, Tag, Creator, UUID ... ║\n");
    fprintf(stdout, "║ -t <Title>         Title of the creation (single line)                              ║\n");
    fprintf(stdout, "║ -d <Description>   Description of the creation (single line)                        ║\n");
    fprintf(stdout, "║ -T <Tag>                                                                            ║\n");
    fprintf(stdout, "║ -c <Creator>                                                                        ║\n");
    fprintf(stdout, "║ -u <UUID>                                                                           ║\n");
//    fprintf(stdout, "║ -s <SteamToken>                                                                     ║\n");
    fprintf(stdout, "║ -o <path>                                                                           ║\n");
    fprintf(stdout, "╚═════════════════════════════════════════════════════════════════════════════════════╝\n");
}


int main(int argc, char *argv[]) {
#ifdef _WIN32
  enable_cmd_vt();
  change_charset_utf8();
#endif
  print_banner();
      repacker._TEST();

  int opt;
  std::string inPathPng;
  std::string inPathJson;
  std::string inPathText;
  std::string Vehicle;
  std::string Uuid;
  std::string Title;
  std::string Description;
  std::string Tag;
  std::string Creator;
  std::string SteamToken;

  std::string out_path;

  while ((opt = getopt(argc, argv, "p:j:f:t:d:T:c:u:s:o:hH")) != -1) {
    switch (opt) {
    case 'p':
      fprintf(stdout, "%s\n",optarg);
      inPathPng = optarg;
      break;
    case 'j':
      inPathJson = optarg;
      break;
    case 'f':
      inPathText = optarg;
      break;
    case 't':
      break;
    case 'd':
      break;
    case 'T':
      break;
    case 'c':
      break;
    case 'u':
      break;
    case 's':
      break;
    case 'o':
      break;
    case 'H':
    case 'h':
      print_help();
      return EXIT_SUCCESS;
    }
  }

  if (inPathPng.empty() || out_path.empty()) {
//    print_help();
    return EXIT_SUCCESS;
  }

  return 0;
}


