#include "COOPReadyState.h"
#include "Renderer.h"
#include "Player.h"
#include "GameManager.h"
#include "NetworkManager.h"
#include "COOPSelectJobState.h"
#include "COOPManager.h"
#include "Utils.h"

void COOPReadyState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\DungeonDoor.png");
    Renderer::SetTopASCIIImage(art);
}

void COOPReadyState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    Renderer::DisplayUI(UIPart::Top, 0, "=== [ 레이드 준비실 ] ===");

    PlayerJob myJob = COOPManager::GetInstance().GetMyJob();
    std::string jobStr = "기본";
    if (myJob == PlayerJob::Tanker) jobStr = "탱커";
    else if (myJob == PlayerJob::Healer) jobStr = "힐러";

    Renderer::DisplayUI(UIPart::CenterLeft, 2, "현재 직업: " + jobStr);
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "1. 직업 선택");
    Renderer::DisplayUI(UIPart::CenterLeft, 4, "2. 준비 완료");

    // --- [추가] 준비 인원 및 목록 표시 ---
    auto& coop = COOPManager::GetInstance();
    int readyCount = 0;
    for (const auto& pair : coop.players)
    {
        if (pair.second.isReady) readyCount++;
    }
    // 아레나의 기대 인원 계산 로직(방장+클라이언트 수)을 재사용
    int expected = NetworkManager::GetInstance().GetExpectedArenaPlayerCount();

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "준비 완료: " + std::to_string(readyCount) + " / " + std::to_string(expected));
    Renderer::DisplayUI(UIPart::CenterLeft, 6, "--- 참가자 목록 ---");

    int row = 7;
    if (coop.players.empty())
    {
        Renderer::DisplayUI(UIPart::CenterLeft, row++, "대기 중...");
    }
    else
    {
        for (const auto& pair : coop.players)
        {
            if (row > 15) break; // UI 오버플로우 방지
            std::string line = pair.first;
            line += pair.second.isReady ? "  [준비 완료]" : "  [대기 중]";
            Renderer::DisplayUI(UIPart::CenterLeft, row++, line);
        }
    }
    // ------------------------------------

    if (ch == 1 || lastCommand == "1")
    {
        GameManager::GetInstance().SetCurrentState(new COOPSelectJobState());
    }
    else if (ch == 2 || lastCommand == "2")
    {
        Player* p = GameManager::GetInstance().GetPlayer();
        if (p)
        {
            int level = p->GetLevel();
            // [수정] 배율을 퍼센트 정수로 계산 (예: 50렙이면 100 + 500 = 600%)
            int multiplierPercent = 100 + (level * COOP_DB::STAT_MULTIPLIER_PERCENT_PER_LEVEL);

            int finalAtk = p->GetAttack();
            int finalHp = p->GetHp();

            if (myJob == PlayerJob::Tanker)
            {
                // [수정] 먼저 곱한 뒤 100으로 나누어 소수점 오차 원천 차단
                finalHp += (COOP_DB::TANKER_BONUS_HP * multiplierPercent) / 100;
            }
            else if (myJob == PlayerJob::None)
            {
                finalAtk += (COOP_DB::DEALER_BONUS_ATK * multiplierPercent) / 100;
            }
            p->SetMaxHp(finalHp);
            p->SetHp(finalHp);

            NetworkManager::GetInstance().SendCOOPUpdateStatus(Client::playerName, finalAtk, finalHp, p->GetMaxHp(), static_cast<int>(myJob), false);
            COOPManager::GetInstance().UpdatePlayerStatus(Client::playerName, finalAtk, finalHp, p->GetMaxHp(), myJob, false);

            COOPManager::GetInstance().SetPlayerReady(Client::playerName, true);
        }
        NetworkManager::GetInstance().SendCOOPReady(true);
        Renderer::DisplayUITimed(UIPart::CenterLeft, 1, "준비 완료 상태를 전송했습니다.", 2.0f);
    }
}
