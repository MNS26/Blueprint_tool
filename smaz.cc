#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <cstdint>
#include <ctime>
#include <ostream>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "smaz.h"


/* compression codebook, used for decompression/compression  */
// Indexes to check: 61, 63, 64, 90
static const char *Smaz_rcb[254] = {
/*0  */ " "    ,"the" ,"e"  ,"t"    ,"a"  ,"of"  ,"o"   ,"and"    ,"i"     ,"n"  ,
/*10 */ "s"    ,"e "  ,"r"  ," th"  ," t" ,"in"  ,"he"  ,"th"     ,"h"     ,"he ",
/*20 */ "to"   ,"\r\n","l"  ,"s "   ,"d"  ," a"  ,"an"  ,"er"     ,"c"     ," o" ,
/*30 */ "d "   ,"on"  ," of","re"   ,"of ","t "  ,", "  ,"is"     ,"u"     ,"at" ,
/*40 */ "   "  ,"n "  ,"or" ,"which","f"  ,"m"   ,"as"  ,"it"     ,"that"  ,"\n" ,
/*50 */ "was"  ,"en"  ,"  " ," w"   ,"es" ," an" ," i"  ,"\r"     ,"f "    ,"g"  ,
/*60 */ "p"    ,"nd"  ," s" ,"nd "  ,"ed ","w"   ,"ed"   ,"http://","for"   ,"te" ,
/*70 */ "ing"  ,"y "  ,"The"," c"   ,"ti" ,"r "  ,"his" ,"st"     ," in"   ,"ar" ,
/*80 */ "nt"   ,","   ," to","y"    ,"ng" ," h"  ,"with","le"     ,"al"    ,"to ",
/*90 */ "b"    ,"ou"  ,"be" ,"were" ," b" ,"se"  ,"o "  ,"ent"    ,"ha"    ,"ng ",
/*100*/ "their","\""  ,"hi" ,"from" ," f" ,"in " ,"de"  ,"ion"    ,"me"    ,"v"  ,
/*110*/ "."    ,"ve"  ,"all","re "  ,"ri" ,"ro"  ,"is " ,"co"     ,"f t"   ,"are",
/*120*/ "ea"   ,". "  ,"her"," m"   ,"er "," p"  ,"es " ,"by"     ,"they"  ,"di" ,
/*130*/ "ra"   ,"ic"  ,"not","s, "  ,"d t","at " ,"ce"  ,"la"     ,"h "    ,"ne" ,
/*140*/ "as "  ,"tio" ,"on ","n t"  ,"io" ,"we"  ," a " ,"om"     ,", a"   ,"s o",
/*150*/ "ur"   ,"li"  ,"ll" ,"ch"   ,"had","this", "e t","g "     ,"e\r\n" ," wh",
/*160*/ "ere"  ," co" ,"e o","a "   ,"us" ," d"  ,"ss"  ,"\n\r\n" ,"\r\n\r","=\"",
/*170*/ " be"  ," e"  ,"s a","ma"   ,"one","t t" ,"or " ,"but"    ,"el"    ,"so" ,
/*180*/ "l "   ,"e s" ,"s," ,"no"   ,"ter"," wa" ,"iv"  ,"ho"     ,"e a"   ," r" , 
/*190*/ "hat"  ,"s t" ,"ns" ,"ch "  ,"wh" ,"tr"  ,"ut"  ,"/"      ,"have"  ,"ly ",
/*200*/ "ta"   ," ha" ," on","tha"  ,"-"  ," l"  ,"ati" ,"en "    ,"pe"    ," re",
/*210*/ "there","ass" ,"si" ," fo"  ,"wa" ,"ec"  ,"our" ,"who"    ,"its"   ,"z"  ,
/*220*/ "fo"   ,"rs"  ,">"  ,"ot"   ,"un" ,"<"   ,"im"  ,"th "    ,"nc"    ,"ate",
/*230*/  "><"  ,"ver" ,"ad" ," we"  ,"ly" ,"ee"  ," n"  ,"id"     ," cl"   ,"ac" ,
/*240*/ "il"   ,"</"  ,"rt" ," wi"  ,"div","e, " ," it" ,"whi"    ," ma"   ,"ge" ,
/*250*/ "x"    ,"e c" ,"men",".com"
};
// 0-9 A-Z jqk 
// are not in the decoder

void cpp_smaz_compress(std::vector<char>& in, std::vector<uint8_t>& out) {
  size_t in_idx = 1;
  auto inlen = in.size();

  while (in_idx <inlen) {
    bool match_found = false;
    size_t best_match_index =0; // Holds what index has largest match
    size_t best_match_len = 0; // Holds what the longest match is so far

    // Try matching Smaz_rcb entries
    for (int i=0; i<254; i++) {
      const char* pattern = Smaz_rcb[i];
      size_t pat_len = strlen(pattern);

      // Check if pattern matches
      if (in_idx+pat_len <= inlen && strncmp(&in[in_idx], pattern, pat_len)==0) {
        // Match found
        if (pat_len > best_match_len) {
          best_match_len = pat_len;
          best_match_index = i;
        }
      }
    }

    if (best_match_len) {
        // Write index for matched pattern
        out.push_back((uint8_t)best_match_index);
        in_idx += best_match_len; // advance by length
    } else {
      // No matches found, switching to literal
      size_t literal_start = in_idx;
      ssize_t literal_len = 0;

      // Continue until we reach the end or find a match
      while (in_idx < inlen) {
        bool can_match = false;

        for (int j=0; j<254; j++) {
          const char* pattern = Smaz_rcb[j];
          size_t pat_len = strlen(pattern);
          if (in_idx+pat_len <= inlen && strncmp(&in[in_idx], pattern, pat_len)==0) {
            can_match = true;
            break;
          }
        }
        if (can_match)
          break;
        // Otherwise add current character to literal
        literal_len++;
        in_idx++;
      }

      //Flush the literal, and segment over 255
      if (literal_len == 1) {
        // Single byte copy
        out.push_back(254);
        out.push_back(in[literal_start]);
      } else {
        // Multi byte copy
        // Split up when more than 255
        size_t pos = 0;
        while (literal_len > 0) {
          // Clamp to 255
          uint8_t chunk = (literal_len > 255 ? 255 : literal_len);
          out.push_back(255);
          out.push_back(chunk);
          for(;chunk;chunk--) {
            out.push_back(in[literal_start+pos]);
            pos++;
            literal_len--;
          }
//          memcpy(out+out_idx, in+literal_start+pos, chunk);
//          out_idx+= chunk;
//          pos += chunk;
//          literal_len -= chunk;
        }
      }
    }
  }
}

