#include "cairo.hpp"

#include <cairomm/context.h>

constexpr static double M_PI_H = M_PI / 2.0;
constexpr static double M_PI_3_2 = M_PI * 3.0 / 2.0;

void CairoUtil::PathRoundedRect(const Cairo::RefPtr<Cairo::Context> &cr, double x, double y, double w, double h, double r) {
    const double degrees = M_PI / 180.0;

    cr->begin_new_sub_path();
    cr->arc(x + w - r, y + r, r, -M_PI_H, 0);
    cr->arc(x + w - r, y + h - r, r, 0, M_PI_H);
    cr->arc(x + r, y + h - r, r, M_PI_H, M_PI);
    cr->arc(x + r, y + r, r, M_PI, M_PI_3_2);
    cr->close_path();
}