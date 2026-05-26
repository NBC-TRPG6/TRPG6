#include "Packet.h"
#include "Player.h"
#include "Item.h"

std::vector<char> BuildArenaPlayerSnapshotPacket(Player* player)
{
    std::vector<char> buffer;
    if (player == nullptr) return buffer;

    uint8_t slotCount = 0;
    std::vector<ArenaItemSlot> itemSlots;
    itemSlots.reserve(MAX_ARENA_ITEM_SLOTS);

    for (const auto& invSlot : player->GetInventory().GetSlots())
    {
        if (invSlot.item == nullptr || invSlot.count <= 0) continue;
        if (slotCount >= MAX_ARENA_ITEM_SLOTS) break;

        ArenaItemSlot slot{};
        CopyStringToPacketField(slot.itemName, sizeof(slot.itemName), invSlot.item->GetName());
        slot.itemType = static_cast<uint8_t>(invSlot.item->GetType());
        slot.value = invSlot.item->GetValue();
        slot.count = invSlot.count;
        itemSlots.push_back(slot);
        ++slotCount;
    }

    const size_t totalSize = ArenaSnapshotPacketSize(slotCount);
    buffer.resize(totalSize);

    auto* hdr = reinterpret_cast<Pkt_ArenaPlayerSnapshotHeader*>(buffer.data());
    hdr->header.size = static_cast<uint16_t>(totalSize);
    hdr->header.type = PacketType::PKT_C2S_ARENA_PLAYER_SNAPSHOT;
    CopyStringToPacketField(hdr->playerName, sizeof(hdr->playerName), player->GetName());
    hdr->maxHp = player->GetMaxHp();
    hdr->hp = player->GetHp();
    hdr->attack = player->GetAttack();
    hdr->level = player->GetLevel();
    hdr->itemSlotCount = slotCount;

    if (slotCount > 0)
    {
        std::memcpy(buffer.data() + ArenaSnapshotHeaderSize(),
            itemSlots.data(),
            slotCount * sizeof(ArenaItemSlot));
    }

    return buffer;
}

const ArenaItemSlot* GetArenaSnapshotItems(const Pkt_ArenaPlayerSnapshotHeader* snapshotHeader)
{
    if (snapshotHeader == nullptr) return nullptr;
    return reinterpret_cast<const ArenaItemSlot*>(
        reinterpret_cast<const char*>(snapshotHeader) + ArenaSnapshotHeaderSize());
}
