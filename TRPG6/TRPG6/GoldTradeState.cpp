#include "GoldTradeState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "TradeState.h"
#include "GoldTradePacket.h"
#include "Utils.h"
#include "Player.h"
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
    Renderer::ClearAllCenterLeftUI();
}

void GoldTradeState::Update(int ch, std::string& lastCommand)
{
    player = GameManager::GetInstance().GetPlayer();

    auto art = LoadImageAsASCII("..\\..\\Resources\\gold.png");
    Renderer::SetTopASCIIImage(art);

    Renderer::DisplayUI(UIPart::CenterLeft, 0, "=== [ 골드 전송 시스템 ] ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "양식 입력: [받을 사람] [골드량]");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "(예시: Hero 500)");
    Renderer::DisplayUI(UIPart::CenterLeft, 5, "0. 뒤로 가기 (거래 센터)");

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    // 0번을 누르거나 "0"을 입력하면 새로 만든 중간 다리(TradeState)로 복귀
    if ((ch == 0 && lastCommand.empty()) || lastCommand == "0")
    {
        READ_MODE = false;
        lastCommand.clear();
        Renderer::ClearAllCenterLeftUI();
        GameManager::GetInstance().SetCurrentState(new TradeState());
        return;
    }

    // -------------------------------------------------------------
    // [1단계] 문자열 입력 처리 (if문 안에서는 로직 검사와 '메시지 저장'만 수행)
    // -------------------------------------------------------------
    if (!lastCommand.empty())
    {
        // 입력 버퍼 복사 후 즉시 클리어 (다음 프레임에 if문이 반복 실행되는 것 방지)
        std::string currentInput = lastCommand;
        lastCommand.clear();

        std::vector<std::string> parts = SplitSpace(currentInput);

        // 예외 검사 : 인자 개수 부족 (오타 등)
        if (parts.size() < 2)
        {
            errorMessage = "입력 형식이 잘못되었습니다! (예: Hero 500)";
            return;
        }

        std::string receiverName = parts[0];
        std::string goldStr = parts[1];

        // 예외 검사 ① : 자기 자신에게는 송금 불가
        if (receiverName == Client::playerName)
        {
            errorMessage = "자기 자신에게는 골드를 보낼 수 없습니다!";
            return;
        }

        // 예외 검사 ② : 골드 액수가 정상적인 숫자인지 검사
        int32_t amount = 0;
        try
        {
            amount = std::stoi(goldStr);
        }
        catch (...)
        {
            errorMessage = "골드 액수는 올바른 숫자여야 합니다!";
            return;
        }

        // 예외 검사 ③ : 0원 이하 전송 제한
        if (amount <= 0)
        {
            errorMessage = "0원 이하의 골드는 전송할 수 없습니다!";
            return;
        }

        // 예외 검사 ④ : 현재 내가 가진 돈보다 더 많이 보내려 할 때
        if (player != nullptr)
        {
            if (player->GetMoney() < amount)
            {
                errorMessage = "보유 골드가 부족합니다! (현재 자산 부족)";
                return;
            }
        }
    }

    //오류 메세지 출력
    if (!errorMessage.empty())
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 7, errorMessage, 2.0f);
    }
}
