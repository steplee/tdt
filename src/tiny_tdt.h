#include "tinygltf/tiny_gltf.h"
#include "tinygltf/json.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <fstream>

/*
 * Note: Many (like all the ones I have my hands on) 3d tiles b3dm's use glTF 1.0
 * and the KHR_binary extension, which is unfortunate because it is now deprecated
 * in favor of the GLB format, which tinygltf can actually parse.
 */

namespace tinytdt {

using nlohmann::json;

struct BoundingRegion {
  // length 0, 6,      12,  4 (resp.)
  enum { NONE, REGION, BOX, SPHERE } type;
  std::vector<double> data;

  inline BoundingRegion() : type(NONE) {}
  BoundingRegion(const json& jobj);
};

enum class Refinement {
  ADD, REPLACE
};

struct TileModelB3DM;

struct TileSpec {
  TileSpec(); // Empty.
  TileSpec(const json& jobj);

  BoundingRegion boundingRegion;
  BoundingRegion viewerRequestRegion;
  std::vector<double> viewerRequestVolume; // length 0 or 4
  std::vector<double> transform;
  Refinement refine;
  double geometricError;

  std::vector<TileSpec> children;
  struct {
    std::string url;
    std::unique_ptr<TileModelB3DM> model=nullptr; // This COULD be lazy-loaded on seperate thread, but right now is not.
    BoundingRegion boundingVolume;
  } content;

  void print(std::ostream& os, int depth=0) const;
};

struct Tileset {
  Tileset(const std::string& url);

  json asset, properties;
  double geometricError;
  TileSpec root;


  void print(std::ostream& os, int depth=0) const;
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
  TileModelI3DM(tinygltf::TinyGLTF*, const std::string& url);

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
  TileModelB3DM(tinygltf::TinyGLTF*, const std::string& url);

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

