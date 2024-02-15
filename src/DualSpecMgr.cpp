#include "DualSpecMgr.h"

#include "AI/ScriptDevAI/include/sc_gossip.h"
#include "Entities/GossipDef.h"
#include "Entities/ObjectGuid.h"
#include "Entities/Player.h"
#include "Globals/ObjectMgr.h"
#include "Spells/SpellMgr.h"

void DualSpecMgr::Init()
{
    sDualSpecConfig.Initialize();

    if (sDualSpecConfig.enabled)
    {
        // Cleanup non existent characters
        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_dualspec_talent`.`guid`);");
        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent_name` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_dualspec_talent_name`.`guid`);");
        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_action` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_dualspec_action`.`guid`);");
        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_characters` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_dualspec_characters`.`guid`);");

        // Add current characters
        CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_characters` (`guid`) SELECT `guid` FROM `characters` WHERE NOT EXISTS (SELECT 1 FROM `custom_dualspec_characters` WHERE `custom_dualspec_characters`.`guid` = `characters`.`guid`);");
        CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_action` (`guid`, `spec`, `button`, `action`, `type`) SELECT `guid`, 0 AS `spec`, `button`, `action`, `type` FROM `character_action` WHERE NOT EXISTS (SELECT 1 FROM `custom_dualspec_action` WHERE `custom_dualspec_action`.`guid` = `character_action`.`guid`);");
    }
}

bool DualSpecMgr::OnPlayerGossipHello(Player* player, Creature* creature)
{
    if (sDualSpecConfig.enabled)
    {
        if (player && creature)
        {
            // Check if speaking with dual spec npc
            if (creature->GetEntry() != DUALSPEC_NPC_ENTRY)
                return false;

            const uint32 cost = sDualSpecConfig.cost;
            const std::string costStr = std::to_string(cost > 0U ? cost / 10000U : 0U);
            const std::string areYouSure = player->GetSession()->GetMangosString(DUAL_SPEC_ARE_YOU_SURE_BEGIN) + costStr + player->GetSession()->GetMangosString(DUAL_SPEC_ARE_YOU_SURE_END);

            const uint8 specCount = GetPlayerSpecCount(player);
            if (specCount < MAX_TALENT_SPECS)
            {
                // Display cost
                const std::string purchase = player->GetSession()->GetMangosString(DUAL_SPEC_PURCHASE);
                const std::string costIs = player->GetSession()->GetMangosString(DUAL_SPEC_COST_IS) + costStr + " g";
                player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, purchase, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF, areYouSure, false);
                player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, costIs, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF, "", 0);
            }
            else
            {
                const std::string changeSpec = player->GetSession()->GetMangosString(DUAL_SPEC_CHANGE_MY_SPEC);
                player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, changeSpec, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5, "", 0);
                if (!player->GetItemCount(DUALSPEC_ITEM_ENTRY, true))
                {
                    AddDualSpecItem(player);
                }
            }

            player->GetPlayerMenu()->SendGossipMenu(DUALSPEC_NPC_TEXT, creature->GetObjectGuid());
            return true;
        }
    }
}

bool DualSpecMgr::OnPlayerGossipSelect(Player* player, const ObjectGuid& guid, uint32 sender, uint32 action, const std::string& code)
{
    // TO DO: Move this to generic module system once done
    if (player)
    {
        if (guid.IsAnyTypeCreature())
        {
            Creature* creature = player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
            if (creature)
            {
                return OnPlayerGossipSelect(player, creature, sender, action, code);
            }
        }
        else if (guid.IsGameObject())
        {
            GameObject* gameObject = player->GetGameObjectIfCanInteractWith(guid);
            if (gameObject)
            {
                return OnPlayerGossipSelect(player, gameObject, sender, action, code);
            }
        }
        else if (guid.IsItem())
        {
            Item* item = player->GetItemByGuid(guid);
            if (item)
            {
                return OnPlayerGossipSelect(player, item, sender, action, code);
            }
        }
    }

    return false;
}

bool DualSpecMgr::OnPlayerGossipSelect(Player* player, Unit* creature, uint32 sender, uint32 action, const std::string& code)
{
    if (sDualSpecConfig.enabled)
    {
        if (player && creature)
        {
            // Check if speaking with dual spec npc
            if (creature->GetEntry() != DUALSPEC_NPC_ENTRY)
                return false;

            if (!code.empty())
            {
                std::string strCode = code;
                CharacterDatabase.escape_string(strCode);

                if (action == GOSSIP_ACTION_INFO_DEF + 10)
                {
                    SetPlayerSpecName(player, 0, strCode);
                }
                else if (action == GOSSIP_ACTION_INFO_DEF + 11)
                {
                    SetPlayerSpecName(player, 1, strCode);
                }

                player->GetPlayerMenu()->CloseGossip();

                OnPlayerGossipSelect(player, creature, sender, action, "");
            }

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF:
                {
                    if (player->GetMoney() >= sDualSpecConfig.cost)
                    {
                        player->ModifyMoney(-int32(sDualSpecConfig.cost));
                        SetPlayerSpecCount(player, GetPlayerSpecCount(player) + 1);
                        OnPlayerGossipSelect(player, creature, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5, "");
                        AddDualSpecItem(player);
                    }
                    else
                    {
                        const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_NO_GOLD_UNLOCK);
                        player->GetSession()->SendNotification(msg.c_str());
                    }

                    break;
                }

                case GOSSIP_ACTION_INFO_DEF + 1:
                {
                    if (GetPlayerActiveSpec(player) == 0)
                    {
                        player->GetPlayerMenu()->CloseGossip();
                        player->GetSession()->SendNotification(player->GetSession()->GetMangosString(DUAL_SPEC_ALREADY_ON_SPEC));
                        OnPlayerGossipSelect(player, creature, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5, "");
                    }
                    else
                    {
                        ActivatePlayerSpec(player, 0);
                    }

                    break;
                }

                case GOSSIP_ACTION_INFO_DEF + 2:
                {
                    if (GetPlayerActiveSpec(player) == 1)
                    {
                        player->GetPlayerMenu()->CloseGossip();
                        player->GetSession()->SendNotification(player->GetSession()->GetMangosString(DUAL_SPEC_ALREADY_ON_SPEC));
                        OnPlayerGossipSelect(player, creature, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5, "");
                    }
                    else
                    {
                        ActivatePlayerSpec(player, 1);
                    }

                    break;
                }

                case GOSSIP_ACTION_INFO_DEF + 5:
                {
                    const uint8 activeSpec = GetPlayerActiveSpec(player);
                    const uint8 specCount = GetPlayerSpecCount(player);
                    for (uint8 spec = 0; spec < specCount; ++spec)
                    {
                        const std::string& specName = GetPlayerSpecName(player, spec);

                        std::stringstream specNameString;
                        specNameString << player->GetSession()->GetMangosString(DUAL_SPEC_ACTIVATE);
                        specNameString << (specName.empty() ? player->GetSession()->GetMangosString(DUAL_SPEC_UNNAMED) : specName);
                        specNameString << (spec == activeSpec ? player->GetSession()->GetMangosString(DUAL_SPEC_ACTIVE) : "");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_CHAT, specNameString.str(), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + (1 + spec), "", 0);
                    }

                    for (uint8 spec = 0; spec < specCount; ++spec)
                    {
                        const std::string& specName = GetPlayerSpecName(player, spec);

                        std::stringstream specNameString;
                        specNameString << player->GetSession()->GetMangosString(DUAL_SPEC_RENAME);
                        specNameString << (specName.empty() ? player->GetSession()->GetMangosString(DUAL_SPEC_UNNAMED) : specName);
                        specNameString << (spec == activeSpec ? player->GetSession()->GetMangosString(DUAL_SPEC_ACTIVE) : "");
                        player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_TALK, specNameString.str(), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + (10 + spec), "", true);
                    }

                    player->GetPlayerMenu()->SendGossipMenu(DUALSPEC_NPC_TEXT, creature->GetObjectGuid());
                    break;
                }
            }

            return true;
        }
    }

    return false;
}

bool DualSpecMgr::OnPlayerGossipSelect(Player* player, GameObject* gameObject, uint32 sender, uint32 action, const std::string& code)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {

        }
    }

    return false;
}

bool DualSpecMgr::OnPlayerGossipSelect(Player* player, Item* item, uint32 sender, uint32 action, const std::string& code)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {

        }
    }

    return false;
}

void DualSpecMgr::OnPlayerLearnTalent(Player* player, uint32 spellId)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {
            SpellEntry const* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(spellId);
            if (!spellInfo)
            {
                sLog.outDetail("Player::addTalent: Non-existed in SpellStore spell #%u request.", spellId);
                return;
            }

            if (!sSpellMgr.IsSpellValid(spellInfo, player, false))
            {
                sLog.outDetail("Player::addTalent: Broken spell #%u learning not allowed.", spellId);
                return;
            }

            const uint32 playerId = player->GetObjectGuid().GetCounter();
            AddPlayerTalent(playerId, spellId, GetPlayerActiveSpec(player), true);
        }
    }
}

void DualSpecMgr::OnPlayerResetTalents(Player* player, uint32 cost)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {
            DualSpecPlayerTalentMap& playerTalents = GetPlayerTalents(player);
            for (auto& playerTalentsPair : playerTalents)
            {
                const uint32 spellId = playerTalentsPair.first;
                DualSpecPlayerTalent& playerTalent = playerTalentsPair.second;
                playerTalent.state = PLAYERSPELL_REMOVED;
            }

            SavePlayerTalents(player);
        }
    }
}

void DualSpecMgr::OnPlayerCharacterCreated(Player* player)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();

            // Create custom_dualspec_characters row
            CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_characters` (`guid`) VALUES ('%u');", playerId);

            // Create the default data
            playersTalents[playerId];
            playersStatus[playerId] = { 1, 0 };
        }
    }
}

