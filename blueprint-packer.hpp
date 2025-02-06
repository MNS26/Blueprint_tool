using namespace std;

class blueprint_unpacker
{
private:
  /* data */
  
public:
  int64_t decompress_data_internal(uint8_t* srcBuffer,size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize,size_t filled, size_t alreadyConsumed, LZ4F_dctx* dctx);
  size_t decompress_file_allocDst(uint8_t *srcBuffer, size_t srcBufferSize,uint8_t *dstBuffer, size_t dstBufferSize, LZ4F_dctx *dctx);
  int decompress_data(uint8_t* srcBuffer, size_t srcBufferSize, uint8_t* dstBuffer, size_t dstBufferSize);
  std::vector<uint8_t> extract_data_from_image(const unsigned char *image_data, int width, int height);
  size_t decompress_protobuf(std::vector<uint8_t> protobuf, string *json_buffer);

  blueprint_unpacker(/* args */);
  ~blueprint_unpacker();
};

