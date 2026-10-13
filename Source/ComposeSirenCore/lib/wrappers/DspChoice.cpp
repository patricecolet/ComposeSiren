#include "DspChoice.h"

#include <cstdlib>
#include <mutex>

namespace cs {

namespace {
std::mutex& lock() { static std::mutex m; return m; }

DspChoice& current()
{
    static DspChoice c = [] {
        DspChoice d;
        if (const char* v = std::getenv("COMPOSESIREN_DSP_VARIANT"); v && *v) d.variant = v;
        if (const char* v = std::getenv("COMPOSESIREN_DSP_DATASET"); v && *v) d.dataset = v;
        if (const char* v = std::getenv("COMPOSESIREN_DSP_SKIP_SILENT"); v && *v) d.skipSilent = *v == '1';
        return d;
    }();
    return c;
}
} // namespace

DspChoice currentDspChoice()
{
    std::lock_guard g(lock());
    return current();
}

void setDspChoice(DspChoice choice)
{
    std::lock_guard g(lock());
    current() = std::move(choice);
}

} // namespace cs
