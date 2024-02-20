#ifndef MANGOS_DUALSPEC_MGR_H
#define MANGOS_DUALSPEC_MGR_H

#include "DualSpecConfig.h"
#include "Module.h"

#include "Platform/Define.h"

#include <unordered_map>
#include <map>

class Creature;
class GameObject;
class Item;
class ObjectGuid;
class Player;
class Unit;

struct ActionButton;

enum DualSpecMessages
{
    DUAL_SPEC_DESCRIPTION = 12000,
    DUAL_SPEC_COST_IS,
    DUAL_SPEC_CHANGE_MY_SPEC,
    DUAL_SPEC_NO_GOLD_UNLOCK,
    DUAL_SPEC_ARE_YOU_SURE_BEGIN,
    DUAL_SPEC_ARE_YOU_SURE_END,
    DUAL_SPEC_ALREADY_ON_SPEC,
    DUAL_SPEC_ACTIVATE,
    DUAL_SPEC_RENAME,
    DUAL_SPEC_UNNAMED,
    DUAL_SPEC_ACTIVE,
    DUAL_SPEC_ERR_COMBAT,
    DUAL_SPEC_ERR_INSTANCE,
    DUAL_SPEC_ERR_MOUNT,
    DUAL_SPEC_ERR_DEAD,
    DUAL_SPEC_ERR_UNLOCK,
    DUAL_SPEC_ERR_LEVEL,
    DUAL_SPEC_ACTIVATE_COLOR,
    DUAL_SPEC_RENAME_COLOR,
    DUAL_SPEC_ARE_YOU_SURE_SWITCH,
    DUAL_SPEC_PURCHASE,
    DUAL_SPEC_ERR_ITEM_CREATE,
};

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

class DualSpecModule : public Module
{
public:
    DualSpecModule() : Module() {}

    void Init();

    // Player hooks
    bool OnPlayerItemUse(Player* player, Item* item);
    bool OnPlayerGossipHello(Player* player, Creature* creature);
    bool OnPlayerGossipSelect(Player* player, const ObjectGuid& guid, uint32 sender, uint32 action, const std::string& code);
    bool OnPlayerGossipSelect(Player* player, Unit* creature, uint32 sender, uint32 action, const std::string& code);
    bool OnPlayerGossipSelect(Player* player, Item* item, uint32 sender, uint32 action, const std::string& code);
    void OnPlayerLearnTalent(Player* player, uint32 spellId);
    void OnPlayerResetTalents(Player* player, uint32 cost);

    void OnPlayerPreLoadFromDB(uint32 playerId);
    void OnPlayerPostLoadFromDB(Player* player);
    void OnPlayerLogOut(Player* player);
    void OnPlayerCharacterCreated(Player* player);
    void OnPlayerCharacterDeleted(uint32 playerId);
    void OnPlayerSaveToDB(Player* player);
    bool OnPlayerLoadActionButtons(Player* player, ActionButtonList& actionButtons);
    bool OnPlayerSaveActionButtons(Player* player, ActionButtonList& actionButtons);

private:
    void LoadPlayerSpec(uint32 playerId);
    uint8 GetPlayerActiveSpec(uint32 playerId) const;
    void SetPlayerActiveSpec(Player* player, uint8 spec);
    uint8 GetPlayerSpecCount(uint32 playerId) const;
    void SetPlayerSpecCount(Player* player, uint8 count);
    void SavePlayerSpec(uint32 playerId);

    void LoadPlayerSpecNames(Player* player);
    const std::string& GetPlayerSpecName(Player* player, uint8 spec) const;
    void SetPlayerSpecName(Player* player, uint8 spec, const std::string& name);
    void SavePlayerSpecNames(Player* player);

    void LoadPlayerTalents(Player* player);
    bool PlayerHasTalent(Player* player, uint32 spellId, uint8 spec);
    DualSpecPlayerTalentMap& GetPlayerTalents(uint32 playerId, int8 spec = -1);
    void AddPlayerTalent(uint32 playerId, uint32 spellId, uint8 spec, bool learned);
    void SavePlayerTalents(uint32 playerId);

    void SendPlayerActionButtons(const Player* player, bool clear) const;

    void ActivatePlayerSpec(Player* player, uint8 spec);
    void AddDualSpecItem(Player* player);

private:
    std::map<uint32, DualSpecPlayerTalentMap[MAX_TALENT_SPECS]> playersTalents;
    std::map<uint32, DualSpecPlayerStatus> playersStatus;
    std::map<uint32, std::string[MAX_TALENT_SPECS]> playersSpecNames;
};

inline DualSpecModule* dualSpecModule = new DualSpecModule();
#endif