#include "click_detector.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace deskh;
static int checks = 0;
static void check(bool value, const char* label) {
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); }
}
int main() {
    ClickDetector state;
    const Limits limits{};
    auto click = [&](Point p, unsigned t, bool blank = true) {
        state.press(p, t, blank); return state.release(p, blank, limits);
    };
    check(!click({10, 10}, 100), "single click");
    check(click({10, 10}, 250), "double click");
    check(!click({10, 10}, 300), "third click does not reuse pair");
    check(click({10, 10}, 400), "fourth click pairs third");
    state.reset(); click({0, 0}, 0);
    check(!click({0, 0}, 501), "timeout");
    state.reset(); click({0, 0}, 0);
    check(click({2, -2}, 500), "inclusive system half-rectangle/time boundary");
    state.reset(); click({0, 0}, 0);
    check(!click({3, 0}, 200), "outside double-click half rectangle");
    state.reset(); click({0, 0}, 0); state.press({0, 0}, 100, true);
    check(!state.release({30, 30}, true, limits), "rubber-band rejected");
    check(!click({30, 30}, 200), "single click after rubber-band rejected");
    check(click({30, 30}, 300), "recovery after rubber-band");
    state.reset(); click({0, 0}, 0); state.press({0, 0}, 100, true);
    check(state.release({0, 0}, true, limits), "jitter returns to press point");
    state.reset(); click({0, 0}, 0); click({0, 0}, 100, false);
    check(!click({0, 0}, 200), "icon or foreign-window press cancels pair");
    state.reset(); click({0, 0}, 0); state.press({0, 0}, 100, true);
    check(!state.release({0, 0}, false, limits), "release on non-background");
    check(!click({0, 0}, 200), "non-background release clears pair");
    state.reset(); check(!state.release({0, 0}, true, limits), "orphan release");
    click({0, 0}, 0); state.reset();
    check(!click({0, 0}, 100), "pause/modifier/other-button reset");
    state.reset(); click({-1900, -50}, 100);
    check(click({-1899, -49}, 200), "negative monitor coordinates");
    state.reset(); click({0, 0}, 0xFFFFFFF0u);
    check(click({0, 0}, 0x20u), "DWORD clock rollover");
    state.reset(); click({0, 0}, 1000);
    check(!click({0, 0}, 900), "out-of-order time rejected");
    state.reset(); click({0, 0}, 0); state.press({0, 0}, 100, true); state.press({0, 0}, 120, true);
    check(!state.release({0, 0}, true, limits), "duplicate press reset");
    state.reset(); click({std::numeric_limits<int>::min(), 0}, 0);
    check(!click({std::numeric_limits<int>::max(), 0}, 100), "coordinate subtraction overflow");
    std::printf("PASS: %d input state checks\n", checks);
}
