#pragma once

#include "base/geom.h"

struct RenderMesh;
struct PictureView;
struct Camera;

struct IGraphicsBackend
{
  virtual ~IGraphicsBackend() = default;

  virtual void setFullscreen(bool fs) = 0;
  virtual void setHdr(bool enable) = 0;
  virtual void setFsaa(bool enable) = 0;
  virtual void setCaption(String caption) = 0;
  virtual void setAmbientLight(float ambientLight) = 0;
  virtual void setLight(int idx, Vector3f pos, Vector3f color) = 0;
  virtual void readPixels(Span<uint8_t> dstRgbPixels) = 0;
  virtual void enableGrab(bool enable) = 0;

  virtual uintptr_t uploadTextureToGPU(PictureView pic) = 0;
  virtual void setTextureNoRepeat(uintptr_t texture) = 0;

  virtual void uploadVerticesToGPU(RenderMesh& mesh) = 0;

  // draw functions
  virtual void beginDraw() = 0;
  virtual void endDraw() = 0;

  virtual void pushMesh(Rect3f where, Quaternion orientation, Camera const& camera, RenderMesh& model, bool blinking, bool depthtest) = 0;
};

struct Camera
{
  Vector3f pos;
  Quaternion dir;
};