int smaz_compress(char* in, int inlen, char* out, int outlen) {
  size_t in_idx = 0;
  size_t out_idx = 0;

  while (in_idx <inlen) {
    bool match_found = false;
    size_t best_match_index =0; // Holds what index has largest match
    size_t best_match_len = 0; // Holds what the longest match is so far
    // Try matching Smaz_rcb entries
    for (int i=0; i<254; i++) {
      const char* pattern = Smaz_rcb[i];
      size_t pat_len = strlen(pattern);

      // Check if pattern matches
      if (in_idx+pat_len <= inlen && strncmp(in+in_idx, pattern, pat_len)==0) {
        // Match found
        if (pat_len > best_match_len) {
          best_match_len = pat_len;
          best_match_index = i;
        }
        // We found 'a' match
//        match_found = true;
      }
    }
    if (best_match_len) {
        // Write index for matched pattern
        out[out_idx++] = (uint8_t)best_match_index;
        in_idx += best_match_len; // advance by length
    } else {
      // No matches found, switching to literal
      size_t literal_start = in_idx;
      ssize_t literal_len = 0;

      // Continue until we reach the end or find a match
      while (in_idx < inlen) {
        bool can_match = false;
        for (int j=0; j<254; j++) {
          const char* pattern = Smaz_rcb[j];
          size_t pat_len = strlen(pattern);
          if (in_idx+pat_len <= inlen && strncmp(in+in_idx, pattern, pat_len)==0) {
            can_match = true;
            break;
          }
        }
        if (can_match)
          break;
        // Otherwise add current character to literal
        literal_len++;
        in_idx++;
      }

      //Flush the literal, and segment over 255
      if (literal_len == 1) {
        // Single byte copy
        out[out_idx++] = 254;
        out[out_idx++] = in[literal_start];
      } else {
        // Multi byte copy
        // Split up when more than 255
        size_t pos = 0;
        while (literal_len > 0) {
          // Clamp to 255
          uint8_t chunk = (literal_len > 255 ? 255 : literal_len);
          out[out_idx++] = 255;
          out[out_idx++] = chunk;
          memcpy(out+out_idx, in+literal_start+pos, chunk);
          out_idx+= chunk;
          pos += chunk;
          literal_len -= chunk;
        }
      }
    }
  }
  return out_idx;
}


void cpp_smaz_decompress(std::vector<uint8_t>& in, std::vector<char>& out) {
  // skipping 1st byte
  uint8_t consumed = 1;

  // Offset of -3:
  // 1 for skipping 1st byte
  // 1 for .size returning count instead of sectors used
  // 1 for some reason since its off by one otherwise... who knows why but it works
  uint8_t inlen = in.size()-3;

  while(inlen) {
    //clamp so it doesnt go below 0 or above input size
//    inlen=inlen>0?inlen<=in.size()?inlen:in.size():0;
    int len =0;
    switch ((uint8_t)in[consumed])
    {
    case 255:
      consumed++;
      /* Verbatim string */
      len = in[consumed++];
//      inlen -= 2;
      for (;len;len--) {
        out.push_back(in[consumed++]);
        inlen--;
      } 
      break;
    case 254:
      consumed++;
      /* Verbatim byte */
      out.push_back(in[consumed++]);
      inlen -= 2;
      break;
    
    default:
      const char *s = Smaz_rcb[in[consumed++]];
      len = strlen(s);
      for (int i = 0;len;len--&&i++) {
        out.push_back(s[i]);
        inlen--;
      }
      break;
    }
  }
}

int smaz_decompress(char *in, int inlen, char *out, int outlen) {
  unsigned char *c = (unsigned char*) in;
  char *_out = out;
  int _outlen = outlen;

  while(inlen) {
    if (*c == 254) {
      /* Verbatim byte */
      if (outlen < 1) return _outlen++;
      *out = *(c+1);
      out++;
      outlen--;
      c += 2;
      inlen -= 2;
    } else if (*c == 255) {
      /* Verbatim string */
      int len = (*(c+1));
      if (outlen < len) return _outlen++;
      memcpy(out,c+2,len);
      out += len;
      outlen -= len;
      c += 2+len;
      inlen -= 2+len;
    } else {
      /* Codebook entry */
      const char *s = Smaz_rcb[*c];
      int len = strlen(s);

      if (outlen < len) return _outlen++;
      memcpy(out,s,len);
      out += len;
      outlen -= len;
      c++;
      inlen--;
    }
  }
  return out-_out;
}