void DualSpecMgr::OnPlayerLogIn(uint32 playerId)
{
    if (sDualSpecConfig.enabled)
    {
        LoadPlayerTalents(playerId);
        LoadPlayerSpec(playerId);
        LoadPlayerSpecNames(playerId);
    }
}

void DualSpecMgr::OnPlayerLogOut(Player* player)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            playersTalents.erase(playerId);
            playersStatus.erase(playerId);
            playersSpecNames.erase(playerId);
        }
    }
}

void DualSpecMgr::OnPlayerSaveToDB(Player* player)
{
    if (sDualSpecConfig.enabled)
    {
        SavePlayerTalents(player);
        SavePlayerSpec(player);
        SavePlayerSpecNames(player);
    }
}

void DualSpecMgr::OnPlayerCharacterDeleted(uint32 playerId)
{
    if (sDualSpecConfig.enabled)
    {
        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent` WHERE `guid` = '%u';", playerId);
        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent_name` WHERE `guid` = '%u';", playerId);
        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_action` WHERE `guid` = '%u';", playerId);
        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_characters` WHERE `guid` = '%u';", playerId);
    }
}

bool DualSpecMgr::OnPlayerLoadActionButtons(Player* player, ActionButtonList& actionButtons)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {
            const uint8 activeSpec = GetPlayerActiveSpec(player);
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            auto result = CharacterDatabase.PQuery("SELECT button, action, type, spec FROM custom_dualspec_action WHERE guid = '%u' ORDER BY button;", playerId);
            if (result)
            {
                actionButtons.clear();

                do
                {
                    Field* fields = result->Fetch();
                    const uint8 button = fields[0].GetUInt8();
                    const uint32 action = fields[1].GetUInt32();
                    const uint8 type = fields[2].GetUInt8();
                    const uint8 spec = fields[3].GetUInt8();

                    if (spec == activeSpec)
                    {
                        if (ActionButton* ab = player->addActionButton(button, action, type))
                        {
                            ab->uState = ACTIONBUTTON_UNCHANGED;
                        }
                        else
                        {
                            actionButtons[button].uState = ACTIONBUTTON_DELETED;
                        }
                    }
                } 
                while (result->NextRow());

                return true;
            }
        }
    }

    return false;
}

