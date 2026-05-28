#include "Packet.h"
#include "Player.h"
#include "Item.h"
#include "WeaponItem.h"
#include "WeaponTable.h"
#include "IPCManager.h"
#include <cstring>

namespace
{
    std::string ArenaSlotItemName(const ArenaItemSlot& slot)
    {
        return std::string(slot.itemName, strnlen(slot.itemName, sizeof(slot.itemName)));
    }

    Item* CreateItemFromArenaSlot(const std::string& itemName, ItemType itemType, int32_t value)
    {
        if (itemType == ItemType::WEAPON)
        {
            const WeaponData* data = WeaponTable::GetInstance().GetWeaponDataByName(itemName);
            if (data != nullptr)
            {
                return new WeaponItem(
                    data->Name,
                    data->BaseName,
                    data->UpgradeCount,
                    data->AttackBonus,
                    data->HPBonus,
                    data->Price);
            }

            IPCManager::GetInstance().SendLog(
                "[아레나] 무기 테이블에 없는 이름, 일반 Item으로 반환: " + itemName);
        }

        return new Item(itemName, itemType, value, 0);
    }

    Item* CreateItemFromArenaSlot(const ArenaItemSlot& slot)
    {
        return CreateItemFromArenaSlot(
            ArenaSlotItemName(slot),
            static_cast<ItemType>(slot.itemType),
            slot.value);
    }

    void UnequipWeaponIfMatches(Player* player, const std::string& itemName)
    {
        if (player == nullptr) return;

        WeaponItem* equipped = player->GetEquippedWeapon();
        if (equipped != nullptr && equipped->GetName() == itemName)
        {
            player->EquipWeapon(nullptr);
        }
    }

    void SetLocalInventoryItemCount(Player* player, Inventory<Item>& inventory,
        const std::string& itemName, ItemType itemType, int32_t value, int32_t count)
    {
        InventorySlot<Item>* slot = inventory.GetItemSlot(itemName);

        if (count <= 0)
        {
            UnequipWeaponIfMatches(player, itemName);
            if (slot != nullptr && slot->count > 0)
            {
                inventory.UseItem(nullptr, itemName, slot->count);
            }
            return;
        }

        if (slot != nullptr)
        {
            if (slot->count > count)
            {
                if (count == 0)
                {
                    UnequipWeaponIfMatches(player, itemName);
                }
                inventory.UseItem(nullptr, itemName, slot->count - count);
            }
            else if (slot->count < count)
            {
                inventory.AddItem(CreateItemFromArenaSlot(itemName, itemType, value), count - slot->count);
            }
            return;
        }

        inventory.AddItem(CreateItemFromArenaSlot(itemName, itemType, value), count);
    }

}

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

const ArenaItemSlot* GetArenaSessionApplyBattleSlots(const Pkt_ArenaSessionApplyHeader* hdr)
{
    if (hdr == nullptr) return nullptr;
    return reinterpret_cast<const ArenaItemSlot*>(
        reinterpret_cast<const char*>(hdr) + ArenaSessionApplyHeaderSize());
}

const ArenaItemSlot* GetArenaSessionApplyRewardSlots(const Pkt_ArenaSessionApplyHeader* hdr)
{
    if (hdr == nullptr) return nullptr;
    return GetArenaSessionApplyBattleSlots(hdr) + hdr->battleSlotCount;
}

void ApplyArenaSessionToLocalPlayer(Player* player, const char* packetData, size_t packetSize)
{
    if (player == nullptr || packetData == nullptr || packetSize < sizeof(PacketHeader)) return;

    const auto* hdr = reinterpret_cast<const Pkt_ArenaSessionApplyHeader*>(packetData);
    if (!IsValidArenaSessionApplySize(hdr->header.size, hdr->battleSlotCount, hdr->rewardSlotCount)) return;
    if (hdr->header.size > packetSize) return;

    const ArenaItemSlot* battleSlots = GetArenaSessionApplyBattleSlots(hdr);
    const ArenaItemSlot* rewardSlots = GetArenaSessionApplyRewardSlots(hdr);

    Inventory<Item>& inventory = player->GetInventory();

    for (uint8_t i = 0; i < hdr->battleSlotCount; ++i)
    {
        const ArenaItemSlot& slot = battleSlots[i];
        const std::string itemName = slot.itemName;
        const ItemType itemType = static_cast<ItemType>(slot.itemType);
        SetLocalInventoryItemCount(player, inventory, itemName, itemType, slot.value, slot.count);
    }

    for (uint8_t i = 0; i < hdr->rewardSlotCount; ++i)
    {
        const ArenaItemSlot& slot = rewardSlots[i];
        if (slot.count <= 0) continue;

        inventory.AddItem(CreateItemFromArenaSlot(slot), slot.count);
    }


    IPCManager::GetInstance().SendLog("[아레나] 로컬 플레이어 반영 완료");
}

void ApplyArenaBetRefundToLocalPlayer(Player* player, const Pkt_ArenaBetRefund& pkt)
{
    if (player == nullptr) return;

    Inventory<Item>& inventory = player->GetInventory();

    for (uint8_t i = 0; i < pkt.slotCount && i < MAX_ARENA_ITEM_SLOTS; ++i)
    {
        const ArenaItemSlot& slot = pkt.slots[i];
        if (slot.count <= 0) continue;

        inventory.AddItem(CreateItemFromArenaSlot(slot), slot.count);
    }

    IPCManager::GetInstance().SendLog("[아레나] 베팅 아이템이 인벤토리로 반환되었습니다.");
}
