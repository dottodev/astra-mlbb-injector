#include "ipc.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__ANDROID__)
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "astra", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#endif

namespace astra {

bool RingBuffer::open(const char* path) {
    char* tmp = getenv("ASTRARING");
    const char* p = tmp ? tmp : path;
    fd_ = ::open(p, O_RDWR | O_CREAT, 0600);
    if (fd_ < 0) { LOGI("ring open failed %s\n", p); return false; }
    if (ftruncate(fd_, (off_t)sizeof(Slot) * kSlots) != 0) { ::close(fd_); fd_ = -1; return false; }
    map_ = mmap(nullptr, sizeof(Slot) * kSlots, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (map_ == MAP_FAILED) { map_ = nullptr; ::close(fd_); fd_ = -1; return false; }
    memset(map_, 0, sizeof(Slot) * kSlots);
    return true;
}

void RingBuffer::close() {
    if (map_) { munmap(map_, sizeof(Slot) * kSlots); map_ = nullptr; }
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

void RingBuffer::publish(const SharedFrame& frame) {
    if (!map_) return;
    Slot* s = &slots()[seq_ % kSlots];
    seq_++;
    memcpy(&s->frame, &frame, sizeof(SharedFrame));
    __sync_synchronize();          // publish data before seq
    s->seq = seq_;
    __sync_synchronize();
}

bool RingBuffer::latest(SharedFrame& out) const {
    if (!map_) return false;
    uint64_t best = 0; const Slot* bestSlot = nullptr;
    for (int i = 0; i < kSlots; ++i) {
        const Slot* s = &slots()[i];
        uint64_t q = s->seq;
        if (q && q > best) { best = q; bestSlot = s; }
    }
    if (!bestSlot) return false;
    memcpy(&out, &bestSlot->frame, sizeof(SharedFrame));
    return best == bestSlot->seq; // stable
}

} // namespace astra
