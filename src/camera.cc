#include "camera.h"
using namespace Eigen;

static Eigen::Matrix4f MakeInfReversedZProjRH(float fovY_radians, float aspectWbyH, float znear, float zfar)
{
  // https://www.khronos.org/opengl/wiki/GluPerspective_code

  Eigen::Matrix4f out;
  out.setZero();

  float ymax, xmax;
  float temp, temp2, temp3, temp4;
  ymax = znear * std::tan(fovY_radians*.5);
  xmax = ymax * aspectWbyH;

  float left = -xmax;
  float right = xmax;
  float bottom = -ymax;
  float top = ymax;

  out(0,0) = -2 * znear / (right-left);

  out(1,1) = 2 * znear / (top-bottom);

  out(0,2) = (right+left) / (right-left);
  out(1,2) = (top+bottom) / (top-bottom);
  out(2,2) = -(-zfar-znear) / (zfar-znear);
  out(3,2) = 1.0;

  out(2,3) = (-2*znear*zfar) / (zfar-znear);
  return out;
}

CamSpec::CamSpec(Eigen::Vector2i wh, float vFov, float near, float far)
  : vfov(vFov), wh(wh), near(near), far(far)
{
  hfov = vFov * aspect_wh();
  proj = MakeInfReversedZProjRH(vFov, aspect_wh(), near, far);
}

InteractiveCamera::InteractiveCamera(CamSpec& spec) 
  : spec(spec)
{
  pose = SE3::transZ(4) * SE3::rotX(M_PI);
}


void InteractiveCamera::step() {
  if (left_down)
    left_mouse_vel += mouse_acc;
  left_mouse_vel = left_mouse_vel - left_mouse_vel * .1;
  if (right_down)
    right_mouse_vel += mouse_acc;
  right_mouse_vel = right_mouse_vel - right_mouse_vel * .1;

  if (!left_mouse_vel.isZero(1e-3) and !just_down) {
    float speed = .01;
    Vector3f y_plus = pose.rotationMatrix().row(0);
    pose.so3() = pose.so3() * SO3::exp(y_plus * left_mouse_vel(1) * speed);
    //Vector3f up = Vector3f::UnitZ();
    //pose.so3() = pose.so3() * SO3::exp(up * mouse_vel(0) * speed);
    pose.so3() = pose.so3() * SO3::rotZ(left_mouse_vel(0) * speed);
  }

  if (!right_mouse_vel.isZero(1e-3) and !just_down) {
    pose = pose.inverse();
    pose.translation() = pose.translation() + pose.translation() * right_mouse_vel(1) * .01;
    pose = pose.inverse();
  }

}

void InteractiveCamera::use() {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadMatrixf(spec.P().data());
  //if (yFlipped) glScalef(1., -1., 1.);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadMatrixf(pose.matrix().data());
}
void InteractiveCamera::unuse() {
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}


void InteractiveCamera::keyboardFunc(int key, int scancode, int action, int mods) {
  exit(0);
}
void InteractiveCamera::clickFunc(int button, int action, int mods) {
  if (action == GLFW_PRESS) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) left_down = true;
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) right_down = true;
    just_down = true;
    mouse_acc.setZero();
  }
  if (action == GLFW_RELEASE) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) left_down = false;
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) right_down = false;
    mouse_acc.setZero();
  }

}
void InteractiveCamera::motionFunc(double xpos, double ypos) {
  Vector2f dxy = Vector2f{xpos,ypos} - prev_mouse;
  float t = .1;

  if ((right_down or left_down) and not just_down)
    mouse_acc = dxy * t;

  prev_mouse = {xpos, ypos};
  just_down = false;

}
