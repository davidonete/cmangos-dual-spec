#include "DualspecModule.h"

#include "AI/ScriptDevAI/include/sc_gossip.h"
#include "Entities/GossipDef.h"
#include "Entities/ObjectGuid.h"
#include "Entities/Player.h"
#include "Globals/ObjectMgr.h"
#include "Spells/SpellMgr.h"

#ifdef ENABLE_PLAYERBOTS
#include "playerbot/PlayerbotAI.h"
#endif

namespace cmangos_module
{
    DualspecModule::DualspecModule()
    : Module("DualSpec", new DualSpecModuleConfig())
    {
        
    }

    const DualSpecModuleConfig* DualspecModule::GetConfig() const
    {
        return (DualSpecModuleConfig*)Module::GetConfig();
    }

    void DualspecModule::OnInitialize()
    {
        if (GetConfig()->enabled)
        {
            // Cleanup non existent characters
            CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_dualspec_talent`.`guid`);");
            CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent_name` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_dualspec_talent_name`.`guid`);");
            CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_action` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_dualspec_action`.`guid`);");
            CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_characters` WHERE NOT EXISTS (SELECT 1 FROM `characters` WHERE `characters`.`guid` = `custom_dualspec_characters`.`guid`);");
        }
    }

    bool DualspecModule::OnUseItem(Player* player, Item* item)
    {
        if (GetConfig()->enabled)
        {
            if (player && item)
            {
                // Check if using dual spec item
                if (item->GetEntry() != DUALSPEC_ITEM_ENTRY)
                    return false;

#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return false;
#endif

                const uint32 playerId = player->GetObjectGuid().GetCounter();
                player->GetPlayerMenu()->ClearMenus();

                if (player->IsInCombat())
                {
                    const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ERR_COMBAT);
                    player->GetSession()->SendNotification(msg.c_str());
                    return false;
                }

                if (player->GetMap()->IsBattleGround() || player->GetMap()->IsDungeon() || player->GetMap()->IsRaid())
                {
                    const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ERR_INSTANCE);
                    player->GetSession()->SendNotification(msg.c_str());
                    return false;
                }

                if (player->IsFlying() || player->IsTaxiFlying() || player->IsMounted())
                {
                    const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ERR_MOUNT);
                    player->GetSession()->SendNotification(msg.c_str());
                    return false;
                }

