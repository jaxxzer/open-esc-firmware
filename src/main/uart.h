
// buffer size for rx and tx buffers
static const uint16_t UART_BUFFER_SIZE = 256;

bool initialize_uart(uint32_t address, uint32_t baudrate);
bool uart_write(uint32_t address, char c);
char uart_read(uint32_t address);
