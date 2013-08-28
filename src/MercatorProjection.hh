#pragma once

#include <cstdint>

namespace mercator {

    typedef float Real;
    typedef float DegreeCoordinate;
    typedef float RadianCoordinate;
    typedef float MercatorCoordinate; // normalized mercator coordinate
    typedef float MercatorLength; // normalized mercator coordinate
    typedef float PixelLength; // normalized mercator coordinate
    typedef int32_t TileCoordinate;
    typedef int32_t Zoom;

/** Mercator Projection using a = 1/pi.
 @remarks
 If A = (-180,180)x(-85.05113,85.05113) (longitudes, latitudes), then
 MercatorProjection(A) = (-1,1)x(-1,1). Note the open interval to avoid
 problems.
 */
class MercatorProjection
{
public:

   static const int TILE_SIDE_PIXELS = 256; // unit: pixels/tile

   static void toMercator (DegreeCoordinate longitude,
                           DegreeCoordinate latitude,
                           MercatorCoordinate& mercator_x,
                           MercatorCoordinate& mercator_y );

   static void toLongitudeLatitude (MercatorCoordinate mercator_x,
                                    MercatorCoordinate mercator_y,
                                    DegreeCoordinate& longitude,
                                    DegreeCoordinate& latitude );

   static bool tileOfLongitudeLatitude(DegreeCoordinate longitude,
                                       DegreeCoordinate latitude,
                                       Zoom zoom,
                                       TileCoordinate &tile_x,
                                       TileCoordinate &tile_y);

   static bool tileOfMercatorCoordinates(MercatorCoordinate mercator_x,
                                         MercatorCoordinate mercator_y,
                                         int Zoom,
                                         TileCoordinate &tile_x,
                                         TileCoordinate &tile_y);

   static void tileBoundsInMercatorCoordinates(TileCoordinate tile_x,
                                               TileCoordinate tile_y,
                                               Zoom zoom,
                                               MercatorCoordinate &merc_x0,
                                               MercatorCoordinate &merc_y0,
                                               MercatorLength &merc_width,
                                               MercatorLength &merc_height);

   static Zoom computeZoomLevel(MercatorLength mercator_length,
                                PixelLength    window_length);

   static Real computeTexturePixelsPerPixel(Zoom zoom_level,
                                            MercatorLength source_mercator_length,
                                            PixelLength target_pixels);

   static Real mercatorTileSide(Zoom z);

   static void tileLowResVersion(TileCoordinate source_tile_x,
                                 TileCoordinate source_tile_y,
                                 Zoom source_zoom,
                                 Zoom target_zoom,
                                 TileCoordinate &target_tile_x,
                                 TileCoordinate &target_tile_y,
                                 Real &coef_x0,
                                 Real &coef_y0,
                                 Real &coef_width,
                                 Real &coef_height);

};

} // end of package namespace
