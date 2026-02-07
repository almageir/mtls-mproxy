#ifndef MTLS_MPROXY_TRANSPORT_IO_BUFFER_H
#define MTLS_MPROXY_TRANSPORT_IO_BUFFER_H

#include <cstdint>
#include <vector>

namespace mtls_mproxy
{
    enum { max_buffer_size = 0x4000 };
    using IoBuffer = std::vector<std::uint8_t>;
}

#endif // MTLS_MPROXY_TRANSPORT_IO_BUFFER_H

