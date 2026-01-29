#ifndef STREAM_H
#define STREAM_H

#include "io_buffer.h"

#include <memory>

class stream_manager;
using stream_manager_ptr = std::shared_ptr<stream_manager>;

class stream
{
public:
    enum {
        max_buffer_size = 0x4000
    };

    explicit stream(stream_manager_ptr smp, int id = 0)
        : stream_manager_(std::move(smp)), id_(id) {
    }

    virtual ~stream() = default;

    void start() { do_start(); }
    void stop() { do_stop(); }
    void read() { do_read(); }
    void write(io_buffer event) { do_write(std::move(event)); }

    [[nodiscard]] int id() const { return id_; }

protected:
    stream_manager_ptr manager() { return stream_manager_; }

private:
    virtual void do_start() = 0;
    virtual void do_stop() = 0;
    virtual void do_read() = 0;
    virtual void do_write(io_buffer event) = 0;

    stream_manager_ptr stream_manager_;
    int id_;
};

using stream_ptr = std::shared_ptr<stream>;




#endif //STREAM_H
