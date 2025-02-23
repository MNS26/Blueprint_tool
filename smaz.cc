#include <string.h>
#include "smaz.h"

int smaz_compress(char *in, int inlen, char *out, int outlen) {
  unsigned int h1 = 0;
  unsigned int h2 = 0;
  unsigned int h3 = 0;
  int verblen = 0, _outlen = outlen;
  char verb[256], *_out = out;

  while(inlen) {
    int j = 7, needed;
    char *flush = NULL;
    const char *slot;

    h1 = h2 = in[0]<<3;
    if (inlen > 1) h2 += in[1];
    if (inlen > 2) h3 = h2^in[2];
    if (j > inlen) j = inlen;

    /* Try to lookup substrings into the hash table, starting from the
     * longer to the shorter substrings */
    for (; j > 0; j--) {
      switch(j) {
      case 1: slot = Smaz_cb[h1%241]; break;
      case 2: slot = Smaz_cb[h2%241]; break;
      default: slot = Smaz_cb[h3%241]; break;
      }
      while(slot[0]) {
        if (slot[0] == j && memcmp(slot+1,in,j) == 0) {
          /* Match found in the hash table,
           * prepare a verbatim bytes flush if needed */
          if (verblen) {
            needed = (verblen == 1) ? 2 : 2+verblen;
            flush = out;
            out += needed;
            outlen -= needed;
          }
          /* Emit the byte */
          if (outlen <= 0) return _outlen+1;
          out[0] = slot[slot[0]+1];
          out++;
          outlen--;
          inlen -= j;
          in += j;
          goto out;
        } else {
          slot += slot[0]+2;
        }
      }
    }
    /* Match not found - add the byte to the verbatim buffer */
    verb[verblen] = in[0];
    verblen++;
    inlen--;
    in++;
out:
    /* Prepare a flush if we reached the flush length limit, and there
     * is not already a pending flush operation. */
    if (!flush && (verblen == 256 || (verblen > 0 && inlen == 0))) {
      needed = (verblen == 1) ? 2 : 2+verblen;
      flush = out;
      out += needed;
      outlen -= needed;
      if (outlen < 0) return _outlen+1;
    }
    /* Perform a verbatim flush if needed */
    if (flush) {
      if (verblen == 1) {
        flush[0] = (signed char)254;
        flush[1] = verb[0];
      } else {
        flush[0] = (signed char)255;
        flush[1] = (signed char)(verblen-1);
        memcpy(flush+2,verb,verblen);
      }
        flush = NULL;
        verblen = 0;
    }
  }
  return out-_out;
}


int smaz_decompress(char *in, int inlen, char *out, int outlen) {
  unsigned char *c = (unsigned char*) in;
  char *_out = out;
  int _outlen = outlen;

  while(inlen) {
    if (*c == 0xfe) {
      /* Verbatim byte */
      if (outlen < 1) return _outlen++;
      *out = *(c+1);
      out++;
      outlen--;
      c += 2;
      inlen -= 2;
    } else if (*c == 0xff) {
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
