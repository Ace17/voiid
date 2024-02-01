#include "misc/file.h"
#include "picture.h"
#include "png.h"
#include <cstring> // memcpy

namespace
{
void blit(PictureView dst, Vec2i dstPos, PictureView src, Vec2i srcPos, Vec2i size)
{
  const auto bpp = 4;
  const uint8_t* srcPels = src.pixels + srcPos.y * src.stride * bpp + srcPos.x * bpp;
  uint8_t* dstPels = dst.pixels + dstPos.y * dst.stride * bpp + dstPos.x * bpp;

  for(int y = 0; y < size.y; ++y)
  {
    memcpy(dstPels, srcPels, size.x * bpp);
    srcPels += src.stride * bpp;
    dstPels += dst.stride * bpp;
  }
}
}

Picture addBorderToTiles(PictureView src, int cols, int rows)
{
  const int border = 1;

  const int srcTileWidth = src.dim.x / cols;
  const int srcTileHeight = src.dim.y / rows;
  const int dstTileWidth = srcTileWidth + border * 2;
  const int dstTileHeight = srcTileHeight + border * 2;

  Picture dst {};
  dst.dim.x = dstTileWidth * cols;
  dst.dim.y = dstTileHeight * rows;
  dst.stride = dst.dim.x;
  dst.pixels.resize(dst.dim.x * dst.dim.y * 4);

  for(int row = 0; row < rows; ++row)
  {
    for(int col = 0; col < cols; ++col)
    {
      const auto srcTileSize = Vec2i(srcTileWidth, srcTileHeight);
      const auto srcPos = Vec2i(col * srcTileWidth, row * srcTileHeight);
      const auto dstPos = Vec2i(col * dstTileWidth, row * dstTileHeight) + Vec2i(border, border);
      blit(dst, dstPos, src, srcPos, srcTileSize);
    }
  }

  return dst;
}

Picture loadPicture(String path)
{
  try
  {
    Picture pic;
    auto pngDataBuf = File::read(path);
    auto pngData = Span<const uint8_t>((uint8_t*)pngDataBuf.data(), (int)pngDataBuf.size());
    pic.pixels = decodePng(pngData, pic.dim.x, pic.dim.y);
    pic.stride = pic.dim.x * 4;

    auto const bpp = 4;

    std::vector<uint8_t> img(pic.dim.x * pic.dim.y * bpp);

    auto src = pic.pixels.data();
    auto dst = img.data() + bpp * pic.dim.x * pic.dim.y;

    // from glTexImage2D doc:
    // "The first element corresponds to the lower left corner of the texture image",
    // (e.g (u,v) = (0,0))
    for(int y = 0; y < pic.dim.y; ++y)
    {
      dst -= bpp * pic.dim.x;
      memcpy(dst, src, bpp * pic.dim.x);
      src += pic.stride;
    }

    Picture r;
    r.dim = pic.dim;
    r.stride = pic.dim.x;
    r.pixels = std::move(img);

    printf("[display] loaded texture '%.*s'\n", path.len, path.data);
    return r;
  }
  catch(std::exception const& e)
  {
    printf("[display] can't load texture '%.*s' (%s)\n", path.len, path.data, e.what());
    printf("[display] falling back on generated texture\n");

    Picture r {};
    r.dim = Vec2i(32, 32);
    r.pixels.resize(r.dim.x * r.dim.y * 4);

    for(int y = 0; y < r.dim.y; ++y)
    {
      for(int x = 0; x < r.dim.x; ++x)
      {
        r.pixels[(x + y * r.dim.x) * 4 + 0] = 0xff;
        r.pixels[(x + y * r.dim.x) * 4 + 1] = x < 16 ? 0xff : 0x00;
        r.pixels[(x + y * r.dim.x) * 4 + 2] = y < 16 ? 0xff : 0x00;
        r.pixels[(x + y * r.dim.x) * 4 + 3] = 0xff;
      }
    }

    return r;
  }
}

