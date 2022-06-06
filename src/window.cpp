#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <iostream>

#include "window.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

EM_JS(int, getCanvasWidth, (), {
   return window.innerWidth;
});

EM_JS(int, getCanvasHeight, (), {
   return window.innerHeight;
});

EM_JS(float, getDevicePixelRatio, (), {
   return window.devicePixelRatio;
});

EM_JS(float, getBrowserScrollWheelSensitivity, (), {
   if (navigator.userAgent.toLowerCase().indexOf('firefox') > -1) {
      return -1.0;
   }
   else
   {
      return -0.02;
   }
});
#endif

Window::Window(const std::string& title)
   : mWindow(nullptr)
   , mWidthOfWindowInPix(0)
   , mHeightOfWindowInPix(0)
   , mWidthOfFramebufferInPix(0)
   , mHeightOfFramebufferInPix(0)
   , mTitle(title)
#ifndef __EMSCRIPTEN__
   , mIsFullScreen(false)
#endif
   , mKeys()
   , mProcessedKeys()
   , mMouseMoved(false)
   , mFirstCursorPosCallback(true)
   , mLastCursorXPos(0.0)
   , mLastCursorYPos(0.0)
   , mCursorXOffset(0.0)
   , mCursorYOffset(0.0)
   , mScrollWheelMoved(false)
   , mScrollYOffset(0.0)
#ifdef __EMSCRIPTEN__
   , mScrollWheelSensitivity(0.0f)
#endif
#ifndef __EMSCRIPTEN__
   , mMultisampleFBO(0)
   , mMultisampleTexture(0)
   , mMultisampleRBO(0)
   , mNumOfSamples(1)
#endif
{

}

Window::~Window()
{
#ifndef __EMSCRIPTEN__
   glDeleteFramebuffers(1, &mMultisampleFBO);
   glDeleteTextures(1, &mMultisampleTexture);
   glDeleteRenderbuffers(1, &mMultisampleRBO);
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

   if (mWindow)
   {
      glfwTerminate();
      mWindow = nullptr;
   }
}

