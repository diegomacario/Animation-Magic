#ifndef WINDOW_H
#define WINDOW_H

#ifdef __EMSCRIPTEN__
#define GLFW_INCLUDE_ES3
#else
#include <glad/glad.h>
#endif
#include <GLFW/glfw3.h>

#include <bitset>

// TODO: Take advantage of inlining in this class.
class Window
{
public:

   Window(const std::string& title);
   ~Window();

   Window(const Window&) = delete;
   Window& operator=(const Window&) = delete;

   Window(Window&&) = delete;
   Window& operator=(Window&&) = delete;

   bool         initialize();

   bool         shouldClose() const;
   void         setShouldClose(bool shouldClose); // TODO: Could this be considered to be const?
   void         swapBuffers();                    // TODO: Could this be considered to be const?
   void         pollEvents();                     // TODO: Could this be considered to be const?

   // Window
   unsigned int getWidthOfWindowInPix() const;
   unsigned int getHeightOfWindowInPix() const;
   unsigned int getWidthOfFramebufferInPix() const;
   unsigned int getHeightOfFramebufferInPix() const;
#ifndef __EMSCRIPTEN__
   bool         isFullScreen() const;
   void         setFullScreen(bool fullScreen);
#endif

   // Keyboard
   bool         keyIsPressed(int key) const;
   bool         keyHasBeenProcessed(int key) const;
   void         setKeyAsProcessed(int key);

   // Cursor
   bool         mouseMoved() const;
   void         resetMouseMoved();
   void         resetFirstMove();
   float        getCursorXOffset() const;
   float        getCursorYOffset() const;
   void         enableCursor(bool enable);
   bool         isMouseButtonPressed(int button);

   // Scroll wheel
   bool         scrollWheelMoved() const;
   void         resetScrollWheelMoved();
   float        getScrollYOffset() const;

   // Anti aliasing support
#ifndef __EMSCRIPTEN__
   bool         configureAntiAliasingSupport();
   bool         createMultisampleFramebuffer();
   void         bindMultisampleFramebuffer();
   void         generateAntiAliasedImage();
   void         resizeFramebuffers();
   void         setNumberOfSamples(unsigned int numOfSamples);
#endif

#ifdef __EMSCRIPTEN__
   void         updateWindowDimensions(int width, int height);
#endif

   int          getLowerLeftCornerOfViewportXInPix() { return mLowerLeftCornerOfViewportXInPix; };
   int          getLowerLeftCornerOfViewportYInPix() { return mLowerLeftCornerOfViewportYInPix; };
   int          getWidthOfViewportInPix() { return mWidthOfViewportInPix; };
   int          getHeightOfViewportInPix() { return mHeightOfViewportInPix; };
   void         setViewport();

private:

   void         setInputCallbacks();
   void         framebufferSizeCallback(GLFWwindow* window, int width, int height);
   void         windowPosCallback(GLFWwindow* window, int xPos, int yPos);
   void         keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
   void         cursorPosCallback(GLFWwindow* window, double xPos, double yPos);
   void         scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

   void         updateBufferAndViewportSizes(int widthOfFramebufferInPix, int heightOfFramebufferInPix);

   // Window
   GLFWwindow*                    mWindow;
   int                            mWidthOfWindowInPix;
   int                            mHeightOfWindowInPix;
   int                            mWidthOfFramebufferInPix;
   int                            mHeightOfFramebufferInPix;
   std::string                    mTitle;
#ifndef __EMSCRIPTEN__
   bool                           mIsFullScreen;
#endif

   // Keyboard
   std::bitset<GLFW_KEY_LAST + 1> mKeys;
   std::bitset<GLFW_KEY_LAST + 1> mProcessedKeys;

   // Cursor
   bool                           mMouseMoved;
   bool                           mFirstCursorPosCallback;
   double                         mLastCursorXPos;
   double                         mLastCursorYPos;
   float                          mCursorXOffset;
   float                          mCursorYOffset;

   // Scroll wheel
   mutable bool                   mScrollWheelMoved;
   float                          mScrollYOffset;
#ifdef __EMSCRIPTEN__
   float                          mScrollWheelSensitivity;
#endif

   // Anti aliasing support
#ifndef __EMSCRIPTEN__
   unsigned int                   mMultisampleFBO;
   unsigned int                   mMultisampleTexture;
   unsigned int                   mMultisampleRBO;
   unsigned int                   mNumOfSamples;
#endif

   int                            mLowerLeftCornerOfViewportXInPix;
   int                            mLowerLeftCornerOfViewportYInPix;
   int                            mWidthOfViewportInPix;
   int                            mHeightOfViewportInPix;
};

#endif
