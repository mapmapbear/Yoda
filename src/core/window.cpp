#include "core/window.h"
// #include "Macros.h"
// #include "Error.h"
// #include "ObjectPython.h"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <atomic>
#include <cassert>

namespace Yoda {
class ApiCallbacks {
public:
  static void windowSizeCallback(GLFWwindow *pGlfwWindow, int width,
                                 int height) {
    // We also get here in case the window was minimized, so we need to ignore
    // it
    if (width == 0 || height == 0) {
      return;
    }

    Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
    if (pWindow != nullptr) {
      pWindow->resize(width, height); // Window callback is handled in here
    }
  }
  static void errorCallback(int errorCode, const char *pDescription) {
    // GLFW errors are always recoverable. Therefore we just log the error.
    // logError("GLFW error {}: {}", errorCode, pDescription);
  }

  static void keyboardCallback(GLFWwindow *pGlfwWindow, int key, int scanCode,
                               int action, int modifiers) {
    Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
    if (pWindow != nullptr) {
      pWindow->mpCallbacks->handleKeyboardEvent(key, action);
    }
  }

  static void mouseButtonCallback(GLFWwindow *pGlfwWindow, int button,
                                  int action, int modifiers) {
    Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
    if (pWindow != nullptr) {
      int state = glfwGetMouseButton(pGlfwWindow, GLFW_MOUSE_BUTTON_RIGHT);
      if (state == GLFW_PRESS)
      {
        double x, y;
        glfwGetCursorPos(pGlfwWindow, &x, &y);
        pWindow->mpCallbacks->handleMouseEvent(-2, x, y);
      }
    }
  }

  static void mouseMoveCallback(GLFWwindow *pGlfwWindow, double mouseX,
                                double mouseY) {
    Window *pWindow = (Window *)glfwGetWindowUserPointer(pGlfwWindow);
    if (pWindow != nullptr) {
      int state = glfwGetMouseButton(pGlfwWindow, GLFW_MOUSE_BUTTON_RIGHT);
      if (state != GLFW_RELEASE)
        pWindow->mpCallbacks->handleMouseEvent(state, mouseX, mouseY);
    }
  }

private:
  /**
   * GLFW reports modifiers inconsistently on different platforms.
   * To make modifiers consistent we check the key action and adjust
   * the modifiers due to changes from the current action.
   */
  static int fixGLFWModifiers(int modifiers, int key, int action) {
    int bit = 0;
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
      bit = GLFW_MOD_SHIFT;
    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
      bit = GLFW_MOD_CONTROL;
    if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
      bit = GLFW_MOD_ALT;
    return (action == GLFW_RELEASE) ? modifiers & (~bit) : modifiers | bit;
  }