bool Window::initialize()
{
   if (!glfwInit())
   {
      std::cout << "Error - Window::initialize - Failed to initialize GLFW" << "\n";
      return false;
   }

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
#ifdef __EMSCRIPTEN__
   glfwWindowHint(GLFW_SAMPLES, 8);
#endif

#ifdef __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

   float devicePixelRatio = 1.0f;
   int width = 1280;
   int height = 720;

#ifdef __EMSCRIPTEN__
   devicePixelRatio = getDevicePixelRatio();
   width = getCanvasWidth() * devicePixelRatio;
   height = getCanvasHeight() * devicePixelRatio;
   mScrollWheelSensitivity = getBrowserScrollWheelSensitivity();
#endif

   mWindow = glfwCreateWindow(width, height, mTitle.c_str(), nullptr, nullptr);
   if (!mWindow)
   {
      std::cout << "Error - Window::initialize - Failed to create the GLFW window" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   glfwMakeContextCurrent(mWindow);

   enableCursor(true);

#ifndef __EMSCRIPTEN__
   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
   {
      std::cout << "Error - Window::initialize - Failed to load pointers to OpenGL functions using GLAD" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }
#endif

   glEnable(GL_CULL_FACE);

   glfwGetFramebufferSize(mWindow, &mWidthOfFramebufferInPix, &mHeightOfFramebufferInPix);

#ifndef __EMSCRIPTEN__
   if (!configureAntiAliasingSupport())
   {
      std::cout << "Error - Window::initialize - Failed to configure anti aliasing support" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }
#endif

   setInputCallbacks();

   updateBufferAndViewportSizes(mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);

#ifndef __EMSCRIPTEN__
   // TODO: The ImGui window is properly scaled in the browser, but in the desktop it looks huge
   //       We need to figure out how to scale things properly in the desktop
   //       Once that's done, the calls to AddFontFromFileTTF and ScaleAllSizes should be done in the desktop too
   //devicePixelRatio = static_cast<float>(mWidthOfFramebufferInPix) / static_cast<float>(mWidthOfWindowInPix);
#endif

   // Initialize ImGui
   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   io.IniFilename = nullptr;
#ifdef __EMSCRIPTEN__
   io.Fonts->AddFontFromFileTTF("resources/fonts/Cousine-Regular.ttf", 12 * devicePixelRatio);
#endif

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();
#ifdef __EMSCRIPTEN__
   ImGui::GetStyle().ScaleAllSizes(devicePixelRatio);
#endif

   // Setup Platform/Renderer bindings
   ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
#ifdef __EMSCRIPTEN__
   ImGui_ImplOpenGL3_Init("#version 300 es");
#else
   ImGui_ImplOpenGL3_Init("#version 330 core");
#endif

   return true;
}

bool Window::shouldClose() const
{
   int windowShouldClose = glfwWindowShouldClose(mWindow);

   if (windowShouldClose == 0)
   {
      return false;
   }
   else
   {
      return true;
   }
}

void Window::setShouldClose(bool shouldClose)
{
   glfwSetWindowShouldClose(mWindow, shouldClose);
}

void Window::swapBuffers()
{
   glfwSwapBuffers(mWindow);
}

void Window::pollEvents()
{
   glfwPollEvents();
}

unsigned int Window::getWidthOfWindowInPix() const
{
   return mWidthOfWindowInPix;
}

unsigned int Window::getHeightOfWindowInPix() const
{
   return mHeightOfWindowInPix;
}

unsigned int Window::getWidthOfFramebufferInPix() const
{
   return mWidthOfFramebufferInPix;
}

unsigned int Window::getHeightOfFramebufferInPix() const
{
   return mHeightOfFramebufferInPix;
}

#ifndef __EMSCRIPTEN__
bool Window::isFullScreen() const
{
   return mIsFullScreen;
}

void Window::setFullScreen(bool fullScreen)
{
   if (fullScreen)
   {
      const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
      mWidthOfWindowInPix = mode->width;
      mHeightOfWindowInPix = mode->height;
      glfwSetWindowMonitor(mWindow, glfwGetPrimaryMonitor(), 0, 0, mWidthOfWindowInPix, mHeightOfWindowInPix, GLFW_DONT_CARE);
   }
   else
   {
      mWidthOfWindowInPix = 1280;
      mHeightOfWindowInPix = 720;
      glfwSetWindowMonitor(mWindow, NULL, 20, 50, mWidthOfWindowInPix, mHeightOfWindowInPix, GLFW_DONT_CARE);
   }

   mIsFullScreen = fullScreen;
}
#endif

bool Window::keyIsPressed(int key) const
{
   return mKeys.test(key);
}

bool Window::keyHasBeenProcessed(int key) const
{
   return mProcessedKeys.test(key);
}

void Window::setKeyAsProcessed(int key)
{
   mProcessedKeys.set(key);
}

bool Window::mouseMoved() const
{
   ImGuiIO& io = ImGui::GetIO();
   if (io.WantCaptureMouse)
   {
      // The cursor is hovering over an imgui window, so we tell the game that the mouse didn't move
      return false;
   }

   return mMouseMoved;
}

void Window::resetMouseMoved()
{
   mMouseMoved = false;
}

void Window::resetFirstMove()
{
   mFirstCursorPosCallback = true;
}

float Window::getCursorXOffset() const
{
   return mCursorXOffset;
}

float Window::getCursorYOffset() const
{
   return mCursorYOffset;
}

void Window::enableCursor(bool enable)
{
   glfwSetInputMode(mWindow, GLFW_CURSOR, enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

bool Window::isMouseButtonPressed(int button)
{
   ImGuiIO& io = ImGui::GetIO();
   if (io.WantCaptureMouse)
   {
      // The cursor is hovering over an imgui window, so we tell the game that no mouse buttons were pressed
      return false;
   }

   return (glfwGetMouseButton(mWindow, button) == GLFW_PRESS);
}

bool Window::scrollWheelMoved() const
{
   ImGuiIO& io = ImGui::GetIO();
   if (io.WantCaptureMouse)
   {
      // The cursor is hovering over an imgui window, so we tell the game that the scroll wheel didn't move
      mScrollWheelMoved = false;
      return false;
   }

   return mScrollWheelMoved;
}

void Window::resetScrollWheelMoved()
{
   mScrollWheelMoved = false;
}

float Window::getScrollYOffset() const
{
   return mScrollYOffset;
}

void Window::setViewport()
{
   glViewport(mLowerLeftCornerOfViewportXInPix, mLowerLeftCornerOfViewportYInPix, mWidthOfViewportInPix, mHeightOfViewportInPix);
}

void Window::setInputCallbacks()
{
   glfwSetWindowUserPointer(mWindow, this);

   auto framebufferSizeCallback = [](GLFWwindow* window, int width, int height)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->framebufferSizeCallback(window, width, height);
   };

   auto windowPosCallback = [](GLFWwindow* window, int xPos, int yPos)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->windowPosCallback(window, xPos, yPos);
   };

   auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->keyCallback(window, key, scancode, action, mods);
   };

   auto cursorPosCallback = [](GLFWwindow* window, double xPos, double yPos)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->cursorPosCallback(window, xPos, yPos);
   };

   auto scrollCallback = [](GLFWwindow* window, double xOffset, double yOffset)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->scrollCallback(window, xOffset, yOffset);
   };

   glfwSetFramebufferSizeCallback(mWindow, framebufferSizeCallback);
   glfwSetWindowPosCallback(mWindow, windowPosCallback);
   glfwSetKeyCallback(mWindow, keyCallback);
   glfwSetCursorPosCallback(mWindow, cursorPosCallback);
   glfwSetScrollCallback(mWindow, scrollCallback);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
   updateBufferAndViewportSizes(width, height);
}

