#ifdef SMEMORY_CPP

void sBus::map_reset() {
  for(uint32_t i = 0x0000; i <= 0xffff; i++)
    map(i << 8, memory::memory_unmapped, 0);

  for(uint16_t i = 0x2000; i <= 0x5fff; i++)
    memory::mmio.map(i, memory::mmio_unmapped);
}

void sBus::map_system() {
  map(MapDirect, 0x00, 0x3f, 0x2000, 0x5fff, memory::mmio);
  map(MapDirect, 0x80, 0xbf, 0x2000, 0x5fff, memory::mmio);

  map(MapDirect, 0x80, 0xbf, 0x2000, 0x5fff, memory::mmio);
  /* custom memmap */
  map(MapDirect, 0x70, 0x7f, 0x0000, 0x01ff, memory::mmio);

  map(MapLinear, 0x00, 0x3f, 0x0000, 0x1fff, memory::wram, 0x000000, 0x002000);
  map(MapLinear, 0x80, 0xbf, 0x0000, 0x1fff, memory::wram, 0x000000, 0x002000);

  map(MapLinear, 0x7e, 0x7f, 0x0000, 0xffff, memory::wram);
}

#endif
