#include "tinygltf/tiny_gltf.h"
#include "tinygltf/json.hpp"

#include <arpa/inet.h>

namespace tinytdt {

using nlohmann::json;

struct BoundingRegion {
  // length 0, 12,     6,   4 (resp.)
  enum { NONE, REGION, BOX, SPHERE } type;
  std::vector<float> data;
};

enum class Refinement {
  ADD, REPLACE
};

struct TileSpec {
  BoundingRegion boundingRegion;
  BoundingRegion viewerRequestRegion;
  std::vector<float> viewerRequestVolume; // length 0 or 4
  std::vector<float> transform;
  Refinement refine;

  std::vector<TileSpec> children;
  struct {
    std::string uri;
    BoundingRegion boundingVolume;
  } content;
};

struct Tileset {
  json asset, properties;
  float geometricError;
  TileSpec root;
};



/*
 * Specific Tile Models.
 */

struct FeatureTable {
  bool load(const char* buf, uint32_t jsonLength, uint32_t binaryLength);
};
struct BatchTable {
  bool load(const char* buf, uint32_t jsonLength, uint32_t binaryLength);
};

#if 0
struct TileModelI3DM {
  // Construct after loading file.
  TileModelI3DM(tinygltf::TinyGLTF*, const std::string& uri);

  struct {
    uint32_t version, byteLength,
             featureTableJsonByteLength,
             featureTableBinaryByteLength,
             batchTableJsonByteLength,
             batchTableBinaryByteLength,
             gltfFormat;
  } header;

  FeatureTable featureTable;
  BatchTable batchTable;

  tinygltf::Model model;
};
#endif

struct TileModelB3DM {
  // Construct after loading file.
  TileModelB3DM(tinygltf::TinyGLTF*, const std::string& uri);

  struct {
    uint32_t version, byteLength,
             featureTableJsonByteLength,
             featureTableBinaryByteLength,
             batchTableJsonByteLength,
             batchTableBinaryByteLength;
  } header;

  FeatureTable featureTable;
  BatchTable batchTable;

