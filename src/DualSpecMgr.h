#ifndef MANGOS_DUALSPEC_MGR_H
#define MANGOS_DUALSPEC_MGR_H

#include "DualSpecConfig.h"

#include "Platform/Define.h"

#include <unordered_map>
#include <map>

class GameObject;
class Item;
class ObjectGuid;
class Player;
class Unit;

struct ActionButton;

struct DualSpecPlayerTalent
{
    uint8 state;
    uint8 spec;
};

struct DualSpecPlayerStatus
{
    uint8 specCount;
    uint8 activeSpec;
};

typedef std::unordered_map<uint32, DualSpecPlayerTalent> DualSpecPlayerTalentMap;
typedef std::map<uint8, ActionButton> ActionButtonList;

class DualSpecMgr
{
public:
    DualSpecMgr() {}

    void Init();

    // Player hooks
    bool OnPlayerGossipSelect(Player* player, const ObjectGuid& guid, uint32 sender, uint32 action);
    bool OnPlayerGossipSelect(Player* player, Unit* creature, uint32 sender, uint32 action);
    bool OnPlayerGossipSelect(Player* player, GameObject* gameObject, uint32 sender, uint32 action);
    bool OnPlayerGossipSelect(Player* player, Item* item, uint32 sender, uint32 action);
    void OnPlayerLearnTalent(Player* player, uint32 spellId);
    void OnPlayerResetTalents(Player* player, uint32 cost);

    void OnPlayerLogIn(uint32 playerId);
    void OnPlayerLogOut(Player* player);
    void OnPlayerCharacterCreated(Player* player);
    void OnPlayerCharacterDeleted(uint32 playerId);
    void OnPlayerSaveToDB(Player* player);
    bool OnPlayerLoadActionButtons(Player* player, ActionButtonList& actionButtons);
    bool OnPlayerSaveActionButtons(Player* player, ActionButtonList& actionButtons);

private:
    void LoadPlayerSpec(uint32 playerId);
    uint8 GetPlayerActiveSpec(Player* player) const;
    uint8 GetPlayerSpecCount(Player* player) const;

    void LoadPlayerTalents(uint32 playerId);
    DualSpecPlayerTalentMap& GetPlayerTalents(Player* player, int8 spec = -1);
    void AddPlayerTalent(uint32 playerId, uint32 spellId, uint8 spec, bool learned);
    void SavePlayerTalents(Player* player);

    void SendPlayerActionButtons(const Player* player, bool clear) const;

private:
    std::map<uint32, DualSpecPlayerTalentMap[MAX_TALENT_SPECS]> playersTalents;
    std::map<uint32, DualSpecPlayerStatus> playersStatus;
};

#define sDualSpecMgr MaNGOS::Singleton<DualSpecMgr>::Instance()
#endif