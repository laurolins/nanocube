#include "MercatorProjection.hh"

#include <cmath>
#include <cassert>

#include <iostream>

namespace mercator {

//
//Reproject the coordinates to the Mercator projection:
//        x = lon
//        y = log(tan(lat) + sec(lat))
//
//        (lat and lon are in radians)
//
//    Transform range of x and y to 0 - 1 and shift origin to top left corner:
//        x = (1 + (x / π)) / 2
//        y = (1 - (y / π)) / 2
//    Calculate the number of tiles across the map, n, using 2zoom
//    Multiply x and y by n. Round results down to give tilex and tiley.
//


void MercatorProjection::toMercator(
        DegreeCoordinate longitude,
        DegreeCoordinate latitude,
        MercatorCoordinate& mercator_x,
        MercatorCoordinate& mercator_y )
{
    RadianCoordinate lat_rad = latitude * (M_PI / 180.f);

    // if a was 1.0 then
    // mercator_x = longitude * (M_PI / 180.f);
    // mercator_y = logf(tanf(lat_rad/2.0 + M_PI/4));

    // as a = 1/pi then
    mercator_x = longitude / 180.f;
    mercator_y = logf(tanf(lat_rad/2.0f + M_PI/4.0f)) / M_PI;
}

void MercatorProjection::toLongitudeLatitude (
        MercatorCoordinate mercator_x,
        MercatorCoordinate mercator_y,
        DegreeCoordinate& longitude,
        DegreeCoordinate& latitude )
{
    // if a was 1.0 then
    // longitude = mercator_x / (M_PI / 180.f);
    // latitude = atanf(sinhf(mercator_y)) / (M_PI / 180.f);

    // as a = 1/pi then
    longitude = mercator_x * 180.f;
    latitude  = atanf(sinhf(mercator_y * M_PI)) / (M_PI / 180.f);
}


bool MercatorProjection::tileOfMercatorCoordinates(
        MercatorCoordinate mercator_x,
        MercatorCoordinate mercator_y,
        Zoom zoom,
        TileCoordinate& tile_x,
        TileCoordinate& tile_y )
{
    //    if (    zoom > 21           ||
    //            mercator_x <= -1.0f ||
    //            mercator_x >=  1.0f ||
    //            mercator_y <= -1.0f ||
    //            mercator_y >=  1.0f      )
    //    {
    //        tile_x = -1;
    //        tile_y = -1;
    //        return false;
    //    }

    int tiles_by_side = 1 << zoom;
    tile_x = (int) ((1.0f + mercator_x)/2.0f * tiles_by_side);
    tile_y = (int) ((1.0f + mercator_y)/2.0f * tiles_by_side);

    return true;
}

void MercatorProjection::tileBoundsInMercatorCoordinates(
        TileCoordinate tile_x,
        TileCoordinate tile_y,
        Zoom zoom,
        MercatorCoordinate &merc_x0,
        MercatorCoordinate &merc_y0,
        MercatorLength &merc_width,
        MercatorLength &merc_height)
{
    int tiles_by_side = 1 << zoom;

    merc_width  =  2.0f / tiles_by_side;
    merc_height =  2.0f / tiles_by_side;

    merc_x0     = -1.0f + tile_x * merc_width;
    merc_y0     = -1.0f + tile_y * merc_height;
}

bool MercatorProjection::tileOfLongitudeLatitude(
        DegreeCoordinate longitude,
        DegreeCoordinate latitude,
        Zoom zoom,
        TileCoordinate& tile_x,
        TileCoordinate& tile_y )
{
    //    if (    zoom > 21                 ||
    //            longitude  <= -180.0f     ||
    //            longitude  >=  180.0f     ||
    //            latitude   <=  -85.05113f ||
    //            latitude   >=   85.05113f)
    //    {
    //        tile_x = -1;
    //        tile_y = -1;
    //        return false;
    //    }

    MercatorCoordinate mercator_x, mercator_y;
    MercatorProjection::toMercator(longitude, latitude, mercator_x, mercator_y);
    Real tiles_by_side = powf(2.0f,zoom);
    tile_x = (int) ((1.0f + mercator_x)/2.0f * tiles_by_side);
    tile_y = (int) ((1.0f + mercator_y)/2.0f * tiles_by_side);
    return true;
}

/**
          * Tile size in normalized mercator coordinates at zoom level "z"
          */
Real MercatorProjection::mercatorTileSide(Zoom z)
{
    return 2.0 / (1 << z); // 2 / 2 ^ -i;
}

void MercatorProjection::tileLowResVersion(TileCoordinate source_tile_x,
                                           TileCoordinate source_tile_y,
                                           Zoom source_zoom,
                                           Zoom target_zoom,
                                           TileCoordinate &target_tile_x,
                                           TileCoordinate &target_tile_y,
                                           Real &coef_x0,
                                           Real &coef_y0,
                                           Real &coef_width,
                                           Real &coef_height)
{
    assert(source_zoom > target_zoom);

    // the target tile will fit
    MercatorCoordinate source_merc_x0, source_merc_y0;
    MercatorLength     source_merc_width, source_merc_height;
    tileBoundsInMercatorCoordinates(source_tile_x,
                                    source_tile_y,
                                    source_zoom,
                                    source_merc_x0,
                                    source_merc_y0,
                                    source_merc_width,
                                    source_merc_height);

    tileOfMercatorCoordinates(source_merc_x0 + source_merc_width/2.0,
                              source_merc_y0 + source_merc_height/2.0,
                              target_zoom,
                              target_tile_x,
                              target_tile_y);

    Real target_merc_x0, target_merc_y0, target_merc_width, target_merc_height;
    tileBoundsInMercatorCoordinates(target_tile_x,
                                    target_tile_y,
                                    target_zoom,
                                    target_merc_x0,
                                    target_merc_y0,
                                    target_merc_width,
                                    target_merc_height);

    coef_x0 = (source_merc_x0-target_merc_x0)/target_merc_width;
    coef_y0 = (source_merc_y0-target_merc_y0)/target_merc_height;

    coef_width = source_merc_width/target_merc_width;
    coef_height = source_merc_height/target_merc_height;
}

/**
          * Assuming "source mercator length" will be mapped into "target pixel length"
          * What is the zoom level we should use for the tiles so that the number of
          * texture pixels using the 256x256 pixel tiles of OSM is at least
          * the "target pixel length".
          */
int MercatorProjection::computeZoomLevel(
        MercatorLength source_mercator_length,
        PixelLength    target_pixels)
{


    // if mercator units is mu then the number below is in   mu . pixels/tile
    Real aux = source_mercator_length * TILE_SIDE_PIXELS;

    //
    // Search for smallest z which yelds:
    //     texture_pixels >= target_pixels
    //
    int left  = 0;
    int right = 17;
    while (left < right)
    {
        int z = (left + right)/2; // Obs. (z == left) may be true, and always (z != right)

        Real texture_pixels = aux / MercatorProjection::mercatorTileSide(z); // number in mu . pixels

        if (texture_pixels > target_pixels)
        {
            right = z;

            if (right == left + 1)
            {
                Real left_texture_pixels = aux / MercatorProjection::mercatorTileSide(left); // number in mu . pixels

                Real abs_diff_left  = fabs(left_texture_pixels-target_pixels);
                Real abs_diff_right = fabs(texture_pixels-target_pixels);

                if (abs_diff_left < abs_diff_right)
                    right = left;
                else
                    left  = right;
            }

        }
        else
        {
            left  = z;

            if (right == left + 1)
            {
                Real right_texture_pixels = aux / MercatorProjection::mercatorTileSide(right); // number in mu . pixels

                Real abs_diff_left  = fabs(texture_pixels-target_pixels);
                Real abs_diff_right = fabs(right_texture_pixels-target_pixels);

                if (abs_diff_left < abs_diff_right)
                    right = left;
                else
                    left  = right;
            }

        }
    }

    //    left -= 1;
    //    if (left < 0)
    //        left = 0;

    // std::cout << "computeZoomLevel:  merc. length = " << source_mercator_length << "  target pixels = " << target_pixels << "  zoom: " << left << std::endl;

    return left;


    //return left-1; // could be right they are the same in the end

}

/**
          * If we use zoom level "zoom_level" and match "source_mercator_length"
          * with "target_pixels" we will return this amount of texture pixel per target pixel.
          * Note that >= 1 is desired.
          */
Real MercatorProjection::computeTexturePixelsPerPixel(
        Zoom           zoom_level,
        MercatorLength source_mercator_length,
        PixelLength    target_pixels)
{
    Real texture_pixels =
            source_mercator_length
            * TILE_SIDE_PIXELS
            / MercatorProjection::mercatorTileSide(zoom_level); // number in mu . pixels

    return texture_pixels / target_pixels;
}

};
