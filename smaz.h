#ifndef _SMAZ_H
#define _SMAZ_H

int smaz_compress(char *in, int inlen, char *out, int outlen);
void cpp_smaz_compress(std::vector<char>& in, std::vector<uint8_t>& out);

int smaz_decompress(char *in, int inlen, char *out, int outlen);
void cpp_smaz_decompress(std::vector<uint8_t>& in, std::vector<char>& out);
#endif