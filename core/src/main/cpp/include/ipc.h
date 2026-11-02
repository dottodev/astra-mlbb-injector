#pragma once
#include <stdint.h>
#include <string.h>

namespace astra {

enum { ENTITY_CAP = 32 };

struct Entity {
    float x, y, z;
    float screenX, screenY;
    float health;
    int32_t team;
    int32_t heroId;
    uint8_t valid;
    uint8_t reserved[3];
};

struct SharedFrame {
    uint64_t seq;
    float deltaTime;
    int count;
    Entity entities[ENTITY_CAP];
};

// mmap-backed, single-writer/C++ friendly. Fixed-size ring of frames.
class RingBuffer {
public:
    static constexpr int kSlots = 4;
    bool open(const char* path);
    void close();
    void publish(const SharedFrame& frame);   // one producer (core)
    bool latest(SharedFrame& out) const;      // one consumer (overlay)
private:
    int fd_ = -1;
    void* map_ = nullptr;
    struct Slot { uint64_t seq; SharedFrame frame; char pad[64]; };
    Slot* slots() { return reinterpret_cast<Slot*>(map_); }
    const Slot* slots() const { return reinterpret_cast<const Slot*>(map_); }
    uint64_t seq_ = 0;
};

} // namespace astra
