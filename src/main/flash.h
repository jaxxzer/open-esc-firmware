// Flash memory interface

bool flash_initialize();
char flash_read(uint32_t address);
bool flash_write(uint32_t address, uint16_t value);
bool flash_erase_page(uint32_t address);
