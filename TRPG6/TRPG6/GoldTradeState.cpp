#include "GoldTradeState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "GoldTradePacket.h" // 골드 패킷 함수(SendGoldTradeRequest) 사용
#include "Utils.h"
#include <sstream>
#include <vector>

// 문자열을 공백(' ') 기준으로 나누는 유틸리티 함수
static std::vector<std::string> SplitSpace(const std::string& s)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (tokenStream >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

void GoldTradeState::Enter()
{
    // 입력 모드를 READ_MODE(문자열 입력 가능 상태)로 전환
    READ_MODE = true;
}

void GoldTradeState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    // 상단 아스키 아트 및 UI 출력
    auto art = LoadImageAsASCII("..\\..\\Resources\\gold.png");
    Renderer::SetTopASCIIImage(art);

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "=== [ 골드 전송 시스템 ] ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "양식 입력: [받을 사람] [골드량]");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "(예시: Hero 500)");
    Renderer::DisplayUI(UIPart::CenterLeft, 5, "0. 메인 화면");

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    // 1. 숫자 입력 처리 (메인 화면 복귀 등)
    if (ch == 0 && lastCommand.empty())
    {
        READ_MODE = false; // 문자열 입력 모드 해제
        GameManager::GetInstance().SetCurrentState(new GameStartState());
        return;
    }

    // 2. 명령어(문자열) 입력 처리 (골드 전송 로직)
    if (!lastCommand.empty())
    {
        // 0을 입력해서 나가려고 한 경우 예외 처리
        if (lastCommand == "0")
        {
            READ_MODE = false;
            lastCommand.clear();
            GameManager::GetInstance().SetCurrentState(new GameStartState());
            return;
        }

        std::vector<std::string> parts = SplitSpace(lastCommand);

        // 형식 검사 (받을사람, 골드량 2가지 요소가 필요)
        if (parts.size() < 2)
        {
            Renderer::DisplayUITimed(UIPart::CenterLeft, 7, "입력 형식이 잘못되었습니다! (예: Hero 500)", 2.0f);
            lastCommand.clear();
            return;
        }

        std::string receiverName = parts[0];
        std::string goldStr = parts[1];

        // 예외 검사 ① : 자기 자신에게는 보낼 수 없음
        if (receiverName == Client::playerName)
        {
            Renderer::DisplayUITimed(UIPart::CenterLeft, 7, "자기 자신에게는 골드를 보낼 수 없습니다!", 2.0f);
            lastCommand.clear();
            return;
        }

        // 예외 검사 ② : 입력한 골드가 유효한 숫자인지, 0원 이하는 아닌지 검사
        int32_t amount = 0;
        try
        {
            amount = std::stoi(goldStr);
        }
        catch (...)
        {
            Renderer::DisplayUITimed(UIPart::CenterLeft, 7, "골드 액수는 올바른 숫자여야 합니다!", 2.0f);
            lastCommand.clear();
            return;
        }

        if (amount <= 0)
        {
            Renderer::DisplayUITimed(UIPart::CenterLeft, 7, "0원 이하의 골드는 전송할 수 없습니다!", 2.0f);
            lastCommand.clear();
            return;
        }

        // 예외 검사 ③ : 현재 내가 보유한 골드보다 더 많이 보내려고 하는지 검사
        if (player != nullptr)
        {
            if (player->GetMoney() < amount)
            {
                Renderer::DisplayUITimed(UIPart::CenterLeft, 7, "보유 골드가 부족합니다! (현재 자산 부족)", 2.0f);
                lastCommand.clear();
                return;
            }
        }

        // 모든 검사 통과 시 서버(혹은 상대방)로 골드 전송 패킷 요청
        SendGoldTradeRequest(receiverName, amount);
        Renderer::DisplayUITimed(UIPart::CenterLeft, 7, receiverName + "님에게 " + std::to_string(amount) + " 골드 전송을 요청했습니다!", 2.0f);

        // 처리가 끝났으므로 입력 버퍼 비우기
        lastCommand.clear();
    }
}
