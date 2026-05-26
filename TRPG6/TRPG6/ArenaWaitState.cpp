#include "ArenaWaitState.h"
#include "Renderer.h"

void ArenaWaitState::Enter() {
    Renderer::ClearAllCenterLeftUI();
}

void ArenaWaitState::Update(int ch, std::string& lastCommand) {
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 대기 중...");
}

void ArenaWaitState::Exit() {
}
