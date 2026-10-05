#pragma once
#include <cstdint>

namespace deskh {
struct Point { int x = 0; int y = 0; };
struct Limits {
    std::uint32_t milliseconds = 500;
    int doubleX = 2, doubleY = 2;
    int dragX = 2, dragY = 2;
};

// Pure state machine. Movement during a press is deliberately irrelevant:
// preserve the optimized reference branch's final-release displacement rule.
class ClickDetector {
public:
    void reset() { down_ = candidate_ = false; }
    void press(Point p, std::uint32_t time, bool blank) {
        if (!blank || down_) { reset(); return; }
        down_ = true;
        point_ = p;
        time_ = time;
    }
    bool release(Point p, bool blank, const Limits& limits) {
        if (!down_) return false;
        down_ = false;
        if (!blank || !near(p, point_, limits.dragX, limits.dragY)) {
            candidate_ = false;
            return false;
        }
        // Unsigned subtraction correctly handles the DWORD tick rollover.
        const bool match = candidate_ && (time_ - previousTime_ <= limits.milliseconds)
            && near(point_, previous_, limits.doubleX, limits.doubleY);
        candidate_ = !match;
        previous_ = point_;
        previousTime_ = time_;
        return match;
    }
private:
    static bool near(Point a, Point b, int x, int y) {
        const auto dx = static_cast<std::int64_t>(a.x) - b.x;
        const auto dy = static_cast<std::int64_t>(a.y) - b.y;
        return dx >= -x && dx <= x && dy >= -y && dy <= y;
    }
    bool down_ = false, candidate_ = false;
    Point point_{}, previous_{};
    std::uint32_t time_ = 0, previousTime_ = 0;
};
}
