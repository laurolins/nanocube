#pragma once

struct TileKey {
    int x_tile, y_tile, zoom;
    TileKey(): x_tile(0), y_tile(0), zoom(0) {};
    TileKey(int x, int y, int z): x_tile(x), y_tile(y), zoom(z) {};
};

inline bool operator==(const TileKey &k1, const TileKey &k2)
{
    return k1.x_tile == k2.x_tile &&
        k1.y_tile == k2.y_tile &&
        k1.zoom == k2.zoom;
}

inline size_t hash_value(const TileKey &k)
{
    return k.x_tile + (((size_t) k.y_tile) << 32);
}

inline bool operator<(const TileKey &v1, const TileKey &v2) {
    if (v1.zoom < v2.zoom) return true;
    if (v2.zoom < v1.zoom) return false;
    if (v1.x_tile < v2.x_tile) return true;
    if (v2.x_tile < v1.x_tile) return false;
    if (v1.y_tile < v2.y_tile) return true;
    if (v2.y_tile < v1.y_tile) return false;
    return false;
}
