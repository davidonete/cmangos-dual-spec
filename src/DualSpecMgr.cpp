#include "DualSpecMgr.h"

#include "Entities/ObjectGuid.h"
#include "Entities/Player.h"
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

bool DualSpecMgr::OnPlayerGossipSelect(Player* player, const ObjectGuid& guid, uint32 sender, uint32 action)
{
    // TO DO: Move this to generic module system once done
    if (player)
    {
        if (guid.IsAnyTypeCreature())
        {
            Creature* creature = player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
            if (creature)
            {
                return OnPlayerGossipSelect(player, creature, sender, action);
            }
        }
        else if (guid.IsGameObject())
        {
            GameObject* gameObject = player->GetGameObjectIfCanInteractWith(guid);
            if (gameObject)
            {
                return OnPlayerGossipSelect(player, gameObject, sender, action);
            }
        }
        else if (guid.IsItem())
        {
            Item* item = player->GetItemByGuid(guid);
            if (item)
            {
                return OnPlayerGossipSelect(player, item, sender, action);
            }
        }
    }

    return false;
}

bool DualSpecMgr::OnPlayerGossipSelect(Player* player, Unit* creature, uint32 sender, uint32 action)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {

        }
    }

    return false;
}

bool DualSpecMgr::OnPlayerGossipSelect(Player* player, GameObject* gameObject, uint32 sender, uint32 action)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {

        }
    }

    return false;
}

bool DualSpecMgr::OnPlayerGossipSelect(Player* player, Item* item, uint32 sender, uint32 action)
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
        AddPlayerTalent(player, spellId, GetPlayerActiveSpec(player), true);
    }
}

void DualSpecMgr::OnPlayerResetTalents(Player* player, uint32 cost)
{
    if (sDualSpecConfig.enabled)
    {
        if (player)
        {
            DualSpecPlayerTalentMap& playerTalents = GetPlayerTalents(player);
            for (unsigned int i = 0; i < sTalentStore.GetNumRows(); ++i)
            {
                TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
                if (!talentInfo)
                    continue;

                TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
                if (!talentTabInfo)
                    continue;

                if ((player->getClassMask() & talentTabInfo->ClassMask) == 0)
                    continue;

                for (unsigned int j : talentInfo->RankID)
                {
                    if (j)
                    {
                        auto talentIt = playerTalents.find(j);
                        if (talentIt != playerTalents.end())
                        {
                            talentIt->second.state = PLAYERSPELL_REMOVED;
                        }
                    }
                }
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
            // Copy the default character action buttons into custom_dualspec_action
            const uint32 playerId = player->GetObjectGuid().GetCounter();
            CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_action` (`guid`, `spec`, `button`, `action`, `type`) SELECT `guid`, 0 AS `spec`, `button`, `action`, `type` FROM `character_action` WHERE `guid` = '%u';", playerId);
    
            // Create custom_dualspec_characters row
            CharacterDatabase.PExecute("INSERT INTO `custom_dualspec_characters` (`guid`) VALUES ('%u');", playerId);
        }
    }
}

void DualSpecMgr::OnPlayerLoadFromDB(Player* player)
{
    if (sDualSpecConfig.enabled)
    {
        LoadPlayerTalents(player);
        LoadPlayerSpec(player);
    }
}

void DualSpecMgr::OnPlayerSaveToDB(Player* player)
{
    if (sDualSpecConfig.enabled)
    {
        SavePlayerTalents(player);
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

void DualSpecMgr::LoadPlayerSpec(Player* player)
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto result = CharacterDatabase.PQuery("SELECT `spec_count`, `active_spec` FROM custom_dualspec_characters` WHERE `guid` = '%u';", playerId);
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

void DualSpecMgr::LoadPlayerTalents(Player* player)
{
    if (player)
    {
        const uint32 playerId = player->GetObjectGuid().GetCounter();
        auto result = CharacterDatabase.PQuery("SELECT `spell`, `spec` FROM `custom_dualspec_talent` WHERE guid = '%u';", playerId);
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                const uint32 spellId = fields[0].GetUInt32();
                const uint8 spec = fields[1].GetUInt8();
                AddPlayerTalent(player, spellId, spec, false);
            } 
            while (result->NextRow());
        }
    }
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

void DualSpecMgr::AddPlayerTalent(Player* player, uint32 spellId, uint8 spec, bool learned)
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
