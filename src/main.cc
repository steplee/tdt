#include "tinygltf/json.hpp"

#include "window.h"

static void renderTestTriangle() {
  glBegin(GL_TRIANGLES);
  glColor4f(1.0,0.f,0.f,1.0f);
  glVertex3f(0.5f,0.f,0.f);
  glColor4f(0.0,1.f,0.f,1.0f);
  glVertex3f(0.f,.5f,0.f);
  glColor4f(0.0,0.f,1.f,1.0f);
  glVertex3f(0.f,0.f,0.5f);
  glEnd();
}

int main() {

  TWindow window(800,800, true);

  for (int i=0; i<500; i++) {
    window.startFrame();
    renderTestTriangle();
    window.endFrame();
  }

  return 0;
}
