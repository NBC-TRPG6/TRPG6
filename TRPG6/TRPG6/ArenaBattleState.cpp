#include "ArenaBattleState.h"
#include "ArenaWaitState.h"
#include "ArenaResultState.h"
#include "GameManager.h"
#include "Renderer.h"

void ArenaBattleState::Enter()
{
    Renderer::ClearAllCenterLeftUI();
}

void ArenaBattleState::Update(int ch, std::string& lastCommand)
{
    Renderer::DisplayUI(UIPart::Top, 0, "아레나 전투 중!");
    Renderer::DisplayUI(UIPart::CenterLeft, 9, "1. 대기하기");
    Renderer::DisplayUI(UIPart::CenterLeft, 10, "2. 결과 확인");

    if (ch == 1)
    {
        GameManager::GetInstance().SetCurrentState(new ArenaWaitState());
    }
    else if (ch == 2)
    {
        GameManager::GetInstance().SetCurrentState(new ArenaResultState());
    }
}

void ArenaBattleState::Exit()
{
}
