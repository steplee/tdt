#include <Eigen/LU>

#include "twindow.h"
//#include <spdlog/spdlog.h>
//#include <spdlog/fmt/ostr.h>

TWindow* TWindow::singleton = nullptr;

static void _reshapeFunc(GLFWwindow* window, int w, int h) {
  TWindow::get()->reshapeFunc(window, w,h);
}
static void _keyboardFunc(GLFWwindow* window, int key, int scancode, int action, int mods) {
  TWindow::get()->keyboardFunc(window, key, scancode, action, mods);
}
static void _clickFunc(GLFWwindow* window, int button, int action, int mods) {
  TWindow::get()->clickFunc(window, button, action, mods);
}
static void _motionFunc(GLFWwindow* window, double xpos, double ypos) {
  TWindow::get()->motionFunc(window, xpos, ypos);
}

TWindow::~TWindow() { TWindow::singleton = nullptr; }

TWindow::TWindow(int width, int height, bool headless)
  : width(width), height(height), headless(headless)
{


  singleton = this;

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    glfwTerminate();
  }

  std::string title = "TerraWorld";

  // NOTE: This doesn't affect non-default FBO!!!
  if (multisample) glfwWindowHint(GLFW_SAMPLES, 4);

  if (doubleBuffered)
    glfwWindowHint( GLFW_DOUBLEBUFFER, GL_TRUE );
  else
    glfwWindowHint( GLFW_DOUBLEBUFFER, GL_FALSE );

  window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
  if (window == NULL) {
    std::cerr << "Failed to open GLFW window. " << std::endl;
    glfwTerminate();
  }

  glfwGetWindowSize(window, &width, &height);

  glfwMakeContextCurrent(window);

  // Callback
  glfwSetWindowSizeCallback(window, &_reshapeFunc);
  glfwSetKeyCallback(window, &_keyboardFunc);
  glfwSetMouseButtonCallback(window, &_clickFunc);
  glfwSetCursorPosCallback(window, &_motionFunc);

  glewExperimental = true;  // This may be only true for linux environment.
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW." << std::endl;
    glfwTerminate();
  }

  if (multisample) glEnable(GL_MULTISAMPLE);
  else glDisable(GL_MULTISAMPLE);

  reshapeFunc(window, width, height);

}

void TWindow::startFrame() {
    glfwPollEvents();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void TWindow::endFrame() {
    if (doubleBuffered) glfwSwapBuffers(window);
    else glFlush();
}





void TWindow::reshapeFunc(GLFWwindow* window, int w, int h) {
  //spdlog::info("Resized to {} {}", w, h);

  width = w;
  height = h;

  for (const auto& io : ioListeners)
    io->reshapeFunc(w,h);
}
void TWindow::keyboardFunc(GLFWwindow* window, int key, int scancode, int action, int mods) {
  //spdlog::info("Key pressed: {} ({} {} {})", key, scancode, action, mods);

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    // Close window
    if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
  }

  for (const auto& io : ioListeners)
    io->keyboardFunc(key, scancode, action, mods);
}
void TWindow::clickFunc(GLFWwindow* window, int button, int action, int mods) {
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  //spdlog::info("Clicked: {} ({} {}, {} {})", button, action, mods, x,y);

  for (const auto& io : ioListeners)
    io->clickFunc(button, action, mods);
}

void TWindow::motionFunc(GLFWwindow* window, double xpos, double ypos) {
  //spdlog::info("MouseMotion: {} {}", xpos,ypos);

  for (const auto& io : ioListeners)
    io->motionFunc(xpos,ypos);
}










UsesIO::UsesIO() {
  if (TWindow::get() == nullptr) {
    //spdlog::error("Tried to construct an derived instance of UsesIO before creating TWindow. This is not allowed!");
    exit(1);
  }
  TWindow::get()->registerIoListener(this);
}
UsesIO::~UsesIO() {
  // Scene would break this if it were not for this check.
  // It's descturctor destroys TWindow (it has last ref to it), but UsesIO destructor then
  // tries to unregister itself!
  if (TWindow::get() != nullptr)
    TWindow::get()->unregisterIoListener(this);
}
