#ifndef MTLS_MPROXY_TRANSPORT_STREAM_H
#define MTLS_MPROXY_TRANSPORT_STREAM_H

#include "io_buffer.h"

#include <memory>

namespace mtls_mproxy
{
    class StreamManager;
    using StreamManagerPtr = std::shared_ptr<StreamManager>;

    class Stream
    {
    public:
        enum {
            max_buffer_size = 0x4000
        };

        explicit Stream(StreamManagerPtr smp, int id = 0)
            : stream_manager_(std::move(smp)), id_(id) {
        }

        virtual ~Stream() = default;

        void start() { do_start(); }
        void stop() { do_stop(); }
        void read() { do_read(); }
        void write(IoBuffer event) { do_write(std::move(event)); }

        [[nodiscard]] int id() const { return id_; }

    protected:
        StreamManagerPtr manager() { return stream_manager_; }

    private:
        virtual void do_start() = 0;
        virtual void do_stop() = 0;
        virtual void do_read() = 0;
        virtual void do_write(IoBuffer event) = 0;

        StreamManagerPtr stream_manager_;
        int id_;
    };

    using stream_ptr = std::shared_ptr<Stream>;
}

#endif // MTLS_MPROXY_TRANSPORT_STREAM_H
