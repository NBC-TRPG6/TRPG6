#include "GoldTradeState.h"
#include "Renderer.h"
#include "GameManager.h"
#include "GameStartState.h"
#include "TradeState.h"       
#include "NetworkManager.h" 
#include "Utils.h"
#include "Player.h"
#include <sstream>
#include <vector>

static std::vector<std::string> SplitSpace(const std::string& s)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (tokenStream >> token) { tokens.push_back(token); }
    return tokens;
}

void GoldTradeState::Enter()
{
    READ_MODE = true;
}

void GoldTradeState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
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

    // 이전으로 돌아가기
    if ((ch == 0 && lastCommand.empty()) || lastCommand == "0")
    {
        READ_MODE = false;
        lastCommand.clear();
        Renderer::ClearAllCenterLeftUI();
        GameManager::GetInstance().SetCurrentState(new TradeState());
        return;
    }

    if (!lastCommand.empty())
    {
        std::string currentInput = lastCommand;
        lastCommand.clear();

        std::vector<std::string> parts = SplitSpace(currentInput);

        if (parts.size() < 2)
        {
            errorMessage = "입력 형식이 잘못되었습니다! (예: Hero 500)";
            return;
        }

        std::string receiverName = parts[0];
        std::string goldStr = parts[1];

        if (receiverName == Client::playerName)
        {
            errorMessage = "자기 자신에게는 골드를 보낼 수 없습니다!";
            return;
        }

        int32_t amount = 0;
        try { amount = std::stoi(goldStr); }
        catch (...)
        {
            errorMessage = "골드 액수는 올바른 숫자여야 합니다!";
            return;
        }

        if (amount <= 0)
        {
            errorMessage = "0원 이하의 골드는 전송할 수 없습니다!";
            return;
        }

        if (player != nullptr)
        {
            if (player->GetMoney() < amount)
            {
                errorMessage = "골드가 부족합니다!";
                return;
            }
        }

        // 서버에 요청 패킷 전송
        NetworkManager::GetInstance().SendGoldTradeRequest(receiverName, amount);
        errorMessage = receiverName + "님에게 " + std::to_string(amount) + " 골드 전송 완료!";
    }

    if (!errorMessage.empty())
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 10, errorMessage, 2.0f);
    }
}
