#include "geometry.hh"

//------------------------------------------------------------------------------
// Unit
//------------------------------------------------------------------------------

std::string geometry::units::degrees::name()  { return "deg";    }
std::string geometry::units::mercator::name() { return "mer";    }
std::string geometry::units::screen::name()   { return "px";     }
std::string geometry::units::tile::name()     { return "tile";   }
std::string geometry::units::zoom::name()     { return "zoom";   }
std::string geometry::units::world::name()    { return "world";  }
std::string geometry::units::norm::name()     { return "norm";   }

//------------------------------------------------------------------------------
// Point Conversion
//------------------------------------------------------------------------------

template <>
template <>
geometry::BasePoint<geometry::units::degrees, double>::operator BasePoint<units::mercator, double>() const {
    return BasePoint<units::mercator, double>(
            this->x.quantity / 180.0,
            std::log(std::tan((this->y.quantity  * M_PI/180.0)/2.0f + M_PI/4.0f)) / M_PI
        );
}

template <>
template <>
geometry::BasePoint<geometry::units::mercator, double>::operator BasePoint<units::degrees, double>() const {
    return BasePoint<units::degrees, double>(
        this->x.quantity * 180.0,
        std::atan(std::sinh(this->y.quantity * M_PI)) / (M_PI / 180.f)
        );
}
