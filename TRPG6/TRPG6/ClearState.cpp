#include "ClearState.h"
#include "Renderer.h"
#include "Utils.h"

void ClearState::Enter()
{
    auto art = LoadImageAsASCII("..\\..\\Resources\\Win.png");
    Renderer::SetTopASCIIImage(art);
}

void ClearState::Update(int ch, std::string& lastCommand)
{
}