                if (player->IsDead())
                {
                    const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ERR_DEAD);
                    player->GetSession()->SendNotification(msg.c_str());
                    return false;
                }

                const uint8 specCount = GetPlayerSpecCount(playerId);
                if (specCount < MAX_TALENT_SPECS)
                {
                    const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ERR_UNLOCK);
                    player->GetSession()->SendNotification(msg.c_str());
                    return false;
                }

                if (player->GetLevel() < 10)
                {
                    const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ERR_LEVEL);
                    player->GetSession()->SendNotification(msg.c_str());
                    return false;
                }

                const uint8 activeSpec = GetPlayerActiveSpec(playerId);
                for (uint8 spec = 0; spec < specCount; ++spec)
                {
                    const std::string& specName = GetPlayerSpecName(player, spec);

                    std::stringstream specNameString;
                    specNameString << player->GetSession()->GetMangosString(DUAL_SPEC_ACTIVATE_COLOR);
                    specNameString << (specName.empty() ? player->GetSession()->GetMangosString(DUAL_SPEC_UNNAMED) : specName);
                    specNameString << (spec == activeSpec ? player->GetSession()->GetMangosString(DUAL_SPEC_ACTIVE) : "");
                    specNameString << "|r";

                    const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ARE_YOU_SURE_SWITCH);
                    player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_BATTLE, specNameString.str(), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + (1 + spec), msg, false);
                }

                for (uint8 spec = 0; spec < specCount; ++spec)
                {
                    const std::string& specName = GetPlayerSpecName(player, spec);

                    std::stringstream specNameString;
                    specNameString << player->GetSession()->GetMangosString(DUAL_SPEC_RENAME_COLOR);
                    specNameString << (specName.empty() ? player->GetSession()->GetMangosString(DUAL_SPEC_UNNAMED) : specName);
                    specNameString << (spec == activeSpec ? player->GetSession()->GetMangosString(DUAL_SPEC_ACTIVE) : "");
                    specNameString << "|r";

                    player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_BATTLE, specNameString.str(), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + (10 + spec), "", true);
                }

                player->GetPlayerMenu()->SendGossipMenu(DUALSPEC_ITEM_TEXT, item->GetObjectGuid());
                return true;
            }
        }

        return false;
    }

    bool DualspecModule::OnPreGossipHello(Player* player, Creature* creature)
    {
        if (GetConfig()->enabled)
        {
            if (player && creature)
            {
                // Check if speaking with dual spec npc
                if (creature->GetEntry() != DUALSPEC_NPC_ENTRY)
                    return false;

#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return false;
#endif

                const uint32 playerId = player->GetObjectGuid().GetCounter();
                player->GetPlayerMenu()->ClearMenus();

                const uint32 cost = GetConfig()->cost;
                const std::string costStr = std::to_string(cost > 0U ? cost / 10000U : 0U);
                const std::string areYouSure = player->GetSession()->GetMangosString(DUAL_SPEC_ARE_YOU_SURE_BEGIN) + costStr + player->GetSession()->GetMangosString(DUAL_SPEC_ARE_YOU_SURE_END);

                const uint8 specCount = GetPlayerSpecCount(playerId);
                if (specCount < MAX_TALENT_SPECS)
                {
                    // Display cost
                    const std::string purchase = player->GetSession()->GetMangosString(DUAL_SPEC_PURCHASE);
                    const std::string costIs = player->GetSession()->GetMangosString(DUAL_SPEC_COST_IS) + costStr + " g";
                    player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, purchase, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF, areYouSure, false);
                    player->GetPlayerMenu()->GetGossipMenu().AddMenuItem(GOSSIP_ICON_MONEY_BAG, costIs, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF, "", 0);
                }

                if (specCount > 1)
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

        return false;
    }

    bool DualspecModule::OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId)
    {
        if (GetConfig()->enabled)
        {
            if (player && creature)
            {
                // Check if speaking with dual spec npc
                if (creature->GetEntry() != DUALSPEC_NPC_ENTRY)
                    return false;

#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return false;
#endif

                const uint32 playerId = player->GetObjectGuid().GetCounter();
                player->GetPlayerMenu()->ClearMenus();

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
                }

                switch (action)
                {
                    case GOSSIP_ACTION_INFO_DEF:
                    {
                        if (player->GetMoney() >= GetConfig()->cost)
                        {
                            player->ModifyMoney(-int32(GetConfig()->cost));
                            SetPlayerSpecCount(player, GetPlayerSpecCount(playerId) + 1);
                            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5, "", gossipListId);
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
                        if (GetPlayerActiveSpec(playerId) == 0)
                        {
                            player->GetPlayerMenu()->CloseGossip();
                            player->GetSession()->SendNotification(player->GetSession()->GetMangosString(DUAL_SPEC_ALREADY_ON_SPEC));
                            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5, "", gossipListId);
                        }
                        else
                        {
                            ActivatePlayerSpec(player, 0);
                        }

                        break;
                    }

                    case GOSSIP_ACTION_INFO_DEF + 2:
                    {
                        if (GetPlayerActiveSpec(playerId) == 1)
                        {
                            player->GetPlayerMenu()->CloseGossip();
                            player->GetSession()->SendNotification(player->GetSession()->GetMangosString(DUAL_SPEC_ALREADY_ON_SPEC));
                            OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5, "", gossipListId);
                        }
                        else
                        {
                            ActivatePlayerSpec(player, 1);
                        }

                        break;
                    }

                    case GOSSIP_ACTION_INFO_DEF + 5:
                    {
                        const uint8 activeSpec = GetPlayerActiveSpec(playerId);
                        const uint8 specCount = GetPlayerSpecCount(playerId);
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

    bool DualspecModule::OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action, const std::string& code, uint32 gossipListId)
    {
        if (GetConfig()->enabled)
        {
            if (player)
            {
                // Check if using dual spec item
                if (item->GetEntry() != DUALSPEC_ITEM_ENTRY)
                    return false;

#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return false;
#endif

                const uint32 playerId = player->GetObjectGuid().GetCounter();
                player->GetPlayerMenu()->ClearMenus();

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
                }
                else
                {
                    switch (action)
                    {
                        case GOSSIP_ACTION_INFO_DEF + 1:
                        {
                            if (GetPlayerActiveSpec(playerId) == 0)
                            {
                                player->GetPlayerMenu()->CloseGossip();
                                const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ALREADY_ON_SPEC);
                                player->GetSession()->SendNotification(msg.c_str());
                            }
                            else
                            {
                                ActivatePlayerSpec(player, 0);
                            }
                        
                            break;
                        }

                        case GOSSIP_ACTION_INFO_DEF + 2:
                        {
                            if (GetPlayerActiveSpec(playerId) == 1)
                            {
                                player->GetPlayerMenu()->CloseGossip();
                                const std::string msg = player->GetSession()->GetMangosString(DUAL_SPEC_ALREADY_ON_SPEC);
                                player->GetSession()->SendNotification(msg.c_str());
                            }
                            else
                            {
                                ActivatePlayerSpec(player, 1);
                            }
                        
                            break;
                        }

                        case GOSSIP_ACTION_INFO_DEF + 999:
                        {
                            player->GetPlayerMenu()->CloseGossip();
                            break;
                        }

                        default: break;
                    }
                }

                return true;
            }
        }

        return false;
    }

    void DualspecModule::OnLearnTalent(Player* player, uint32 spellId)
    {
        if (GetConfig()->enabled)
        {
            if (player)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif

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
                AddPlayerTalent(playerId, spellId, GetPlayerActiveSpec(playerId), true);
            }
        }
    }

    void DualspecModule::OnResetTalents(Player* player, uint32 cost)
    {
        if (GetConfig()->enabled)
        {
            if (player)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif

                const uint32 playerId = player->GetObjectGuid().GetCounter();
                DualSpecPlayerTalentMap* playerTalents = GetPlayerTalents(playerId, -1, false);
                if (playerTalents)
                {
                    for (auto& playerTalentsPair : *playerTalents)
                    {
                        const uint32 spellId = playerTalentsPair.first;
                        DualspecPlayerTalent& playerTalent = playerTalentsPair.second;
                        playerTalent.state = PLAYERSPELL_REMOVED;
                    }

                    SavePlayerTalents(playerId);
                }
                else
                {
                    // The player talents have been reset before a complete login
                    // Try to get the current active spec 
                    uint8 activeSpec = 0;
                    auto result = CharacterDatabase.PQuery("SELECT `active_spec` FROM `custom_dualspec_characters` WHERE `guid` = '%u';", playerId);
                    if (result)
                    {
                        Field* fields = result->Fetch();
                        activeSpec = fields[0].GetUInt8();
                    }

                    // Remove the talents from the reset spec
                    CharacterDatabase.DirectPExecute("DELETE FROM `custom_dualspec_talent` WHERE `guid` = '%u' and `spec` = '%u';", playerId, activeSpec);
                }
            }
        }
    }

    void DualspecModule::OnPreLoadFromDB(uint32 playerId)
    {
        if (GetConfig()->enabled)
        {
#ifdef ENABLE_PLAYERBOTS
            if (sRandomPlayerbotMgr.IsFreeBot(playerId))
                return;
#endif

            LoadPlayerSpec(playerId);
        }
    }

    void DualspecModule::OnLoadFromDB(Player* player)
    {
        if (GetConfig()->enabled)
        {
#ifdef ENABLE_PLAYERBOTS
            if (sRandomPlayerbotMgr.IsFreeBot(player))
                return;
#endif

            LoadPlayerTalents(player);
            LoadPlayerSpecNames(player);
        }
    }

    void DualspecModule::OnSaveToDB(Player* player)
    {
        if (GetConfig()->enabled)
        {
#ifdef ENABLE_PLAYERBOTS
            if (sRandomPlayerbotMgr.IsFreeBot(player))
                return;
#endif

            const uint32 playerId = player->GetObjectGuid().GetCounter();
            SavePlayerTalents(playerId);
            SavePlayerSpec(playerId);
            SavePlayerSpecNames(player);
        }
    }

    void DualspecModule::OnDeleteFromDB(uint32 playerId)
    {
        if (GetConfig()->enabled)
        {
#ifdef ENABLE_PLAYERBOTS
            if (sRandomPlayerbotMgr.IsFreeBot(playerId))
                return;
#endif

            CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent` WHERE `guid` = '%u';", playerId);
            CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent_name` WHERE `guid` = '%u';", playerId);
            CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_action` WHERE `guid` = '%u';", playerId);
            CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_characters` WHERE `guid` = '%u';", playerId);
        }
    }

    void DualspecModule::OnLogOut(Player* player)
    {
        if (GetConfig()->enabled)
        {
            if (player)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif

                const uint32 playerId = player->GetObjectGuid().GetCounter();
                playersTalents.erase(playerId);
                playersStatus.erase(playerId);
                playersSpecNames.erase(playerId);
            }
        }
    }

    void DualspecModule::OnCharacterCreated(Player* player)
    {
        if (GetConfig()->enabled)
        {
            if (player)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return;
#endif

                // Create the default data
                const uint32 playerId = player->GetObjectGuid().GetCounter();
                playersTalents[playerId];
                playersSpecNames[playerId];
                playersStatus[playerId] = { 1, 0 };
            }
        }
    }

    bool DualspecModule::OnLoadActionButtons(Player* player, ActionButtonList& actionButtons)
    {
        if (GetConfig()->enabled)
        {
            if (player)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return false;
#endif

                const uint32 playerId = player->GetObjectGuid().GetCounter();
                const uint8 activeSpec = GetPlayerActiveSpec(playerId);
            
                auto result = CharacterDatabase.PQuery("SELECT `button`, `action`, `type`, `spec` FROM `custom_dualspec_action` WHERE guid = '%u' ORDER BY `button`;", playerId);
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
                else
                {
                    // Missing buttons for new character, create it from current config
                    CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_action` (`guid`, `spec`, `button`, `action`, `type`) SELECT `guid`, 0 AS `spec`, `button`, `action`, `type` FROM `character_action` WHERE `character_action`.`guid` = '%u';", playerId);
                }
            }
        }

        return false;
    }

    bool DualspecModule::OnSaveActionButtons(Player* player, ActionButtonList& actionButtons)
    {
        if (GetConfig()->enabled)
        {
            if (player)
            {
#ifdef ENABLE_PLAYERBOTS
                if (sRandomPlayerbotMgr.IsFreeBot(player))
                    return false;
#endif

                const uint32 playerId = player->GetObjectGuid().GetCounter();
                const uint8 activeSpec = GetPlayerActiveSpec(playerId);

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

    void DualspecModule::LoadPlayerSpec(uint32 playerId)
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

        // Add row to db if not found
        auto playerStatusIt = playersStatus.find(playerId);
        if (playerStatusIt == playersStatus.end())
        {
            playersStatus[playerId] = { 1, 0 };
            CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_characters` (`guid`) VALUES ('%u');", playerId);
        }
    }

    uint8 DualspecModule::GetPlayerActiveSpec(uint32 playerId) const
    {
        auto playerStatusIt = playersStatus.find(playerId);
        if (playerStatusIt != playersStatus.end())
        {
            return playerStatusIt->second.activeSpec;
        }

        MANGOS_ASSERT(false);
        return 0;
    }

    void DualspecModule::SetPlayerActiveSpec(Player* player, uint8 spec)
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

    uint8 DualspecModule::GetPlayerSpecCount(uint32 playerId) const
    {
        auto playerStatusIt = playersStatus.find(playerId);
        if (playerStatusIt != playersStatus.end())
        {
            return playerStatusIt->second.specCount;
        }

        MANGOS_ASSERT(false);
        return 1;
    }

    void DualspecModule::SetPlayerSpecCount(Player* player, uint8 count)
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

    void DualspecModule::SavePlayerSpec(uint32 playerId)
    {
        CharacterDatabase.PExecute("UPDATE `custom_dualspec_characters` SET `spec_count` = '%u', `active_spec` = '%u' WHERE `guid` = '%u';",
            GetPlayerSpecCount(playerId),
            GetPlayerActiveSpec(playerId),
            playerId
        );
    }

    void DualspecModule::LoadPlayerSpecNames(Player* player)
    {
        if (player)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();
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
    }

    const std::string& DualspecModule::GetPlayerSpecName(Player* player, uint8 spec) const
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
    }

    void DualspecModule::SetPlayerSpecName(Player* player, uint8 spec, const std::string& name)
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

    void DualspecModule::SavePlayerSpecNames(Player* player)
    {
        if (player)
        {
            bool deleteOld = true;
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            for (uint8 spec = 0; spec < MAX_TALENT_SPECS; spec++)
            {
                const std::string& specName = GetPlayerSpecName(player, spec);
                if (!specName.empty())
                {
                    if (deleteOld)
                    {
                        CharacterDatabase.PExecute("DELETE FROM `custom_dualspec_talent_name` WHERE `guid` = '%u';", playerId);
                        deleteOld = false;
                    }

                    CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_talent_name` (`guid`, `spec`, `name`) VALUES ('%u', '%u', '%s');", 
                        playerId, 
                        spec,
                        specName.c_str()
                    );
                }
            }
        }
    }

    void DualspecModule::LoadPlayerTalents(Player* player)
    {
        if (player)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();
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

            // Add rows to db if not found
            auto playerTalentsIt = playersTalents.find(playerId);
            if (playerTalentsIt == playersTalents.end())
            {
                const uint8 spec = 0;
                DualSpecPlayerTalentMap& playerTalents = playersTalents[playerId][spec];
        
                for (uint32 talentId = 0; talentId < sTalentStore.GetNumRows(); talentId++)
                {
                    TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);
                    if (!talentInfo)
                        continue;

                    TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
                    if (!talentTabInfo)
                        continue;

                    if ((player->getClassMask() & talentTabInfo->ClassMask) == 0)
                        continue;

                    for (uint8 rank = 0; rank < MAX_TALENT_RANK; ++rank)
                    {
                        if (talentInfo->RankID[rank] != 0)
                        {
                            if (player->HasSpell(talentInfo->RankID[rank]))
                            {
                                AddPlayerTalent(playerId, talentInfo->RankID[rank], spec, true);
                            }
                        }
                    }
                }

                SavePlayerTalents(playerId);
            }
        }
    }

    bool DualspecModule::PlayerHasTalent(Player* player, uint32 spellId, uint8 spec)
    {
        if (player)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            DualSpecPlayerTalentMap* playerTalents = GetPlayerTalents(playerId, spec);
            if (playerTalents)
            {
                auto it = playerTalents->find(spellId);
                if (it != playerTalents->end())
                {
                    return it->second.state != PLAYERSPELL_REMOVED;
                }
            }
        }

        return false;
    }

    DualSpecPlayerTalentMap* DualspecModule::GetPlayerTalents(uint32 playerId, int8 spec, bool assert)
    {
        auto playerTalentSpecIt = playersTalents.find(playerId);
        if (playerTalentSpecIt != playersTalents.end())
        {
            spec = spec >= 0 ? spec : GetPlayerActiveSpec(playerId);
            return &playerTalentSpecIt->second[spec];
        }

        if (assert)
        {
            MANGOS_ASSERT(false);
        }
        
        return nullptr;
    }

    void DualspecModule::AddPlayerTalent(uint32 playerId, uint32 spellId, uint8 spec, bool learned)
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

    void DualspecModule::SavePlayerTalents(uint32 playerId)
    {
        for (uint8 i = 0; i < MAX_TALENT_SPECS; ++i)
        {
            DualSpecPlayerTalentMap* playerTalents = GetPlayerTalents(playerId, i);
            if (playerTalents)
            {
                for (auto playerTalentsIt = playerTalents->begin(); playerTalentsIt != playerTalents->end();)
                {
                    const uint32 spellId = playerTalentsIt->first;
                    DualspecPlayerTalent& playerTalent = playerTalentsIt->second;

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
                        playerTalents->erase(playerTalentsIt++);
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

    void DualspecModule::SendPlayerActionButtons(const Player* player, bool clear) const
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

    void DualspecModule::ActivatePlayerSpec(Player* player, uint8 spec)
    {
        if (player)
        {
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            if (GetPlayerActiveSpec(playerId) == spec)
                return;

            if (spec > GetPlayerSpecCount(playerId))
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

    void DualspecModule::AddDualSpecItem(Player* player)
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
}