void Window::windowPosCallback(GLFWwindow* window, int xPos, int yPos)
{
   updateBufferAndViewportSizes(mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (key != -1)
   {
      if (action == GLFW_PRESS)
      {
         mKeys.set(key);
      }
      else if (action == GLFW_RELEASE)
      {
         mKeys.reset(key);
      }

      mProcessedKeys.reset(key);
   }
}

void Window::cursorPosCallback(GLFWwindow* window, double xPos, double yPos)
{
   if (mFirstCursorPosCallback)
   {
      mLastCursorXPos = xPos;
      mLastCursorYPos = yPos;
      mFirstCursorPosCallback = false;
   }

   // TODO: Ideally this function would tell the camera to update its position based on the offsets.
   // I'm going to make the camera ask the window if it should update its position. Is there a better way to do this?
   mCursorXOffset = static_cast<float>(xPos - mLastCursorXPos);
   mCursorYOffset = static_cast<float>(mLastCursorYPos - yPos); // Reversed since the Y-coordinates of the mouse go from the bottom to the top

   mLastCursorXPos = xPos;
   mLastCursorYPos = yPos;

   mMouseMoved = true;
}

void Window::scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
   // TODO: Ideally this function would tell the camera to update its FOVY based on the Y offset.
   // I'm going to make the camera ask the window if it should update its FOVY. Is there a better way to do this?
   mScrollYOffset = static_cast<float>(yOffset);

#ifdef __EMSCRIPTEN__
   mScrollYOffset *= mScrollWheelSensitivity;
#endif

   mScrollWheelMoved = true;
}

#ifndef __EMSCRIPTEN__
bool Window::configureAntiAliasingSupport()
{
   if (!createMultisampleFramebuffer())
   {
      return false;
   }

   return true;
}

bool Window::createMultisampleFramebuffer()
{
   // Configure a framebuffer object to store raw multisample renders

   glGenFramebuffers(1, &mMultisampleFBO);

   glBindFramebuffer(GL_FRAMEBUFFER, mMultisampleFBO);

   // Create a multisample texture and use it as a color attachment
   glGenTextures(1, &mMultisampleTexture);

   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture, 0);

   // Create a multisample renderbuffer object and use it as a depth attachment
   glGenRenderbuffers(1, &mMultisampleRBO);

   glBindRenderbuffer(GL_RENDERBUFFER, mMultisampleRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mMultisampleRBO);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      std::cout << "Error - Window::configureAntiAliasingSupport - Multisample framebuffer is not complete" << "\n";
      return false;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return true;
}

void Window::bindMultisampleFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mMultisampleFBO);
}

