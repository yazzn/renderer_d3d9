# Direct3D 9 Renderer

This is a simple Direct3D 9 renderer that is able to draw deferred using render lists. For text rendering a forked version of CD3DFont was used.

# Quick guide

```cpp
#include "renderer.hpp"

RendererPtr renderer;
RenderListPtr render_list;

// init once
{
  renderer = std::make_shared<Renderer>(device, 1536);
  render_list = renderer->make_render_list();
  renderer->draw_filled_rect(render_list, { 100.f, 300.f, 200.f, 150.f }, 0xff00ff00); // adds vertices to render_list
}

// in your render loop
{
  device->Clear(...);
  device->BeginScene(...);

  renderer->begin(); // sets necessary render states

  renderer->draw_filled_rect({ 100.f, 100.f, 200.f, 150.f }, 0xffff0000); // adds vertices to internal render list

  renderer->draw(render_list); // draws render_list
  renderer->draw(); // draws internal render list and resets its vertices
  renderer->end(); // restores previous render states

  device->EndScene(...);
  device->Present(...);
}
