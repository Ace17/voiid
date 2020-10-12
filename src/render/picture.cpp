#include "misc/file.h"
#include "picture.h"
#include "png.h"
#include <cstring> // memcpy

namespace
{
void blit(PictureView dst, Vector2i dstPos, PictureView src, Vector2i srcPos, Size2i size)
{
  const auto bpp = 4;
  const uint8_t* srcPels = src.pixels + srcPos.y * src.stride * bpp + srcPos.x * bpp;
  uint8_t* dstPels = dst.pixels + dstPos.y * dst.stride * bpp + dstPos.x * bpp;

  for(int y = 0; y < size.height; ++y)
  {
    memcpy(dstPels, srcPels, size.width * bpp);
    srcPels += src.stride * bpp;
    dstPels += dst.stride * bpp;
  }
}
}

Picture addBorderToTiles(PictureView src, int cols, int rows)
{
  const int border = 1;

  const int srcTileWidth = src.dim.width / cols;
  const int srcTileHeight = src.dim.height / rows;
  const int dstTileWidth = srcTileWidth + border * 2;
  const int dstTileHeight = srcTileHeight + border * 2;

  Picture dst {};
  dst.dim.width = dstTileWidth * cols;
  dst.dim.height = dstTileHeight * rows;
  dst.stride = dst.dim.width;
  dst.pixels.resize(dst.dim.width * dst.dim.height * 4);

  for(int row = 0; row < rows; ++row)
  {
    for(int col = 0; col < cols; ++col)
    {
      const auto srcTileSize = Size2i(srcTileWidth, srcTileHeight);
      const auto srcPos = Vector2i(col * srcTileWidth, row * srcTileHeight);
      const auto dstPos = Vector2i(col * dstTileWidth, row * dstTileHeight) + Vector2i(border, border);
      blit(dst, dstPos, src, srcPos, srcTileSize);
    }
  }

  return dst;
}

Picture loadPicture(const char* path)
{
  try
  {
    Picture pic;
    auto pngDataBuf = File::read(path);
    auto pngData = Span<const uint8_t>((uint8_t*)pngDataBuf.data(), (int)pngDataBuf.size());
    pic.pixels = decodePng(pngData, pic.dim.width, pic.dim.height);
    pic.stride = pic.dim.width * 4;

    auto const bpp = 4;

    vector<uint8_t> img(pic.dim.width * pic.dim.height * bpp);

    auto src = pic.pixels.data();
    auto dst = img.data() + bpp * pic.dim.width * pic.dim.height;

    // from glTexImage2D doc:
    // "The first element corresponds to the lower left corner of the texture image",
    // (e.g (u,v) = (0,0))
    for(int y = 0; y < pic.dim.height; ++y)
    {
      dst -= bpp * pic.dim.width;
      memcpy(dst, src, bpp * pic.dim.width);
      src += pic.stride;
    }

    Picture r;
    r.dim = pic.dim;
    r.stride = pic.dim.width;
    r.pixels = std::move(img);

    return r;
  }
  catch(std::exception const& e)
  {
    printf("[display] can't load texture: %s\n", e.what());
    printf("[display] falling back on generated texture\n");

    Picture r {};
    r.dim = Size2i(32, 32);
    r.pixels.resize(r.dim.width * r.dim.height * 4);

    for(int y = 0; y < r.dim.height; ++y)
    {
      for(int x = 0; x < r.dim.width; ++x)
      {
        r.pixels[(x + y * r.dim.width) * 4 + 0] = 0xff;
        r.pixels[(x + y * r.dim.width) * 4 + 1] = x < 16 ? 0xff : 0x00;
        r.pixels[(x + y * r.dim.width) * 4 + 2] = y < 16 ? 0xff : 0x00;
        r.pixels[(x + y * r.dim.width) * 4 + 3] = 0xff;
      }
    }

    return r;
  }
}

