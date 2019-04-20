#include "demo.h"

#include "arrows.h"
#include "text.h"

#include <GL/glew.h>

demo::demo(int width, int height)
    : width_{width}
    , height_{height}
    , arrow_effect_{new arrows(width, height)}
{
    start_ = std::chrono::steady_clock::now();
}

demo::~demo() = default;

void demo::redraw()
{
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
    arrow_effect_->redraw(elapsed);
    text_effect_->redraw(elapsed);
}
