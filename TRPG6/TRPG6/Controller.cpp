#include <conio.h> // Windows 전용: _kbhit(), _getch()
#include "Controller.h"
#include "GameManager.h"
#include "Renderer.h"
#include "DATABASE.h"

void Controller::ProcessInput() {
    // 1. 비동기 입력 처리 (Input Blocking 개선)
    if (_kbhit()) {
        char ch = _getch();
        if (ch == '\r') { // Enter 키
            lastCommand = inputBuffer;		
			// Renderer::DisplayUI(UIPart::CenterLeft, 12, std::string("Last Command: " + lastCommand)); // 디버깅용
            if (lastCommand == "exit") GameManager::GetInstance().SetIsGameRunning(false);
            inputBuffer.clear();
        }
        else if (ch == '\b') { // Backspace
            if (!inputBuffer.empty()) inputBuffer.pop_back();
        }
        else if (READ_MODE) {
            inputBuffer += ch;
        }
        else if (inputBuffer.length() < 6 && ch >= 48 && ch <= 57) {
            inputBuffer += ch;
        }
    }
}
