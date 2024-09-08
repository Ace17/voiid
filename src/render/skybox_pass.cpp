#include "base/matrix4.h"
#include "engine/graphics_backend.h"
#include "renderpass.h"
#include <vector>

namespace
{
struct CubeVertex
{
  float x, y, z;
};

const CubeVertex UnitCube[] =
{
  { -1, -1, -1, },
  { -1, -1, 1, },
  { -1, 1, 1, },
  { 1, 1, -1, },
  { -1, -1, -1, },
  { -1, 1, -1, },
  { 1, -1, 1, },
  { -1, -1, -1, },
  { 1, -1, -1, },
  { 1, 1, -1, },
  { 1, -1, -1, },
  { -1, -1, -1, },
  { -1, -1, -1, },
  { -1, 1, 1, },
  { -1, 1, -1, },
  { 1, -1, 1, },
  { -1, -1, 1, },
  { -1, -1, -1, },
  { -1, 1, 1, },
  { -1, -1, 1, },
  { 1, -1, 1, },
  { 1, 1, 1, },
  { 1, -1, -1, },
  { 1, 1, -1, },
  { 1, -1, -1, },
  { 1, 1, 1, },
  { 1, -1, 1, },
  { 1, 1, 1, },
  { 1, 1, -1, },
  { -1, 1, -1, },
  { 1, 1, 1, },
  { -1, 1, -1, },
  { -1, 1, 1, },
  { 1, 1, 1, },
  { -1, 1, 1, },
  { 1, -1, 1 },
};

struct SkyboxPass : RenderPass
{
  SkyboxPass(IGraphicsBackend* backend, const Camera* camera) : backend(backend), m_camera(camera)
  {
    m_shader = backend->createGpuProgram("skybox", false);
    m_vb = backend->createVertexBuffer();
    m_vb->upload(UnitCube, sizeof UnitCube);
  }

  void execute(FrameBuffer dst) override
  {
    backend->setRenderTarget(dst.fb);

    backend->useGpuProgram(m_shader.get());

    auto const forward = m_camera->dir.rotate(Vec3f(1, 0, 0));
    auto const up = m_camera->dir.rotate(Vec3f(0, 0, 1));

    auto const target = forward;
    auto const view = ::lookAt(Vec3f(0, 0, 0), target, up);
    auto const pos = ::translate({});

    static const float fovy = (float)((60.0f / 180) * PI);
    static const float near_ = 0.1f;
    static const float far_ = 1000.0f;
    const auto perspective = ::perspective(fovy, 16.0 / 9.0, near_, far_);

    // Must match the uniform block in skybox.frag
    struct MyUniformBlock
    {
      Matrix4f MVP;
    };

    MyUniformBlock ub {};
    ub.MVP = perspective * view * pos;
    ub.MVP = transpose(ub.MVP);

    backend->setUniformBlock(&ub, sizeof ub);

    backend->useVertexBuffer(m_vb.get());
    backend->enableVertexAttribute(0, 3, sizeof(CubeVertex), OFFSET(CubeVertex, x));

    backend->draw(Span<const CubeVertex>(UnitCube).len);
  }

  IGraphicsBackend* const backend;
  std::unique_ptr<IGpuProgram> m_shader;
  std::unique_ptr<IVertexBuffer> m_vb;
  const Camera* const m_camera;
};
}

std::unique_ptr<RenderPass> CreateSkyboxPass(IGraphicsBackend* backend, const Camera* camera)
{
  return std::make_unique<SkyboxPass>(backend, camera);
}