void Window::generateAntiAliasedImage()
{
   glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultisampleFBO);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glBlitFramebuffer(0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, 0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_COLOR_BUFFER_BIT, GL_NEAREST); // TODO: Should this be GL_LINEAR?
}

void Window::resizeFramebuffers()
{
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glBindRenderbuffer(GL_RENDERBUFFER, mMultisampleRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Window::setNumberOfSamples(unsigned int numOfSamples)
{
   mNumOfSamples = numOfSamples;

   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glBindRenderbuffer(GL_RENDERBUFFER, mMultisampleRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
#endif

#ifdef __EMSCRIPTEN__
void Window::updateWindowDimensions(int width, int height)
{
   float devicePixelRatio = getDevicePixelRatio();
   mWidthOfWindowInPix    = width * devicePixelRatio;
   mHeightOfWindowInPix   = height * devicePixelRatio;
   glfwSetWindowSize(mWindow, mWidthOfWindowInPix, mHeightOfWindowInPix);
}
#endif

void Window::updateBufferAndViewportSizes(int widthOfFramebufferInPix, int heightOfFramebufferInPix)
{
   mWidthOfFramebufferInPix = widthOfFramebufferInPix;
   mHeightOfFramebufferInPix = heightOfFramebufferInPix;
   glfwGetWindowSize(mWindow, &mWidthOfWindowInPix, &mHeightOfWindowInPix);

#ifndef __EMSCRIPTEN__
   resizeFramebuffers();

   // Clear the multisample framebuffer
   glBindFramebuffer(GL_FRAMEBUFFER, mMultisampleFBO);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

   float aspectRatioOfScene  = 1280.0f / 720.0f;

   int lowerLeftCornerOfViewportXInPix = 0, lowerLeftCornerOfViewportYInPix = 0, widthOfViewportInPix = 0, heightOfViewportInPix = 0;

   // Let's say we want to use the width of the window for the viewport
   // What height would we need to keep the aspect ratio of the scene?
   float requiredHeight = mWidthOfFramebufferInPix * (1.0f / aspectRatioOfScene);

   // If the required height is greater than the height of the window, then we use the height of the window for the viewport
   if (requiredHeight > mHeightOfFramebufferInPix)
   {
      // What width would we need to keep the aspect ratio of the scene?
      float requiredWidth = mHeightOfFramebufferInPix * aspectRatioOfScene;
      if (requiredWidth > mWidthOfFramebufferInPix)
      {
         std::cout << "Error - Window::updateBufferAndViewportSizes - Couldn't calculate dimensions that preserve the aspect ratio of the scene" << '\n';
      }
      else
      {
         float widthOfTheTwoVerticalBars = mWidthOfFramebufferInPix - requiredWidth;

         lowerLeftCornerOfViewportXInPix = static_cast<int>(widthOfTheTwoVerticalBars / 2.0f);
         lowerLeftCornerOfViewportYInPix = 0;
         widthOfViewportInPix            = static_cast<int>(requiredWidth);
         heightOfViewportInPix           = mHeightOfFramebufferInPix;
      }
   }
   else
   {
      float heightOfTheTwoHorizontalBars = mHeightOfFramebufferInPix - requiredHeight;

      lowerLeftCornerOfViewportXInPix = 0;
      lowerLeftCornerOfViewportYInPix = static_cast<int>(heightOfTheTwoHorizontalBars / 2.0f);
      widthOfViewportInPix            = mWidthOfFramebufferInPix;
      heightOfViewportInPix           = static_cast<int>(requiredHeight);
   }

   glViewport(lowerLeftCornerOfViewportXInPix, lowerLeftCornerOfViewportYInPix, widthOfViewportInPix, heightOfViewportInPix);

   mLowerLeftCornerOfViewportXInPix = lowerLeftCornerOfViewportXInPix;
   mLowerLeftCornerOfViewportYInPix = lowerLeftCornerOfViewportYInPix;
   mWidthOfViewportInPix = widthOfViewportInPix;
   mHeightOfViewportInPix = heightOfViewportInPix;

#ifdef __EMSCRIPTEN__
   emscripten_set_element_css_size("canvas", getCanvasWidth(), getCanvasHeight());
#endif
}