  void print(std::ostream& os, int depth=0) const;
};

bool ReadWholeFile(std::vector<unsigned char> *out, std::string *err,
                   const std::string &filepath, void *);

template<class T> inline T view(const std::vector<uint8_t>& v, const size_t& byte) {
  return static_cast<T>(((char*)v.data())+byte);
}
#include <endian.h>
#include <byteswap.h>
inline uint32_t to_little(const uint32_t& a) { return ntohl(a); }
inline uint16_t to_little(const uint16_t& a) { return ntohs(a); }
template<class T> T view_little(const std::vector<uint8_t>& v, const size_t& byte) {
  /*
#if __BYTE_ORDER != __LITTLE_ENDIAN
  if (std::is_same<std::make_unsigned<T>, std::int16_t>::value) {
    return static_cast<T>(bswap_16(*reinterpret_cast<typename std::add_pointer<std::make_unsigned<T>>::type>(((char*)v.data())+byte)));
  }
  if (std::is_same<std::make_unsigned<T>, std::int32_t>::value) {
    return static_cast<T>(bswap_32(*reinterpret_cast<typename std::add_pointer<std::make_unsigned<T>>::type>(((char*)v.data())+byte)));
  }
  if (std::is_same<std::make_unsigned<T>, std::int64_t>::value) {
    return static_cast<T>(bswap_64(*reinterpret_cast<typename std::add_pointer<std::make_unsigned<T>>::type>(((char*)v.data())+byte)));
  }
#endif
  return *reinterpret_cast<T*>(((char*)v.data())+byte);
  */
#if __BYTE_ORDER != __LITTLE_ENDIAN
  if (std::is_same<T, std::int32_t>::value or std::is_same<T, std::uint32_t>::value) {
    uint32_t z = *reinterpret_cast<uint32_t*> ( ((char*)v.data())+byte );
    return bswap_32(z);
  }
  if (std::is_same<T, std::int16_t>::value or std::is_same<T, std::uint16_t>::value) {
    uint16_t z = *reinterpret_cast<uint16_t*> ( ((char*)v.data())+byte );
    return bswap_16(z);
  }
#endif
  T z = *reinterpret_cast<T*> ( ((char*)v.data())+byte );
}

/*
 * Declare util functions.
 */

// ...

#ifdef TINY_TDT_IMPLEMENTATION

/*
 * Parsing functions.
 */

bool ReadWholeFile(std::vector<unsigned char> *out, std::string *err,
                   const std::string &filepath, void *) {
  std::ifstream f(filepath.c_str(), std::ifstream::binary);
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
}

inline bool parse_double_array(std::vector<double> *ret,
                                     const json &o, const std::string &key,
                                     bool required) {
  auto it = o.find(key);

  if (it == o.end()) {
    if (required) {
      std::cerr << "Missing (required) array: " << key << std::endl;
      exit(1);
    }
    return false;
  }

  ret->reserve(it->size());

  for (double d : *it) {
    ret->push_back(d);
  }

  return true;
}

BoundingRegion::BoundingRegion(const json& jobj) {
  if (jobj.find("box") != jobj.end()) {
    type = BoundingRegion::BOX;
    parse_double_array(&data, jobj, "box", true);
    assert(data.size() == 12);
  }
  else if (jobj.find("sphere") != jobj.end()) {
    type = BoundingRegion::SPHERE;
    parse_double_array(&data, jobj, "sphere", true);
    assert(data.size() == 4);
  }
  else if (jobj.find("region") != jobj.end()) {
    type = BoundingRegion::REGION;
    parse_double_array(&data, jobj, "region", true);
    assert(data.size() == 6);
  }
  else {
    std::cerr << "No bounding region primitive specified!\n";
    exit(1);
  }
}

Tileset::Tileset(const std::string& url) {
  std::string err;
  std::vector<unsigned char> json_bytes;
  bool ret = ReadWholeFile(&json_bytes, &err, url, nullptr);
  if (not ret) {
    std::cerr << "Error loading tileset file: " << url << " :: " << err << std::endl;
    exit(1);
  }
  auto jobj = json::parse(json_bytes.data(), json_bytes.data()+json_bytes.size(), nullptr, false);

  asset = jobj["asset"];
  properties = jobj["properties"];
  geometricError = jobj["geometricError"];

  if (jobj.find("root") != jobj.end())
    root = TileSpec(jobj["root"]);
}

TileSpec::TileSpec() {
}
TileSpec::TileSpec(const json& jobj) {
  if (jobj.find("boundingRegion") != jobj.end())
    boundingRegion = BoundingRegion(jobj["boundingRegion"]);
  if (jobj.find("viewerRequestRegion") != jobj.end())
    viewerRequestRegion = BoundingRegion(jobj["viewerRequestRegion"]);
  if (jobj.find("viewerRequestVolume") != jobj.end()) {
    parse_double_array(&viewerRequestVolume, jobj, "viewerRequestVolume", true);
    assert(viewerRequestVolume.size() == 4);
  }
  if (jobj.find("transform") != jobj.end()) {
    parse_double_array(&transform, jobj, "transform", true);
    assert(transform.size() == 16);
  }

  if (jobj.find("refine") != jobj.end())
    refine = jobj["refine"] == "ADD" ? Refinement::ADD : Refinement::REPLACE;

  geometricError = jobj["geometricError"];

  if (jobj.find("content") != jobj.end()) {
    json content_jobj = jobj["content"];
    if (content_jobj.find("boundingVolume") != content_jobj.end())
      content.boundingVolume = BoundingRegion(content_jobj["boundingVolume"]);
    if (content_jobj.find("url") != content_jobj.end()) {
      content.url = content_jobj["url"];

      // TODO should lazy-load on seperate thread.
      tinygltf::TinyGLTF tiny_gltf;
      content.model = std::make_unique<TileModelB3DM>(&tiny_gltf, content.url);
    }
    else {
      std::cerr << "No tile content url specified!\n";
      exit(1);
    }
  }

  if (jobj.find("children") != jobj.end())
    for (const auto& child_jobj : jobj["children"]) {
      children.emplace_back(child_jobj);
    }
}


TileModelB3DM::TileModelB3DM(tinygltf::TinyGLTF *tinygltf, const std::string& url) {
  std::stringstream ss;

  std::vector<unsigned char> data;
  std::string fileerr;
  bool fileread = ReadWholeFile(&data, &fileerr, url, nullptr);
  assert(fileread);
  assert(data.size() > 0);

  // Magic.
  std::cout << "magic:\n" << std::hex << view_little<uint32_t>(data, 0) << std::dec << "\n";
  //assert(view_little<uint32_t>(data, 0) == 0x6233646d);
  assert(view_little<uint32_t>(data, 0) == 0x6d643362);

  // Parse header.
  header.version                      = view_little<uint32_t>(data, 4);
  header.byteLength                   = view_little<uint32_t>(data, 8);
  header.featureTableJsonByteLength   = view_little<uint32_t>(data, 12);
  header.featureTableBinaryByteLength = view_little<uint32_t>(data, 16);
  header.batchTableJsonByteLength     = view_little<uint32_t>(data, 20);
  header.batchTableBinaryByteLength   = view_little<uint32_t>(data, 24);
  //header.gltfFormat                   = view_little<uint32_t>(data, 32);

  std::cout << "file size       : " << data.size() << std::endl;
  std::cout << "header byte size: " << header.byteLength << std::endl;
  assert(data.size() == header.byteLength);

  size_t offset = 28;

  // Parse feature table.
  std::cout << "load feature table, sizes " << header.featureTableJsonByteLength << " " <<  header.featureTableBinaryByteLength << std::endl;
  char* featureTableByteStart = (char*) data.data() + offset;
  featureTable.load(featureTableByteStart, header.featureTableJsonByteLength, header.featureTableBinaryByteLength);
  offset += header.featureTableJsonByteLength + header.featureTableBinaryByteLength;

  // Parse batch table.
  std::cout << "load batch table, sizes " << header.batchTableJsonByteLength << " " <<  header.batchTableBinaryByteLength << std::endl;
  char* batchTableByteStart = (char*) data.data() + offset;
  batchTable.load(batchTableByteStart, header.batchTableJsonByteLength, header.batchTableBinaryByteLength);
  offset += header.batchTableJsonByteLength + header.batchTableBinaryByteLength;

  // Get glTF as string. B3DM always has binary glTF embedded.
  unsigned char* gltf_str = ((unsigned char*) data.data()) + offset;
  std::cout << "gltf magic: ";
  std::cout << view_little<char>(data, offset+0) << " ";
  std::cout << view_little<char>(data, offset+1) << " ";
  std::cout << view_little<char>(data, offset+2) << " ";
  std::cout << view_little<char>(data, offset+3) << "\n";
  uint32_t gltf_version = view_little<uint32_t>(data, offset+4);
  uint32_t gltf_len = view_little<uint32_t>(data, offset+8);
  uint32_t gltf_model_len = view_little<uint32_t>(data, offset+12);
  uint32_t gltf_model_format = view_little<uint32_t>(data, offset+16);
  std::cout << "gltf stuff:\n";
  std::cout << "  len " << gltf_len << "\n";
  std::cout << "  model_len " << gltf_model_len << "\n";
  std::cout << "  format " << gltf_model_format << "\n";
  assert(gltf_len+offset == data.size());
  std::string err, warn;
  bool good = tinygltf->LoadBinaryFromMemory(&model, &err, &warn, gltf_str, gltf_len);
  if (not good) {
    std::cout << "Error loading gltf!\n - warn: " << warn << "\n - err: " << err << std::endl;
    assert(false);
  }

  // Done.
}

#if 0
TileModelI3DM::TileModelI3DM(tinygltf::TinyGLTF *tinygltf, const std::string& url) {
  std::stringstream ss;

  std::vector<unsigned char> data;
  std::string fileerr;
  bool fileread = ReadWholeFile(&data, &fileerr, url, nullptr);
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
#endif



bool FeatureTable::load(const char* buf, uint32_t jsonLength, uint32_t binaryLength) {
  return true;
}
bool BatchTable::load(const char* buf, uint32_t jsonLength, uint32_t binaryLength) {
  return true;
}

static inline std::ostream& tab_it(std::ostream& os, int d) {
  while (d--) os << '\t';
  return os;
}


void Tileset::print(std::ostream& os, int depth) const {
  tab_it(os, depth) << "- Tileset:\n";
  tab_it(os, depth) << "  - Root:\n";
  root.print(os, depth+1);
}
void TileSpec::print(std::ostream& os, int depth) const {
  tab_it(os, depth) << "- TileSpec:\n";
  if (transform.size()) {
    tab_it(os, depth) << "  - transform: [";
    for (int i=0; i<transform.size(); i++) {
      os << transform[i];
      if (i < transform.size()-1) os << ", ";
    }
    os << "]\n";
  }

  tab_it(os, depth) << "  - geoError: " << geometricError << "\n";
  tab_it(os, depth) << "  - content : " << content.url << "\n";
  if (content.model) {
    tab_it(os, depth) << "  - content (model): \n";
    content.model->print(os, depth+1);
  }

  if (children.size()) {
    tab_it(os, depth) << "  - children (" << children.size() << "):\n";
    for (const auto& c : children)
      c.print(os, depth+1);
  } else
    tab_it(os, depth) << "  - no children\n";
}
void TileModelB3DM::print(std::ostream& os, int depth) const {
  tab_it(os, depth) << "- B3DM Tile Model:" << "\n";
  tab_it(os, depth) << "-   asset version: " << model.asset.version << "\n";
  tab_it(os, depth) << "-   asset gen    : " << model.asset.generator << "\n";
  tab_it(os, depth) << "-   n_scenes  : " << model.scenes.size() << "\n";
  tab_it(os, depth) << "-   n_meshes  : " << model.meshes.size() << "\n";
  tab_it(os, depth) << "-   n_nodes   : " << model.nodes.size()  << "\n";
  tab_it(os, depth) << "-   n_buffers : " << model.buffers.size()  << "\n";
  tab_it(os, depth) << "-   n_bufferViews : " << model.bufferViews.size()  << "\n";
}


#endif
}