bool DualSpecMgr::OnPlayerSaveActionButtons(Player* player, ActionButtonList& actionButtons)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            const uint8 activeSpec = GetPlayerActiveSpec(player);

            for (auto actionButtonIt = actionButtons.begin(); actionButtonIt != actionButtons.end();)
            {
                const uint8 buttonId = actionButtonIt->first;
                ActionButton& button = actionButtonIt->second;
                switch (button.uState)
                {
                    case ACTIONBUTTON_NEW:
                    {
                        CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_action` (`guid`, `button`, `action`, `type`, `spec`) VALUES ('%u', '%u', '%u', '%u', '%u');",
                            playerId,
                            buttonId,
                            button.GetAction(),
                            button.GetType(),
                            activeSpec
                        );

                        button.uState = ACTIONBUTTON_UNCHANGED;
                        ++actionButtonIt;
                        break;
                    }

                    case ACTIONBUTTON_CHANGED:
                    {
                        CharacterDatabase.PExecute("UPDATE `custom_dualspec_action` SET `action` = '%u', `type` = '%u' WHERE `guid` = '%u' AND `button` = '%u' AND `spec` = '%u';",
                            button.GetAction(),
                            button.GetType(),
                            playerId,
                            buttonId,
                            activeSpec
                        );

                        button.uState = ACTIONBUTTON_UNCHANGED;
                        ++actionButtonIt;
                        break;
                    }

                    case ACTIONBUTTON_DELETED:
                    {
                        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_action` WHERE `guid` = '%u' AND `button` = '%u' AND `spec` = '%u';",
                            playerId,
                            buttonId,
                            activeSpec
                        );

                        actionButtons.erase(actionButtonIt++);
                        break;
                    }

                    default:
                    {
                        ++actionButtonIt;
                        break;
                    }
                }
            }
        }
    }

    // Return false to also save the buttons to the default character table 
    // in case of disabling the dual spec system
    return false;
}