  // static inline float2 calcMousePos(double xPos, double yPos, const float2&
  // mouseScale)
  // {
  //     float2 pos = float2(float(xPos), float(yPos));
  //     pos *= mouseScale;
  //     return pos;
  // }
};

static std::atomic<size_t> sWindowCount;

std::shared_ptr<Window> Window::create(const Desc &desc,
                                       ICallbacks *pCallbacks) {
  return std::shared_ptr<Window>(new Window(desc, pCallbacks));
}

Window::Window(const Desc &desc, ICallbacks *pCallbacks)
    : mDesc(desc), mpCallbacks(pCallbacks) {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  m_io = &ImGui::GetIO();
  m_io->ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  m_io->ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
  m_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
  // Multi-Viewport / Platform Windows
  // IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable

  ImGui::StyleColorsDark();
  ImGuiStyle &Style = ImGui::GetStyle();
  if (m_io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    Style.WindowRounding = 0.0f;
    Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }
  // Set error callback
  glfwSetErrorCallback(ApiCallbacks::errorCallback);
  // Init GLFW when first window is created.
  if (sWindowCount.fetch_add(1) == 0) {
    if (glfwInit() == GLFW_FALSE)
      assert(-1);
    // FALCOR_THROW("Failed to initialize GLFW.");
  }

  // Create the window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  uint32_t w = desc.width;
  uint32_t h = desc.height;

  if (desc.mode == WindowMode::Fullscreen) {
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    auto mon = glfwGetPrimaryMonitor();
    auto mod = glfwGetVideoMode(mon);
    w = mod->width;
    h = mod->height;
  } else if (desc.mode == WindowMode::Minimized) {
    // Start with window being invisible
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
  }

  if (desc.resizableWindow == false) {
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  }

  mpGLFWWindow = glfwCreateWindow(w, h, desc.title.c_str(), nullptr, nullptr);
  if (!mpGLFWWindow) {
    assert(-1);
    // FALCOR_THROW("Failed to create GLFW window.");
  }

  // Init handles
  mApiHandle = glfwGetWin32Window(mpGLFWWindow);

  updateWindowSize();

  glfwSetWindowUserPointer(mpGLFWWindow, this);

  // Set callbacks
  glfwSetWindowSizeCallback(mpGLFWWindow, ApiCallbacks::windowSizeCallback);
  glfwSetKeyCallback(mpGLFWWindow, ApiCallbacks::keyboardCallback);
  glfwSetMouseButtonCallback(mpGLFWWindow, ApiCallbacks::mouseButtonCallback);
  // glfwSetInputMode(mpGLFWWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glfwSetCursorPosCallback(mpGLFWWindow, ApiCallbacks::mouseMoveCallback);

  /*glfwSetScrollCallback(mpGLFWWindow, ApiCallbacks::mouseWheelCallback);
  glfwSetCharCallback(mpGLFWWindow, ApiCallbacks::charInputCallback);
  glfwSetDropCallback(mpGLFWWindow, ApiCallbacks::droppedFileCallback);*/

  if (desc.mode == WindowMode::Minimized) {
    // Iconify and show window to make it available if user clicks on it
    glfwIconifyWindow(mpGLFWWindow);
    glfwShowWindow(mpGLFWWindow);
  } else {
    glfwShowWindow(mpGLFWWindow);
    glfwFocusWindow(mpGLFWWindow);
  }
}

Window::~Window() {
  glfwDestroyWindow(mpGLFWWindow);

  // Shutdown GLFW when last window is destroyed.
  if (sWindowCount.fetch_sub(1) == 1)
    glfwTerminate();
}

void Window::updateWindowSize() {
  // Actual window size may be clamped to slightly lower than monitor resolution
  int32_t width, height;
  glfwGetWindowSize(mpGLFWWindow, &width, &height);
  setWindowSize(width, height);
}

void Window::setWindowSize(uint32_t width, uint32_t height) {
  assert(width > 0 && height > 0);

  mDesc.width = width;
  mDesc.height = height;
  // mMouseScale.x = 1.0f / (float)mDesc.width;
  // mMouseScale.y = 1.0f / (float)mDesc.height;
}

void Window::shutdown() { glfwSetWindowShouldClose(mpGLFWWindow, 1); }

bool Window::shouldClose() const { return glfwWindowShouldClose(mpGLFWWindow); }

void Window::resize(uint32_t width, uint32_t height) {
  glfwSetWindowSize(mpGLFWWindow, width, height);

  // In minimized mode GLFW reports incorrect window size
  if (mDesc.mode == WindowMode::Minimized) {
    setWindowSize(width, height);
  } else {
    updateWindowSize();
  }

  mpCallbacks->handleWindowSizeChange();
}

void Window::msgLoop() {
  // Samples often rely on a size change event as part of initialization
  // This would have happened from a WM_SIZE message when calling ShowWindow on
  // Win32
  
  //mpCallbacks->handleWindowSizeChange();

  while (!shouldClose()) {
    pollForEvents();
    mpCallbacks->handleRenderFrame();
  }
}

void Window::pollForEvents() { glfwPollEvents(); }

void Window::setWindowPos(int32_t x, int32_t y) {
  glfwSetWindowPos(mpGLFWWindow, x, y);
}

void Window::setWindowTitle(const std::string &title) {
  glfwSetWindowTitle(mpGLFWWindow, title.c_str());
}

} // namespace Yoda