  tinygltf::Model model;
};


/*
 * Parsing functions.
 */

bool ReadWholeFile(std::vector<unsigned char> *out, std::string *err,
                   const std::string &filepath, void *) {
#ifdef _WIN32
#if defined(__GLIBCXX__) // mingw
  int file_descriptor = _wopen(UTF8ToWchar(filepath).c_str(), _O_RDONLY | _O_BINARY);
  __gnu_cxx::stdio_filebuf<char> wfile_buf(file_descriptor, std::ios_base::in);
  std::istream f(&wfile_buf);
#elif defined(_MSC_VER)
  std::ifstream f(UTF8ToWchar(filepath).c_str(), std::ifstream::binary);
#else // clang?
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
#endif
  if (!f) {
    if (err) {
      (*err) += "File open error : " + filepath + "\n";
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  f.seekg(0, f.beg);

  if (int64_t(sz) < 0) {
    if (err) {
      (*err) += "Invalid file size : " + filepath +
                " (does the path point to a directory?)";
    }
    return false;
  } else if (sz == 0) {
    if (err) {
      (*err) += "File is empty : " + filepath + "\n";
    }
    return false;
  }

  out->resize(sz);
  f.read(reinterpret_cast<char *>(&out->at(0)),
         static_cast<std::streamsize>(sz));

  return true;
#endif
}

template<class T> T view(const std::vector<uint8_t>& v, const size_t& byte) {
  return static_cast<T>(((char*)v.data())+byte);
}
#include <endian.h>
#include <byteswap.h>
inline uint32_t to_little(const uint32_t& a) { return ntohl(a); }
inline uint16_t to_little(const uint16_t& a) { return ntohs(a); }
template<class T> T view_little(const std::vector<uint8_t>& v, const size_t& byte) {
#if __BYTE_ORDER != __LITTLE_ENDIAN
  if (std::is_same<std::make_unsigned<T>, std::int16_t>::value) {
    //uint16_t val = static_cast<uint16_t>(((char*)v.data())+byte);
    //return static_cast<T>(to_little(val));
    return static_cast<T>(bswap_16(static_cast<typename std::make_unsigned<T>::type>(*((char*)v.data())+byte)));
  }
  if (std::is_same<std::make_unsigned<T>, std::int32_t>::value) {
    //uint32_t val = static_cast<uint32_t>(((char*)v.data())+byte);
    //return static_cast<T>(to_little(val));
    return static_cast<T>(bswap_32(static_cast<typename std::make_unsigned<T>::type>(*((char*)v.data())+byte)));
  }
  if (std::is_same<std::make_unsigned<T>, std::int64_t>::value) {
    return static_cast<T>(bswap_64(static_cast<typename std::make_unsigned<T>::type>(*((char*)v.data())+byte)));
  }
#endif
  return static_cast<T>(*((char*)v.data())+byte);
}

TileModelB3DM::TileModelB3DM(tinygltf::TinyGLTF *tinygltf, const std::string& uri) {
  std::stringstream ss;

  std::vector<unsigned char> data;
  std::string fileerr;
  bool fileread = ReadWholeFile(&data, &fileerr, uri, nullptr);
  assert(fileread);

  assert(data.size() > 0);
  assert(data.size() == header.byteLength);

  // Magic.
  assert(view_little<uint32_t>(data, 0) == 0x6233646d);

  // Parse header.
  header.version                      = view_little<uint32_t>(data, 4);
  header.byteLength                   = view_little<uint32_t>(data, 8);
  header.featureTableJsonByteLength   = view_little<uint32_t>(data, 16);
  header.featureTableBinaryByteLength = view_little<uint32_t>(data, 20);
  header.batchTableJsonByteLength     = view_little<uint32_t>(data, 24);
  header.batchTableBinaryByteLength   = view_little<uint32_t>(data, 28);
  //header.gltfFormat                   = view_little<uint32_t>(data, 32);

  size_t offset = 28;

  // Parse feature table.
  char* featureTableByteStart = (char*) data.data() + offset;
  featureTable.load(featureTableByteStart, header.featureTableJsonByteLength, header.featureTableBinaryByteLength);
  offset += header.featureTableJsonByteLength + header.featureTableBinaryByteLength;

  // Parse batch table.
  char* batchTableByteStart = (char*) data.data() + offset;
  batchTable.load(batchTableByteStart, header.batchTableJsonByteLength, header.batchTableBinaryByteLength);
  offset += header.batchTableJsonByteLength + header.batchTableBinaryByteLength;

  // Get glTF as string. Either load from disk, or reference the embedded blob.
  // Get glTF as string. B3DM always has binary glTF embedded.
  unsigned char* gltf_str = (unsigned char*) data.data() + offset;
  unsigned int gltf_len = data.size() - offset;
  std::string err, warn;
  bool good = tinygltf->LoadBinaryFromMemory(&model, &err, &warn, gltf_str, gltf_len);
  if (not good) {
    std::cout << "Error loading gltf!\n - warn: " << warn << "\n - err: " << err << std::endl;
    assert(false);
  }

  // Done.
}

TileModelI3DM::TileModelI3DM(tinygltf::TinyGLTF *tinygltf, const std::string& uri) {
  std::stringstream ss;

  std::vector<unsigned char> data;
  std::string fileerr;
  bool fileread = ReadWholeFile(&data, &fileerr, uri, nullptr);
  assert(fileread);

  assert(data.size() > 0);
  assert(data.size() == header.byteLength);

  // Magic.
  assert(view_little<uint32_t>(data, 0) == 0x6233646d);

  // Parse header.
  header.version                      = view_little<uint32_t>(data, 4);
  header.byteLength                   = view_little<uint32_t>(data, 8);
  header.featureTableJsonByteLength   = view_little<uint32_t>(data, 16);
  header.featureTableBinaryByteLength = view_little<uint32_t>(data, 20);
  header.batchTableJsonByteLength     = view_little<uint32_t>(data, 24);
  header.batchTableBinaryByteLength   = view_little<uint32_t>(data, 28);
  header.gltfFormat                   = view_little<uint32_t>(data, 32);

  size_t offset = 32;

  // Parse feature table.
  char* featureTableByteStart = (char*) data.data() + offset;
  featureTable.load(featureTableByteStart, header.featureTableJsonByteLength, header.featureTableBinaryByteLength);
  offset += header.featureTableJsonByteLength + header.featureTableBinaryByteLength;

  // Parse batch table.
  char* batchTableByteStart = (char*) data.data() + offset;
  batchTable.load(batchTableByteStart, header.batchTableJsonByteLength, header.batchTableBinaryByteLength);
  offset += header.batchTableJsonByteLength + header.batchTableBinaryByteLength;

  // Get glTF as string. Either load from disk, or reference the embedded blob.
  // Get glTF as string. B3DM always has binary glTF embedded.
  unsigned char* gltf_str = (unsigned char*) data.data() + offset;
  unsigned int gltf_len = data.size() - offset;
  std::string err, warn;

  // TODO
  if (gltfFormat == 0) {
  }
  bool good = tinygltf->LoadBinaryFromMemory(&model, &err, &warn, gltf_str, gltf_len);
  if (not good) {
    std::cout << "Error loading gltf!\n - warn: " << warn << "\n - err: " << err << std::endl;
    assert(false);
  }

  // Done.
}


}
