#include "tinygltf/json.hpp"

#include "twindow.h"
#include "camera.h"
#include "scene_graph.h"

static void renderGizmo() {
  glLineWidth(4.);
  glBegin(GL_LINES);
  glColor4f(1.0,0.f,0.f,1.0f);
  glVertex3f(0.f,0.f,0.f);
  glVertex3f(1.f,0.f,0.f);
  glColor4f(0.0,1.f,0.f,1.0f);
  glVertex3f(0.f,0.f,0.f);
  glVertex3f(0.f,1.f,0.f);
  glColor4f(0.0,0.f,1.f,1.0f);
  glVertex3f(0.f,0.f,0.f);
  glVertex3f(0.f,0.f,1.f);
  glEnd();
}

int main() {

  TWindow window(800,800, true);

  CamSpec spec({800,800}, M_PI/4.);
  InteractiveCamera cam(spec);

  SceneGraph sg;

  glEnable(GL_DEPTH_TEST);

  for (int i=0; i<50000; i++) {
    window.startFrame();

    cam.step();
    cam.use();

    renderGizmo();

    RenderState rs;
    rs.view_proj = cam.spec().P() * cam.pose().matrix();
    sg.render(rs);

    cam.unuse();

    window.endFrame();
  }

  return 0;
}
