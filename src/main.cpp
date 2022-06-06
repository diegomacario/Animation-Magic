#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "game.h"

#ifdef __EMSCRIPTEN__
Game game;

void gameLoop()
{
   game.executeGameLoop();
}

extern "C" void EMSCRIPTEN_KEEPALIVE updateWindowDimensions(int width, int height)
{
   game.updateWindowDimensions(width, height);
}

EM_JS(bool, runningOnFirefox, (), {
   if (navigator.userAgent.toLowerCase().indexOf('firefox') > -1) {
      return true;
   }
   else
   {
      return false;
   }
});

int runKey;
std::string runInstruction;
#endif

int main()
{
#ifdef __EMSCRIPTEN__
   if (runningOnFirefox())
   {
      runKey = 70; // Left control
      runInstruction = "Hold the F key to run.";
   }
   else
   {
      runKey = 340; // Left shift
      runInstruction = "Hold the left Shift key to run.";
   }
#endif

#ifndef __EMSCRIPTEN__
   Game game;
#endif

   if (!game.initialize("Animation Magic"))
   {
      std::cout << "Error - main - Failed to initialize the game" << "\n";
      return -1;
   }

#ifdef __EMSCRIPTEN__
   emscripten_set_main_loop(gameLoop, 0, true);
#else
   game.executeGameLoop();
#endif

   return 0;
}
