#ifdef SMEMORY_CPP

void sBus::map_cx4() {
  map(MapDirect, 0x00, 0x3f, 0x6000, 0x7fff, cx4);
  map(MapDirect, 0x80, 0xbf, 0x6000, 0x7fff, cx4);
}

void sBus::map_dsp1() {
  switch(cartridge.dsp1_mapper()) {
    case Cartridge::DSP1LoROM1MB: {
      printf("sBus::map_dsp1 DSP1LoROM1MB\n");
      map(MapDirect, 0x20, 0x3f, 0x8000, 0xffff, dsp1);
      map(MapDirect, 0xa0, 0xbf, 0x8000, 0xffff, dsp1);
    } break;

    case Cartridge::DSP1LoROM2MB: {
      printf("sBus::map_dsp1 DSP1LoROM2MB\n");
      map(MapDirect, 0x60, 0x6f, 0x0000, 0x7fff, dsp1);
      map(MapDirect, 0xe0, 0xef, 0x0000, 0x7fff, dsp1);
    } break;

    case Cartridge::DSP1HiROM: {
      printf("sBus::map_dsp1 DSP1HiROM\n");
      map(MapDirect, 0x00, 0x1f, 0x6000, 0x7fff, dsp1);
      map(MapDirect, 0x80, 0x9f, 0x6000, 0x7fff, dsp1);
    } break;
  }
}

void sBus::map_dsp2() {
  printf("sBus::map_dsp2\n");
  map(MapDirect, 0x20, 0x3f, 0x6000, 0x6fff, dsp2);
  map(MapDirect, 0x20, 0x3f, 0x8000, 0xbfff, dsp2);
  map(MapDirect, 0xa0, 0xbf, 0x6000, 0x6fff, dsp2);
  map(MapDirect, 0xa0, 0xbf, 0x8000, 0xbfff, dsp2);
}

void sBus::map_dsp3() {
  printf("sBus::map_dsp3\n");
  map(MapDirect, 0x20, 0x3f, 0x8000, 0xffff, dsp3);
  map(MapDirect, 0xa0, 0xbf, 0x8000, 0xffff, dsp3);
}

void sBus::map_dsp4() {
  printf("sBus::map_dsp4\n");
  map(MapDirect, 0x30, 0x3f, 0x8000, 0xffff, dsp4);
  map(MapDirect, 0xb0, 0xbf, 0x8000, 0xffff, dsp4);
}

void sBus::map_obc1() {
  printf("sBus::map_obc1\n");
  map(MapDirect, 0x00, 0x3f, 0x6000, 0x7fff, obc1);
  map(MapDirect, 0x80, 0xbf, 0x6000, 0x7fff, obc1);
}

void sBus::map_st010() {
  printf("sBus::map_st010\n");
  map(MapDirect, 0x68, 0x6f, 0x0000, 0x0fff, st010);
  map(MapDirect, 0xe8, 0xef, 0x0000, 0x0fff, st010);
}

#endif
