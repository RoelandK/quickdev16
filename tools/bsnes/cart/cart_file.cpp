#ifdef CART_CPP

#include "../reader/filereader.hpp"

#if defined(GZIP_SUPPORT)
  #include "../reader/gzreader.hpp"
  #include "../reader/zipreader.hpp"
#endif

#if defined(JMA_SUPPORT)
  #include "../reader/jmareader.hpp"
#endif

string Cartridge::modify_extension(const char *filename_, const char *extension) const {
  string filename = filename_;
  int i;
  for(i = strlen(filename); i >= 0; i--) {
    if(filename[i] == '.')  break;
    if(filename[i] == '/')  break;
    if(filename[i] == '\\') break;
  }
  if(i > 0 && filename[i] == '.') filename[i] = 0;
  return filename << "." << extension;
}

string Cartridge::get_filename(const char *source, const char *extension, const char *path) const {
  return filepath(modify_extension(source, extension), path);
}

bool Cartridge::load_file(const char *fn, uint8 *&data, unsigned &size, CompressionMode compression) const {
  if(file::exists(fn) == false) return false;

  Reader::Type filetype = Reader::Normal;
  if(compression == CompressionInspect) filetype = Reader::detect(fn, true);
  if(compression == CompressionAuto) filetype = Reader::detect(fn, snes.config.file.autodetect_type);

  switch(filetype) { default:
    case Reader::Normal: {
      FileReader ff(fn);
      if(!ff.ready()) return false;
      size = ff.size();
      data = ff.read();
    } break;

    #ifdef GZIP_SUPPORT
    case Reader::GZIP: {
      GZReader gf(fn);
      if(!gf.ready()) return false;
      size = gf.size();
      data = gf.read();
    } break;

    case Reader::ZIP: {
      ZipReader zf(fn);
      if(!zf.ready()) return false;
      size = zf.size();
      data = zf.read();
    } break;
    #endif

    #ifdef JMA_SUPPORT
    case Reader::JMA: {
      try {
        JMAReader jf(fn);
        size = jf.size();
        data = jf.read();
      } catch(JMA::jma_errors jma_error) {
        return false;
      }
    } break;
    #endif
  }

  return true;
}

bool Cartridge::apply_patch(const uint8_t *pdata, const unsigned psize, uint8_t *&data, unsigned &size) const {
  uint8_t *outdata = 0;
  unsigned outsize;
  ups patcher;
  ups::result result = patcher.apply(pdata, psize, data, size, outdata, outsize);

  bool apply = false;
  if(result == ups::ok) apply = true;
  if(snes.config.file.bypass_patch_crc32 == true) {
    if(result == ups::input_crc32_invalid)  apply = true;
    if(result == ups::output_crc32_invalid) apply = true;
  }

  //if patch application was successful, replace old data, size with new data, size
  if(apply == true) {
    delete[] data;
    data = new uint8_t[size = outsize];
    memcpy(data, outdata, outsize);
  }

  if(outdata) delete[] outdata;
  return apply;
}

bool Cartridge::save_file(const char *fn, uint8 *data, unsigned size) const {
  file fp;
  if(!fp.open(fn, file::mode_write)) return false;
  fp.write(data, size);
  fp.close();
  return true;
}

#endif
