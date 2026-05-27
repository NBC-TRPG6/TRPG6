// GoldTradePacket.cpp
#include "GoldTradePacket.h"
#include "NetworkManager.h"
#include "IPCManager.h"

// [클라이언트] 유저가 UI 등에서 '골드 보내기'를 수행했을 때 호출하는 함수
void SendGoldTradeRequest(const std::string& receiverName, int32_t amount) {
    if (amount <= 0) {
        IPCManager::GetInstance().SendLog("[오류] 0원 이하의 골드는 보낼 수 없습니다.");
        return;
    }

    Pkt_GoldTradeRequest pkt;
    pkt.goldAmount = amount;
    std::strncpy(pkt.receiverName, receiverName.c_str(), sizeof(pkt.receiverName) - 1);

    // 내가 방장(서버)인 경우: 소켓 없이 내 네트워크 매니저에서 바로 처리하도록 토스
    if (Client::isServer) {
        NetworkManager::GetInstance().HandleGoldTradeRequest(INVALID_SOCKET, &pkt);
    }
    // 내가 일반 참가자(클라이언트)인 경우: 서버 연결 소켓을 통해 패킷 전송
    else {
        // NetworkManager.h에 정의된 클라이언트 소켓 변수와 전송 메서드를 활용하여 서버로 보냅니다.
        SOCKET cSock = NetworkManager::GetInstance().GetClientSocket();

        if (cSock != INVALID_SOCKET) {
            send(cSock, reinterpret_cast<char*>(&pkt), pkt.header.size, 0);
        }
        else {
            IPCManager::GetInstance().SendLog("[오류] 서버와의 연결 소켓이 유효하지 않습니다.");
        }
    }
}
