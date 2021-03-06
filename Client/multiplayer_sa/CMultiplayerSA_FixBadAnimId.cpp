/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        multiplayer_sa/CMultiplayerSA_FixBadAnimId.cpp
 *  PORPOISE:
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include "../game_sa/CAnimBlendAssocGroupSA.h"

constexpr CAnimBlendAssocGroupSAInterface* getAnimAssocGroupInterface(AssocGroupId animGroup)
{
    DWORD* pAnimAssocGroupsArray = reinterpret_cast<DWORD*>(*(DWORD*)0xb4ea34);
    return reinterpret_cast<CAnimBlendAssocGroupSAInterface*>(pAnimAssocGroupsArray + 5 * animGroup);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Check for anims that will crash and change to one that wont. (The new anim will be wrong and look crap though)
int _cdecl OnCAnimBlendAssocGroupCopyAnimation(AssocGroupId animGroup, int iAnimId)
{
    auto pGroup = getAnimAssocGroupInterface(animGroup);

    // Apply offset
    int iUseAnimId = iAnimId - pGroup->iIDOffset;

    if (pGroup->pAssociationsArray)
    {
        CAnimBlendStaticAssociationSAInterface* pAssociation = pGroup->pAssociationsArray + iUseAnimId;
        if (pAssociation && pAssociation->pAnimHeirarchy == NULL)
        {
            // Choose another animId
            int iNewAnimId = iUseAnimId;
            for (int i = 0; i < pGroup->iNumAnimations; i++)
            {
                pAssociation = pGroup->pAssociationsArray + i;
                if (pAssociation->pAnimHeirarchy)
                {
                    // Find closest valid anim id
                    if (abs(iUseAnimId - i) < abs(iUseAnimId - iNewAnimId) || iNewAnimId == iUseAnimId)
                        iNewAnimId = i;
                }
            }

            iUseAnimId = iNewAnimId;
            LogEvent(534, "CopyAnimation", "", SString("Group:%d replaced id:%d with id:%d", pGroup->groupID, iAnimId, iUseAnimId + pGroup->iIDOffset));
        }
    }

    // Unapply offset
    iAnimId = iUseAnimId + pGroup->iIDOffset;
    return iAnimId;
}

// Hook info
#define HOOKPOS_CAnimBlendAssocGroupCopyAnimation        0x4CE130
#define HOOKSIZE_CAnimBlendAssocGroupCopyAnimation       6
DWORD RETURN_CAnimBlendAssocGroupCopyAnimation = 0x4CE136;
void _declspec(naked) HOOK_CAnimBlendAssocGroupCopyAnimation()
{
    _asm
    {
        pushad
        push    [esp+32+4*1]
        push    ecx
        call    OnCAnimBlendAssocGroupCopyAnimation
        add     esp, 4*2
        mov     [esp+32+4*1],eax
        popad

        mov     eax,  fs:0
        jmp     RETURN_CAnimBlendAssocGroupCopyAnimation
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Check for invalid RpHAnimHierarchy*
void _cdecl OnGetAnimHierarchyFromSkinClump(RpClump* pRpClump, void* pRpHAnimHierarchyResult)
{
    if (pRpHAnimHierarchyResult == nullptr)
    {
        // Crash will occur at offset 003C51A8
        uint uiModelId = pGameInterface->GetPools()->GetModelIdFromClump(pRpClump);
        LogEvent(818, "Model bad anim hierarchy", "GetAnimHierarchyFromSkinClump", SString("Corrupt model:%d", uiModelId), 5418);
        CArgMap argMap;
        argMap.Set("id", uiModelId);
        argMap.Set("reason", "anim_hierarchy");
        SetApplicationSetting("diagnostics", "gta-model-fail", argMap.ToString());
    }
}

#define HOOKPOS_GetAnimHierarchyFromSkinClump        0x734A5D
#define HOOKSIZE_GetAnimHierarchyFromSkinClump       7
DWORD RETURN_GetAnimHierarchyFromSkinClump = 0x734A64;
void _declspec(naked) HOOK_GetAnimHierarchyFromSkinClump()
{
    _asm
    {
        pushad
        push    [esp+32+0x0C]       // RpHAnimHierarchy* (return value)
        push    [esp+32+4+0x14]     // RpClump*
        call    OnGetAnimHierarchyFromSkinClump
        add     esp, 4*2
        popad

        mov     eax, [esp+0x0C]
        add     esp, 10h
        jmp     RETURN_GetAnimHierarchyFromSkinClump
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Setup hooks
//
//////////////////////////////////////////////////////////////////////////////////////////
void CMultiplayerSA::InitHooks_FixBadAnimId(void)
{
    EZHookInstall(GetAnimHierarchyFromSkinClump);
}
