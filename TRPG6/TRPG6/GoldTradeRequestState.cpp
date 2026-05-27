#include "GoldTradeRequestState.h"
#include "Renderer.h"
#include "NetworkManager.h"
#include "GameManager.h"
#include "GoldTradeState.h"
#include "GoldTradePacket.h" // ★ 골드 패킷 함수(SendGoldTradeRequest)를 쓰기 위해 인클루드
#include <sstream>
#include <vector>

// 문자열을 공백(' ')이나 구분자로 나누는 유틸리티 함수
std::vector<std::string> Split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

void GoldTradeRequestState::Enter()
{
}

void GoldTradeRequestState::Update(int ch, std::string& lastCommand)
{
    Renderer::ClearAllCenterLeftUI();
    player = GameManager::GetInstance().GetPlayer();

    if (player != nullptr)
    {
        player->PrintStatus();
    }

    Renderer::DisplayUI(UIPart::Top, 0, "=== [ 골드 전송 신청서 작성 ] ===");
    Renderer::DisplayUI(UIPart::CenterLeft, 2, "형식: [받을사람이름] [보낼골드량]");
    Renderer::DisplayUI(UIPart::CenterLeft, 3, "예시: Hero 500  (이름과 골드 사이에 한 칸 띄우기)");
    Renderer::DisplayUI(UIPart::CenterLeft, 5, "0. 뒤로 가기");

    // 0을 입력하면 이전 화면(골드 전송 시스템 메인)으로 돌아감
    if (ch == 0 || lastCommand == "0")
    {
        GameManager::GetInstance().SetCurrentState(new GoldTradeState());
        return;
    }

    // 유저가 아무것도 입력하지 않고 엔터만 친 경우 처리 안 함
    if (lastCommand.empty())
    {
        return;
    }

    // 1. 유저가 입력한 명령어 파싱 (공백 ' ' 기준으로 분리)
    std::vector<std::string> parts = Split(lastCommand, ' ');

    // 올바른 형식은 [이름] [골드] 총 2개의 인자가 있어야 함
    if (parts.size() < 2)
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 8, "입력 형식이 잘못되었습니다! (예: Hero 500)", 2.0f);
        return;
    }

    std::string receiverName = parts[0];
    std::string goldStr = parts[1];

    // 2. 예외 검사 ① : 자기 자신에게는 보낼 수 없음
    if (receiverName == Client::playerName)
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 8, "자기 자신에게는 골드를 보낼 수 없습니다!", 2.0f);
        return;
    }

    // 3. 예외 검사 ② : 입력한 골드가 유효한 숫자인지, 0원 이하는 아닌지 검사
    int32_t amount = 0;
    try
    {
        amount = std::stoi(goldStr);
    }
    catch (...)
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 8, "골드 액수는 올바른 숫자여야 합니다!", 2.0f);
        return;
    }

    if (amount <= 0)
    {
        Renderer::DisplayUITimed(UIPart::CenterLeft, 8, "0원 이하의 골드는 전송할 수 없습니다!", 2.0f);
        return;
    }

    // 4. 예외 검사 ③ : 현재 내가 보유한 골드보다 더 많이 보내려고 하는지 검사
    if (player != nullptr)
    {
        // Character.h에 구현된 GetMoney() 함수 활용
        if (player->GetMoney() < amount)
        {
            Renderer::DisplayUITimed(UIPart::CenterLeft, 8, "보유하신 골드가 부족합니다! (현재 잔액 부족)", 2.0f);
            return;
        }
    }

    // 5. 모든 검사 통과 시 서버로 골드 거래 요청 전송!
    // 동료분이 만들어 두신 GoldTradePacket.cpp의 전송 함수를 호출합니다.
    SendGoldTradeRequest(receiverName, amount);

    Renderer::DisplayUITimed(UIPart::CenterLeft, 8, receiverName + "님에게 " + std::to_string(amount) + " 골드 전송 요청을 보냈습니다!", 2.0f);

    // 전송 후에는 다시 골드 전송 메인 화면으로 빠져나갑니다.
    GameManager::GetInstance().SetCurrentState(new GoldTradeState());
}