void DualSpecMgr::LoadPlayerSpec(uint32 playerId)
{
    auto result = CharacterDatabase.PQuery("SELECT `spec_count`, `active_spec` FROM `custom_dualspec_characters` WHERE `guid` = '%u';", playerId);
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            const uint8 specCount = fields[0].GetUInt8();
            const uint8 activeSpec = fields[1].GetUInt8();
            playersStatus[playerId] = { specCount, activeSpec };
        } 
        while (result->NextRow());
    }
}

uint8 DualSpecMgr::GetPlayerActiveSpec(Player* player) const
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto playerStatusIt = playersStatus.find(playerId);
        if (playerStatusIt != playersStatus.end())
        {
            return playerStatusIt->second.activeSpec;
        }
    }

    MANGOS_ASSERT(false);
    return 0;
}

void DualSpecMgr::SetPlayerActiveSpec(Player* player, uint8 spec)
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto playerStatusIt = playersStatus.find(playerId);
        if (playerStatusIt != playersStatus.end())
        {
            playerStatusIt->second.activeSpec = spec;
        }
        else
        {
            MANGOS_ASSERT(false);
        }
    }
}

uint8 DualSpecMgr::GetPlayerSpecCount(Player* player) const
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto playerStatusIt = playersStatus.find(playerId);
        if (playerStatusIt != playersStatus.end())
        {
            return playerStatusIt->second.specCount;
        }
    }

    MANGOS_ASSERT(false);
    return 1;
}

void DualSpecMgr::SetPlayerSpecCount(Player* player, uint8 count)
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto playerStatusIt = playersStatus.find(playerId);
        if (playerStatusIt != playersStatus.end())
        {
            playerStatusIt->second.specCount = count;
        }
        else
        {
            MANGOS_ASSERT(false);
        }
    }
}

void DualSpecMgr::SavePlayerSpec(Player* player)
{
    if (player)
    {
        CharacterDatabase.PExecute("UPDATE `custom_dualspec_characters` SET `spec_count` = '%u', `active_spec` = '%u' WHERE `guid` = '%u';",
            GetPlayerSpecCount(player),
            GetPlayerActiveSpec(player),
            player->GetObjectGuid().GetCounter()
        );
    }
}

