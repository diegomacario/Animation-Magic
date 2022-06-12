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
#endif

int main()
{
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
