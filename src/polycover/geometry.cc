#include "geometry.hh"

namespace polycover {


	namespace geometry {

		//------------------------------------------------------------------------------
		// Unit
		//------------------------------------------------------------------------------

		namespace units {

		std::string degrees::name() { return "deg"; }
		std::string mercator::name() { return "mer"; }
		std::string screen::name() { return "px"; }
		std::string tile::name() { return "tile"; }
		std::string zoom::name() { return "zoom"; }
		std::string world::name() { return "world"; }
		std::string norm::name() { return "norm"; }

		}

		//------------------------------------------------------------------------------
		// Point Conversion
		//------------------------------------------------------------------------------

		template <>
		template <>
		BasePoint<units::degrees, double>::operator BasePoint<units::mercator, double>() const {
			return BasePoint<units::mercator, double>(
				this->x.quantity / 180.0,
				std::log(std::tan((this->y.quantity  * M_PI / 180.0) / 2.0f + M_PI / 4.0f)) / M_PI
				);
		}

		template <>
		template <>
		BasePoint<units::mercator, double>::operator BasePoint<units::degrees, double>() const {
			return BasePoint<units::degrees, double>(
				this->x.quantity * 180.0,
				std::atan(std::sinh(this->y.quantity * M_PI)) / (M_PI / 180.f)
				);
		}

	} // namespace geometry

} // polycover namespace