void DualSpecMgr::LoadPlayerSpecNames(uint32 playerId)
{
    auto& playerSpecNames = playersSpecNames[playerId];
    auto result = CharacterDatabase.PQuery("SELECT `spec`, `name` FROM `custom_dualspec_talent_name` WHERE `guid` = '%u';", playerId);
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            const uint8 spec = fields[0].GetUInt8();
            const std::string name = fields[1].GetCppString();
            playerSpecNames[spec] = name;
        } 
        while (result->NextRow());
    }
}

const std::string& DualSpecMgr::GetPlayerSpecName(Player* player, uint8 spec) const
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto playerSpecNamesIt = playersSpecNames.find(playerId);
        if (playerSpecNamesIt != playersSpecNames.end())
        {
            return playerSpecNamesIt->second[spec];
        }
    }

    MANGOS_ASSERT(false);
    return "";
}

void DualSpecMgr::SetPlayerSpecName(Player* player, uint8 spec, const std::string& name)
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto playerSpecNamesIt = playersSpecNames.find(playerId);
        if (playerSpecNamesIt != playersSpecNames.end())
        {
            playerSpecNamesIt->second[spec] = name;
        }
        else
        {
            MANGOS_ASSERT(false);
        }
    }
}

void DualSpecMgr::SavePlayerSpecNames(Player* player)
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        for (uint8 spec = 0; spec < MAX_TALENT_SPECS; spec++)
        {
            const std::string& specName = GetPlayerSpecName(player, spec);
            if (!specName.empty())
            {
                CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent_name` WHERE `guid` = '%u';", playerId);
                CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_talent_name` (`guid`, `spec`, `name`) VALUES ('%u', '%u', '%s')", 
                    playerId, 
                    spec,
                    specName.c_str()
                );
            }
        }
    }
}

void DualSpecMgr::LoadPlayerTalents(uint32 playerId)
{
    // Create player talents container for later use
    playersTalents[playerId];

    auto result = CharacterDatabase.PQuery("SELECT `spell`, `spec` FROM `custom_dualspec_talent` WHERE `guid` = '%u';", playerId);
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            const uint32 spellId = fields[0].GetUInt32();
            const uint8 spec = fields[1].GetUInt8();
            AddPlayerTalent(playerId, spellId, spec, false);
        } 
        while (result->NextRow());
    }
}

bool DualSpecMgr::PlayerHasTalent(Player* player, uint32 spellId, uint8 spec)
{
    if (player)
    {
        DualSpecPlayerTalentMap& playerTalents = GetPlayerTalents(player, spec);
        auto it = playerTalents.find(spellId);
        if (it != playerTalents.end())
        {
            return it->second.state != PLAYERSPELL_REMOVED;
        }
    }

    return false;
}

DualSpecPlayerTalentMap& DualSpecMgr::GetPlayerTalents(Player* player, int8 spec)
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto playerTalentSpecIt = playersTalents.find(playerId);
        if (playerTalentSpecIt != playersTalents.end())
        {
            spec = spec >= 0 ? spec : GetPlayerActiveSpec(player);
            return playerTalentSpecIt->second[spec];
        }
    }

    MANGOS_ASSERT(false);
    return playersTalents[0][0];
}

void DualSpecMgr::AddPlayerTalent(uint32 playerId, uint32 spellId, uint8 spec, bool learned)
{
    auto& playerTalents = playersTalents[playerId][spec];

    auto talentIt = playerTalents.find(spellId);
    if (talentIt != playerTalents.end())
    {
        talentIt->second.state = PLAYERSPELL_UNCHANGED;
    }
    else if (TalentSpellPos const* talentPos = GetTalentSpellPos(spellId))
    {
        if (TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentPos->talent_id))
        {
            for (uint8 rank = 0; rank < MAX_TALENT_RANK; ++rank)
            {
                // skip learning spell and no rank spell case
                uint32 rankSpellId = talentInfo->RankID[rank];
                if (!rankSpellId || rankSpellId == spellId)
                {
                    continue;
                }

                talentIt = playerTalents.find(rankSpellId);
                if (talentIt != playerTalents.end())
                {
                    talentIt->second.state = PLAYERSPELL_REMOVED;
                }
            }
        }

        const uint8 state = learned ? PLAYERSPELL_NEW : PLAYERSPELL_UNCHANGED;
        playerTalents[spellId] = { state, spec };
    }
}

