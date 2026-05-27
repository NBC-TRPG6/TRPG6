#pragma once
#include "IGameState.h"
#include "WeaponItem.h"
#include <vector>
#include <string>

enum class SmithUIStep {
    MainMenu,            // 1.조합 2.강화
    SelectCraftFirst,    // 조합 첫 번째 재료 선택
    SelectCraftSecond,   // 조합 두 번째 재료 선택
    SelectUpgradeFirst,  // 강화 첫 번째 무기 선택
    SelectUpgradeSecond, // 강화 두 번째 무기 선택
    ShowResult,          // 결과 출력
};

class SmithState : public IGameState {
public:
    void Enter() override;
    void Update(int ch, std::string& lastCommand) override;
    void Exit() override;

private:
    void DrawMainMenu();
    void DrawItemList(); // 현재 단계에 맞는 아이템 목록 출력

    SmithUIStep CurrentStep = SmithUIStep::MainMenu;

    // 첫 번째 선택한 슬롯 인덱스
    int FirstSelectedIndex = -1;

    // 결과 출력용 메시지
    std::string ResultMessage;

    // 현재 단계에서 보여줄 아이템 슬롯 인덱스 목록
    std::vector<int> filteredIndices;
};
