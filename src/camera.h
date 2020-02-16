#include <Eigen/Core>
#include "twindow.h"

#include <sophus/se3.hpp>

using SE3 = Sophus::SE3f;
using SO3 = Sophus::SO3f;

struct CamSpec {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  bool ortho = false;
  float vfov;
  float hfov;
  float near, far;
  Eigen::Vector2i wh;

  inline float aspect_wh() { return ((float)wh(0)) / wh(1); }

  inline const Eigen::Matrix4f& P() { return proj; }

  // Perspective
  CamSpec(Eigen::Vector2i wh, float vFov, float near=.1, float far=100);
  // Ortho (todo)
  //CamSpec(Eigen::Vector2i wh, float vFov);

  private:
    Eigen::Matrix4f proj;
};

class InteractiveCamera : public UsesIO {

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    InteractiveCamera(CamSpec& spec);

    void step();

    void use();
    void unuse();

    //virtual void reshapeFunc(int w, int h) override;
    virtual void keyboardFunc(int key, int scancode, int action, int mods) override;
    virtual void clickFunc(int button, int action, int mods) override;
    virtual void motionFunc(double xpos, double ypos) override;

  protected:
    Eigen::Vector2f prev_mouse;
    Eigen::Vector2f left_mouse_vel = Eigen::Vector2f::Zero();
    Eigen::Vector2f right_mouse_vel = Eigen::Vector2f::Zero();
    Eigen::Vector2f mouse_acc = Eigen::Vector2f::Zero();
    bool left_down = false, right_down = false, just_down = false;

    CamSpec spec;
    SE3 pose;
};