void DualSpecMgr::SavePlayerTalents(Player* player)
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        for (uint8 i = 0; i < MAX_TALENT_SPECS; ++i)
        {
            DualSpecPlayerTalentMap& playerTalents = GetPlayerTalents(player, i);
            for (auto playerTalentsIt = playerTalents.begin(); playerTalentsIt != playerTalents.end();)
            {
                const uint32 spellId = playerTalentsIt->first;
                DualSpecPlayerTalent& playerTalent = playerTalentsIt->second;

                if (playerTalent.state == PLAYERSPELL_REMOVED || playerTalent.state == PLAYERSPELL_CHANGED)
                {
                    CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent` WHERE `guid` = '%u' and `spell` = '%u' and `spec` = '%u';",
                        playerId,
                        spellId,
                        playerTalent.spec
                    );
                }

                if (playerTalent.state == PLAYERSPELL_NEW || playerTalent.state == PLAYERSPELL_CHANGED)
                {
                    CharacterDatabase.PExecute("INSERT INTO custom_dualspec_talent (`guid`, `spell`, `spec`) VALUES ('%u', '%u', '%u');",
                        playerId,
                        spellId,
                        playerTalent.spec
                    );
                }

                if (playerTalent.state == PLAYERSPELL_REMOVED)
                {
                    playerTalents.erase(playerTalentsIt++);
                }
                else
                {
                    playerTalent.state = PLAYERSPELL_UNCHANGED;
                    ++playerTalentsIt;
                }
            }
        }
    }
}

void DualSpecMgr::SendPlayerActionButtons(const Player* player, bool clear) const
{
    if (player)
    {
        if (clear)
        {
            WorldPacket data(SMSG_ACTION_BUTTONS, (MAX_ACTION_BUTTONS * 4));
            data << uint32(0);
            player->GetSession()->SendPacket(data);
        }
        else
        {
            player->SendInitialActionButtons();
        }
    }
}

void DualSpecMgr::ActivatePlayerSpec(Player* player, uint8 spec)
{
    if (player)
    {
        if (GetPlayerActiveSpec(player) == spec)
            return;

        if (spec > GetPlayerSpecCount(player))
            return;

        if (player->IsNonMeleeSpellCasted(false))
        {
            player->InterruptNonMeleeSpells(false);
        }

        // Save current Actions
        player->SaveToDB();

        // Clear action bars
        SendPlayerActionButtons(player, true);

        // TO-DO: We need more research to know what happens with warlock's reagent
        if (Pet* pet = player->GetPet())
        {
            player->RemovePet(PET_SAVE_NOT_IN_SLOT);
        }

        player->ClearComboPointHolders();
        player->ClearAllReactives();
        player->UnsummonAllTotems();

        // REMOVE TALENTS
        for (uint32 talentId = 0; talentId < sTalentStore.GetNumRows(); talentId++)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);
            if (!talentInfo)
                continue;

            TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
            if (!talentTabInfo)
                continue;

            // unlearn only talents for character class
            // some spell learned by one class as normal spells or know at creation but another class learn it as talent,
            // to prevent unexpected lost normal learned spell skip another class talents
            if ((player->getClassMask() & talentTabInfo->ClassMask) == 0)
                continue;

            for (int8 rank = 0; rank < MAX_TALENT_RANK; rank++)
            {
                for (PlayerSpellMap::iterator itr = player->GetSpellMap().begin(); itr != player->GetSpellMap().end();)
                {
                    if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.disabled || itr->first == 33983 || itr->first == 33982 || itr->first == 33986 || itr->first == 33987) // skip mangle rank 2 and 3
                    {
                        ++itr;
                        continue;
                    }

                    // remove learned spells (all ranks)
                    uint32 itrFirstId = sSpellMgr.GetFirstSpellInChain(itr->first);

                    // unlearn if first rank is talent or learned by talent
                    if (itrFirstId == talentInfo->RankID[rank] || sSpellMgr.IsSpellLearnToSpell(talentInfo->RankID[rank], itrFirstId))
                    {
                        player->removeSpell(itr->first, true);
                        itr = player->GetSpellMap().begin();
                        continue;
                    }
                    else
                    {
                        ++itr;
                    }
                }
            }
        }

        SetPlayerActiveSpec(player, spec);
        uint32 spentTalents = 0;

        // Add Talents
        for (uint32 talentId = 0; talentId < sTalentStore.GetNumRows(); talentId++)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);
            if (!talentInfo)
                continue;

            TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
            if (!talentTabInfo)
                continue;

            // Learn only talents for character class
            if ((player->getClassMask() & talentTabInfo->ClassMask) == 0)
                continue;

            for (int8 rank = 0; rank < MAX_TALENT_RANK; rank++)
            {
                // Skip non-existent talent ranks
                if (talentInfo->RankID[rank] == 0)
                    continue;

                // If the talent can be found in the newly activated PlayerTalentMap
                if (PlayerHasTalent(player, talentInfo->RankID[rank], spec))
                {
                    // Ensure both versions of druid mangle spell are properly relearned
                    if (talentInfo->RankID[rank] == 33917) 
                    {
                        player->learnSpell(33876, false, true);         // Mangle (Cat) (Rank 1)
                        player->learnSpell(33878, false, true);         // Mangle (Bear) (Rank 1)
                    }

                    player->learnSpell(talentInfo->RankID[rank], false, true);
                    spentTalents += (rank + 1);             // increment the spentTalents count
                }
            }
        }

        //m_usedTalentCount = spentTalents;
        //player->InitTalentForLevel();
        {
            uint32 level = player->GetLevel();
            // talents base at level diff ( talents = level - 9 but some can be used already)
            if (level < 10)
            {
                // Remove all talent points
                if (spentTalents > 0)                          // Free any used talents
                {
                    player->resetTalents(true);
                    player->SetFreeTalentPoints(0);
                }
            }
            else
            {
                uint32 talentPointsForLevel = player->CalculateTalentsPoints();

                // if used more that have then reset
                if (spentTalents > talentPointsForLevel)
                {
                    if (player->GetSession()->GetSecurity() < SEC_ADMINISTRATOR)
                    {
                        player->resetTalents(true);
                    }
                    else
                    {
                        player->SetFreeTalentPoints(0);
                    }
                }
                // else update amount of free points
                else
                {
                    player->SetFreeTalentPoints(talentPointsForLevel - spentTalents);
                }
            }
        }

        // Load new Action Bar
        //QueryResult* actionResult = CharacterDatabase.PQuery("SELECT button, action, type FROM character_action WHERE guid = '%u' AND spec = '%u' ORDER BY button", GetGUIDLow(), m_activeSpec);
        //_LoadActions(actionResult);

        //SendActionButtons(1);
        // Need to relog player ???: TODO fix packet sending
        player->GetSession()->LogoutPlayer();
    }
}

void DualSpecMgr::AddDualSpecItem(Player* player)
{
    if (player)
    {
        WorldSession* session = player->GetSession();
        ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(DUALSPEC_ITEM_ENTRY);
        if (!pProto)
        {
            session->SendAreaTriggerMessage("%s", DUAL_SPEC_ERR_ITEM_CREATE);
            return;
        }

        // Adding items
        uint32 count = 1;
        uint32 noSpaceForCount = 0;

        // Check space and find places
        ItemPosCountVec dest;
        uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, DUALSPEC_ITEM_ENTRY, count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)
        {
            count -= noSpaceForCount;
        }

        if (count == 0 || dest.empty())
        {
            session->SendAreaTriggerMessage("%s", DUAL_SPEC_ERR_ITEM_CREATE);
            return;
        }

        Item* item = player->StoreNewItem(dest, DUALSPEC_ITEM_ENTRY, true, Item::GenerateItemRandomPropertyId(DUALSPEC_ITEM_ENTRY));
        if (count > 0 && item)
        {
            player->SendNewItem(item, count, false, true);
        }

        if (noSpaceForCount > 0)
        {
            session->SendAreaTriggerMessage("%s", DUAL_SPEC_ERR_ITEM_CREATE);
        }
    }
}