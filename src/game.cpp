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
   static double lastFrame = glfwGetTime();
   static double simulationTimeSeconds = 0.0;
   static float fixedDelta = 1.0f / 60.0f;

   double currentFrame = glfwGetTime();
   float deltaTime     = static_cast<float>(currentFrame - lastFrame);
   lastFrame           = currentFrame;
   simulationTimeSeconds += deltaTime;

   // Simulation time got very big, which probably means the browser window or tab running the game went
   // out of focus so the game loop stopped executing. This affects many systems in a bad way. Clamp it down
   // to something normal
   if (simulationTimeSeconds > 4)
   {
      simulationTimeSeconds = fixedDelta; // 1 frame of simulation
   }

   mFSM->processInputInCurrentState();

   while (simulationTimeSeconds >= fixedDelta)
   {
      mFSM->updateCurrentState(fixedDelta);
      simulationTimeSeconds -= fixedDelta;
   }

   mFSM->renderCurrentState();
}
#else
void Game::executeGameLoop()
{
   double simulationTimeSeconds = 0.0;
   const float fixedDelta = 1.0f / 60.0f;

   double currentFrame = 0.0;
   double lastFrame    = glfwGetTime();
   float  deltaTime    = 0.0f;

   while (!mWindow->shouldClose())
   {
      currentFrame = glfwGetTime();
      deltaTime    = static_cast<float>(currentFrame - lastFrame);
      lastFrame    = currentFrame;
      simulationTimeSeconds += deltaTime;

      mFSM->processInputInCurrentState();

      while (simulationTimeSeconds >= fixedDelta) {
         mFSM->updateCurrentState(fixedDelta);
         simulationTimeSeconds -= fixedDelta;
      }

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
