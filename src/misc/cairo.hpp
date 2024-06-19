#pragma once

#include <cairomm/context.h>

namespace CairoUtil {
void PathRoundedRect(const Cairo::RefPtr<Cairo::Context> &cr, double x, double y, double w, double h, double r);
} // namespace CairoUtil
