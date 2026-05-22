#include "GameManager.h"
#include "GameStartState.h"
#include "Renderer.h"
#include "Player.h"

void GameStartState::Update(int ch, std::string& lastCommand) {
        GameManager::GetInstance().GetPlayer()->PrintStatus();

}
