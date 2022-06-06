#include <iostream>

#include "ModelViewerState.h"
#include "game.h"

Game::Game()
   : mFSM()
   , mWindow()
{

}

Game::~Game()
{

}

bool Game::initialize(const std::string& title)
{
   // Initialize the window
   mWindow = std::make_shared<Window>(title);

   if (!mWindow->initialize())
   {
      std::cout << "Error - Game::initialize - Failed to initialize the window" << "\n";
      return false;
   }

   // Create the FSM
   mFSM = std::make_shared<FiniteStateMachine>();

   // Initialize the states
   std::unordered_map<std::string, std::shared_ptr<State>> mStates;

   mStates["viewer"] = std::make_shared<ModelViewerState>(mFSM,
                                                          mWindow);

   // Initialize the FSM
   mFSM->initialize(std::move(mStates), "viewer");

   return true;
}

#ifdef __EMSCRIPTEN__
void Game::executeGameLoop()
{
   static double lastFrame = 0.0;

   double currentFrame = glfwGetTime();
   float deltaTime     = static_cast<float>(currentFrame - lastFrame);
   lastFrame           = currentFrame;

   mFSM->processInputInCurrentState(deltaTime);
   mFSM->updateCurrentState(deltaTime);
   mFSM->renderCurrentState();
}
#else
void Game::executeGameLoop()
{
   double currentFrame = 0.0;
   double lastFrame    = 0.0;
   float  deltaTime    = 0.0f;

   while (!mWindow->shouldClose())
   {
      currentFrame = glfwGetTime();
      deltaTime    = static_cast<float>(currentFrame - lastFrame);
      lastFrame    = currentFrame;

      mFSM->processInputInCurrentState(deltaTime);
      mFSM->updateCurrentState(deltaTime);
      mFSM->renderCurrentState();
   }
}
#endif

#ifdef __EMSCRIPTEN__
void Game::updateWindowDimensions(int width, int height)
{
   mWindow->updateWindowDimensions(width, height);
}
#endif
