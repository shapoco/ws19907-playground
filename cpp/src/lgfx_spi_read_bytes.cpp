#include <stdint.h>
#include <hardware/spi.h>

namespace lgfx::v1::spi {
  void readBytes(int spi_host, unsigned char* data, size_t length) {
    spi_inst_t* spi = spi_host == 0 ? spi0 : spi1;
    spi_write_read_blocking(spi, data, data, length);
  }
}
