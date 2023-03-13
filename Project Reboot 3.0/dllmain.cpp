#include <Windows.h>
#include <iostream>

#include "FortGameModeAthena.h"
#include "reboot.h"
#include "finder.h"
#include "hooking.h"
#include "GameSession.h"
#include "FortPlayerControllerAthena.h"
#include "AbilitySystemComponent.h"
#include "FortPlayerPawn.h"
#include "globals.h"
#include "FortInventoryInterface.h"

#include "Map.h"
#include "events.h"
#include "FortKismetLibrary.h"
#include "vehicles.h"
#include "UObjectArray.h"
#include "BuildingTrap.h"
#include "commands.h"

enum ENetMode
{
    NM_Standalone,
    NM_DedicatedServer,
    NM_ListenServer,
    NM_Client,
    NM_MAX,
};

static ENetMode GetNetModeHook() { /* std::cout << "AA!\n"; */ return ENetMode::NM_DedicatedServer; }
static ENetMode GetNetModeHook2() { /* std::cout << "AA!\n"; */ return ENetMode::NM_DedicatedServer; }

static bool ReturnTrueHook() { return true; }
static int Return2Hook() { return 2; }

static void NoMCPHook() { return; }
static void CollectGarbageHook() { return; }

static __int64 (*DispatchRequestOriginal)(__int64 a1, __int64* a2, int a3);
static __int64 DispatchRequestHook(__int64 a1, __int64* a2, int a3) { return DispatchRequestOriginal(a1, a2, 3); }

DWORD WINAPI Main(LPVOID)
{
    InitLogger();

    std::cin.tie(0);
    std::cout.tie(0);
    std::ios_base::sync_with_stdio(false);

    auto MH_InitCode = MH_Initialize();

    if (MH_InitCode != MH_OK)
    {
        LOG_ERROR(LogInit, "Failed to initialize MinHook {}!", MH_StatusToString(MH_InitCode));
        return 1;
    }

    LOG_INFO(LogInit, "Initializing Project Reboot!");

    Addresses::SetupVersion();

    Offsets::FindAll(); // We have to do this before because FindCantBuild uses FortAIController.CreateBuildingActor
    Offsets::Print();

    Addresses::FindAll();
    // Addresses::Print();
    Addresses::Init();
    Addresses::Print();

    static auto GameModeDefault = FindObject<UClass>(L"/Script/FortniteGame.Default__FortGameModeAthena");
    static auto FortPlayerControllerAthenaDefault = FindObject<UClass>(L"/Script/FortniteGame.Default__FortPlayerControllerAthena"); // FindObject<UClass>(L"/Game/Athena/Athena_PlayerController.Default__Athena_PlayerController_C");
    static auto FortPlayerPawnAthenaDefault = FindObject<UClass>(L"/Game/Athena/PlayerPawn_Athena.Default__PlayerPawn_Athena_C");
    static auto FortAbilitySystemComponentAthenaDefault = FindObject<UClass>(L"/Script/FortniteGame.Default__FortAbilitySystemComponentAthena");
    static auto FortKismetLibraryDefault = FindObject<UClass>(L"/Script/FortniteGame.Default__FortKismetLibrary");

    static auto SwitchLevel = FindObject<UFunction>(L"/Script/Engine.PlayerController.SwitchLevel");
    FString Level = Engine_Version < 424
        ? L"Athena_Terrain" : Engine_Version >= 500 ? Engine_Version >= 501
        ? L"Asteria_Terrain"
        : L"Artemis_Terrain"
        : Globals::bCreative ? L"Creative_NoApollo_Terrain" 
        : L"Apollo_Terrain";

    if (Globals::bNoMCP)
    {
        if (Hooking::MinHook::Hook((PVOID)Addresses::NoMCP, (PVOID)NoMCPHook, nullptr))
        {
            Hooking::MinHook::Hook((PVOID)Addresses::GetNetMode, (PVOID)GetNetModeHook, nullptr);
        }
    }
    else
    {
        Hooking::MinHook::Hook((PVOID)Addresses::DispatchRequest, (PVOID)DispatchRequestHook, (PVOID*)&DispatchRequestOriginal);
        Hooking::MinHook::Hook((PVOID)Addresses::GetNetMode, (PVOID)GetNetModeHook, nullptr);
    }

    Hooking::MinHook::Hook((PVOID)Addresses::KickPlayer, (PVOID)AGameSession::KickPlayerHook, (PVOID*)&AGameSession::KickPlayerOriginal);

    LOG_INFO(LogDev, "Size: 0x{:x}", sizeof(TMap<FName, void*>));

    GetLocalPlayerController()->ProcessEvent(SwitchLevel, &Level);

    LOG_INFO(LogPlayer, "Switched level.");

    Hooking::MinHook::Hook((PVOID)Addresses::ActorGetNetMode, (PVOID)GetNetModeHook2, nullptr);

    LOG_INFO(LogDev, "FindGIsServer: 0x{:x}", FindGIsServer() - __int64(GetModuleHandleW(0)));
    LOG_INFO(LogDev, "FindGIsClient: 0x{:x}", FindGIsClient() - __int64(GetModuleHandleW(0)));

    // if (false)
    {
        if (FindGIsServer())
            *(bool*)FindGIsServer() = true;

        if (FindGIsClient())
            *(bool*)FindGIsClient() = false;
    }

    if (Fortnite_Version == 17.30)
    {
        // Hooking::MinHook::Hook((PVOID)(__int64(GetModuleHandleW(0)) + 0x3E07910), (PVOID)Return2Hook, nullptr);
        // Hooking::MinHook::Hook((PVOID)(__int64(GetModuleHandleW(0)) + 0x3DED12C), (PVOID)ReturnTrueHook, nullptr);
        Hooking::MinHook::Hook((PVOID)(__int64(GetModuleHandleW(0)) + 0x3DED158), (PVOID)ReturnTrueHook, nullptr);
    }

    auto& LocalPlayers = GetLocalPlayers();

    if (LocalPlayers.Num() && LocalPlayers.Data)
    {
        LocalPlayers.Remove(0);
    }

    for (auto func : Addresses::GetFunctionsToNull())
    {
        if (func == 0)
            continue;

        DWORD dwProtection;
        VirtualProtect((PVOID)func, 1, PAGE_EXECUTE_READWRITE, &dwProtection);

        *(uint8_t*)func = 0xC3;

        DWORD dwTemp;
        VirtualProtect((PVOID)func, 1, dwProtection, &dwTemp);
    }

    // return false;

    // UNetDriver::ReplicationDriverOffset = FindOffsetStruct("/Script/Engine.NetDriver", "ReplicationDriver"); // NetDriver->GetOffset("ReplicationDriver");

    Hooking::MinHook::Hook(GameModeDefault, FindObject<UFunction>(L"/Script/Engine.GameMode.ReadyToStartMatch"), AFortGameModeAthena::Athena_ReadyToStartMatchHook,
       (PVOID*)&AFortGameModeAthena::Athena_ReadyToStartMatchOriginal, false);

    // return false;

    Hooking::MinHook::Hook(GameModeDefault, FindObject<UFunction>(L"/Script/Engine.GameModeBase.SpawnDefaultPawnFor"),
        AGameModeBase::SpawnDefaultPawnForHook, nullptr, false);
    Hooking::MinHook::Hook(GameModeDefault, FindObject<UFunction>(L"/Script/Engine.GameModeBase.HandleStartingNewPlayer"), AFortGameModeAthena::Athena_HandleStartingNewPlayerHook,
        (PVOID*)&AFortGameModeAthena::Athena_HandleStartingNewPlayerOriginal, false);

    static auto ControllerServerAttemptInteractFn = FindObject<UFunction>("/Script/FortniteGame.FortPlayerController.ServerAttemptInteract");

    if (ControllerServerAttemptInteractFn)
    {
        Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, ControllerServerAttemptInteractFn, AFortPlayerController::ServerAttemptInteractHook,
            (PVOID*)&AFortPlayerController::ServerAttemptInteractOriginal, false, true);
    }
    else
    {
        Hooking::MinHook::Hook(FindObject("/Script/FortniteGame.Default__FortControllerComponent_Interaction"),
            FindObject<UFunction>("/Script/FortniteGame.FortControllerComponent_Interaction.ServerAttemptInteract"),
            AFortPlayerController::ServerAttemptInteractHook, (PVOID*)&AFortPlayerController::ServerAttemptInteractOriginal, false, true);
    }

    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/Engine.PlayerController.ServerAcknowledgePossession"),
        AFortPlayerControllerAthena::ServerAcknowledgePossessionHook, nullptr, false);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ServerAttemptInventoryDrop"),
        AFortPlayerController::ServerAttemptInventoryDropHook, nullptr, false);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ServerCheat"),
        ServerCheatHook, nullptr, false);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ServerExecuteInventoryItem"),
       AFortPlayerController::ServerExecuteInventoryItemHook, nullptr, false);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ServerPlayEmoteItem"),
       AFortPlayerController::ServerPlayEmoteItemHook, nullptr, false);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ServerCreateBuildingActor"), 
        AFortPlayerController::ServerCreateBuildingActorHook, (PVOID*)&AFortPlayerController::ServerCreateBuildingActorOriginal, false, true);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ServerBeginEditingBuildingActor"),
        AFortPlayerController::ServerBeginEditingBuildingActorHook, nullptr, false);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ServerEditBuildingActor"),
        AFortPlayerController::ServerEditBuildingActorHook, nullptr, false);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ServerEndEditingBuildingActor"),
        AFortPlayerController::ServerEndEditingBuildingActorHook, nullptr, false);
    Hooking::MinHook::Hook(FortPlayerControllerAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerControllerAthena.ServerPlaySquadQuickChatMessage"),
        AFortPlayerControllerAthena::ServerPlaySquadQuickChatMessage, nullptr, false);

    Hooking::MinHook::Hook(FortPlayerPawnAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerPawn.ServerSendZiplineState"),
        AFortPlayerPawn::ServerSendZiplineStateHook, nullptr, false);

    // if (false)
    if (Addresses::FrameStep)
    {
        Hooking::MinHook::Hook(FortKismetLibraryDefault, FindObject<UFunction>(L"Script/FortniteGame.FortKismetLibrary.K2_GiveItemToPlayer"),
           UFortKismetLibrary::K2_GiveItemToPlayerHook, (PVOID*)&UFortKismetLibrary::K2_GiveItemToPlayerOriginal, false, true);
        Hooking::MinHook::Hook(FortKismetLibraryDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortKismetLibrary.GiveItemToInventoryOwner"),
            UFortKismetLibrary::GiveItemToInventoryOwnerHook, (PVOID*)&UFortKismetLibrary::GiveItemToInventoryOwnerOriginal, false, true);
        Hooking::MinHook::Hook(FortKismetLibraryDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortKismetLibrary.K2_RemoveItemFromPlayerByGuid"),
            UFortKismetLibrary::K2_RemoveItemFromPlayerByGuidHook, (PVOID*)&UFortKismetLibrary::K2_RemoveItemFromPlayerByGuidOriginal, false, true);
        Hooking::MinHook::Hook(FortKismetLibraryDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortKismetLibrary.K2_RemoveItemFromPlayer"),
           UFortKismetLibrary::K2_RemoveItemFromPlayerHook, (PVOID*)&UFortKismetLibrary::K2_RemoveItemFromPlayerOriginal, false, true);
        Hooking::MinHook::Hook(FortKismetLibraryDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortKismetLibrary.K2_RemoveFortItemFromPlayer"),
            UFortKismetLibrary::K2_RemoveFortItemFromPlayerHook, (PVOID*)&UFortKismetLibrary::K2_RemoveFortItemFromPlayerOriginal, false, true);
        Hooking::MinHook::Hook(FortKismetLibraryDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortKismetLibrary.K2_SpawnPickupInWorld"),
            UFortKismetLibrary::K2_SpawnPickupInWorldHook, (PVOID*)&UFortKismetLibrary::K2_SpawnPickupInWorldOriginal, false, true);
    }

    static auto ServerHandlePickupInfoFn = FindObject<UFunction>("/Script/FortniteGame.FortPlayerPawn.ServerHandlePickupInfo");

    if (ServerHandlePickupInfoFn)
    {
        Hooking::MinHook::Hook(FortPlayerPawnAthenaDefault, ServerHandlePickupInfoFn, AFortPlayerPawn::ServerHandlePickupInfoHook, nullptr, false);
    }
    else
    {
        Hooking::MinHook::Hook(FortPlayerPawnAthenaDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerPawn.ServerHandlePickup"),
            AFortPlayerPawn::ServerHandlePickupHook, nullptr, false);
    }

    if (Globals::bAbilitiesEnabled)
    {
        static auto PredictionKeyStruct = FindObject<UStruct>("/Script/GameplayAbilities.PredictionKey");
        static auto PredictionKeySize = PredictionKeyStruct->GetPropertiesSize();

        if (PredictionKeySize == 0x10)
        {
            Hooking::MinHook::Hook(FortAbilitySystemComponentAthenaDefault, FindObject<UFunction>(L"/Script/GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbility"),
                UAbilitySystemComponent::ServerTryActivateAbilityHook1, nullptr, false);
            Hooking::MinHook::Hook(FortAbilitySystemComponentAthenaDefault, FindObject<UFunction>(L"/Script/GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbilityWithEventData"),
                UAbilitySystemComponent::ServerTryActivateAbilityWithEventDataHook1, nullptr, false);
        }
        else if (PredictionKeySize == 0x18)
        {
            Hooking::MinHook::Hook(FortAbilitySystemComponentAthenaDefault, FindObject<UFunction>(L"/Script/GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbility"),
                UAbilitySystemComponent::ServerTryActivateAbilityHook2, nullptr, false);
            Hooking::MinHook::Hook(FortAbilitySystemComponentAthenaDefault, FindObject<UFunction>(L"/Script/GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbilityWithEventData"),
                UAbilitySystemComponent::ServerTryActivateAbilityWithEventDataHook2, nullptr, false);
        }

        // Hooking::MinHook::Hook(FortAbilitySystemComponentAthenaDefault, FindObject<UFunction>(L"/Script/GameplayAbilities.AbilitySystemComponent.ServerAbilityRPCBatch"),
            // UAbilitySystemComponent::ServerAbilityRPCBatchHook, nullptr, false);
    }

    if (Engine_Version >= 424)
    {
        static auto FortControllerComponent_AircraftDefault = FindObject<UClass>(L"/Script/FortniteGame.Default__FortControllerComponent_Aircraft");

        Hooking::MinHook::Hook(FortControllerComponent_AircraftDefault, FindObject<UFunction>(L"/Script/FortniteGame.FortControllerComponent_Aircraft.ServerAttemptAircraftJump"),
            AFortPlayerController::ServerAttemptAircraftJumpHook, nullptr, false);
    }

    Hooking::MinHook::Hook((PVOID)Addresses::GetPlayerViewpoint, (PVOID)AFortPlayerControllerAthena::GetPlayerViewPointHook, (PVOID*)&AFortPlayerControllerAthena::GetPlayerViewPointOriginal);
    Hooking::MinHook::Hook((PVOID)Addresses::TickFlush, (PVOID)UNetDriver::TickFlushHook, (PVOID*)&UNetDriver::TickFlushOriginal);

    if (Engine_Version < 427)
        Hooking::MinHook::Hook((PVOID)Addresses::OnDamageServer, (PVOID)ABuildingActor::OnDamageServerHook, (PVOID*)&ABuildingActor::OnDamageServerOriginal);
    
    // Hooking::MinHook::Hook((PVOID)Addresses::CollectGarbage, (PVOID)CollectGarbageHook, nullptr);
    Hooking::MinHook::Hook((PVOID)Addresses::PickTeam, (PVOID)AFortGameModeAthena::Athena_PickTeamHook);
    // Hooking::MinHook::Hook((PVOID)Addresses::SetZoneToIndex, (PVOID)AFortGameModeAthena::SetZoneToIndexHook, (PVOID*)&AFortGameModeAthena::SetZoneToIndexOriginal);
    Hooking::MinHook::Hook((PVOID)Addresses::CompletePickupAnimation, (PVOID)AFortPickup::CompletePickupAnimationHook, (PVOID*)&AFortPickup::CompletePickupAnimationOriginal);
    Hooking::MinHook::Hook((PVOID)Addresses::CanActivateAbility, ReturnTrueHook); // ahhh wtf
    // Hooking::MinHook::Hook((PVOID)FindFunctionCall(L"ServerRemoveInventoryItem"), UFortInventoryInterface::RemoveInventoryItemHook);

    AddVehicleHook();

    LOG_INFO(LogDev, "Test: 0x{:x}", FindFunctionCall(L"ClientOnPawnDied") - __int64(GetModuleHandleW(0)));
    Hooking::MinHook::Hook((PVOID)FindFunctionCall(L"ClientOnPawnDied"), AFortPlayerController::ClientOnPawnDiedHook, (PVOID*)&AFortPlayerController::ClientOnPawnDiedOriginal);

    {
        MemberOffsets::FortPlayerStateAthena::DeathInfo = FindOffsetStruct("/Script/FortniteGame.FortPlayerStateAthena", "DeathInfo");

        MemberOffsets::DeathInfo::bDBNO = FindOffsetStruct("/Script/FortniteGame.DeathInfo", "bDBNO");
        MemberOffsets::DeathInfo::DeathCause = FindOffsetStruct("/Script/FortniteGame.DeathInfo", "DeathCause");
        MemberOffsets::DeathInfo::bInitialized = FindOffsetStruct("/Script/FortniteGame.DeathInfo", "bInitialized", false);
        MemberOffsets::DeathInfo::Distance = FindOffsetStruct("/Script/FortniteGame.DeathInfo", "Distance");
        MemberOffsets::DeathInfo::DeathTags = FindOffsetStruct("/Script/FortniteGame.DeathInfo", "DeathTags", false);
        MemberOffsets::DeathInfo::DeathLocation = FindOffsetStruct("/Script/FortniteGame.DeathInfo", "DeathLocation");

        MemberOffsets::DeathReport::Tags = FindOffsetStruct("/Script/FortniteGame.FortPlayerDeathReport", "Tags");
        MemberOffsets::DeathReport::KillerPawn = FindOffsetStruct("/Script/FortniteGame.FortPlayerDeathReport", "KillerPawn");
        MemberOffsets::DeathReport::KillerPlayerState = FindOffsetStruct("/Script/FortniteGame.FortPlayerDeathReport", "KillerPlayerState");
        MemberOffsets::DeathReport::DamageCauser = FindOffsetStruct("/Script/FortniteGame.FortPlayerDeathReport", "DamageCauser");
    }

    /* auto GetMaxTickRateIndex = *Memcury::Scanner::FindStringRef(L"GETMAXTICKRATE")
        .ScanFor({ 0x4D, 0x8B, 0xC7, 0xE8 })
        .RelativeOffset(4)
        .ScanFor({ 0xFF, 0x90 })
        .AbsoluteOffset(2)
        .GetAs<int*>() / 8;

    LOG_INFO(LogHook, "GetMaxTickRateIndex {}", GetMaxTickRateIndex); */



    srand(time(0));

    LOG_INFO(LogHook, "Finished!");

    while (true)
    {
        if (GetAsyncKeyState(VK_F7) & 1)
        {
            LOG_INFO(LogEvent, "Starting {} event!", GetEventName());
            StartEvent();
        }

        else if (GetAsyncKeyState(VK_F8) & 1)
        {
            float Duration = 0;
            float EarlyDuration = Duration;

            float TimeSeconds = 0; // UGameplayStatics::GetTimeSeconds(GetWorld());

            LOG_INFO(LogDev, "Starting bus!");

            auto GameMode = (AFortGameMode*)GetWorld()->GetGameMode();
            auto GameState = GameMode->GetGameState();

            GameState->Get<float>("WarmupCountdownEndTime") = 0;
            GameMode->Get<float>("WarmupCountdownDuration") = 0;

            GameState->Get<float>("WarmupCountdownStartTime") = 0;
            GameMode->Get<float>("WarmupEarlyCountdownDuration") = 0;
        }

        /* else if (GetAsyncKeyState(VK_F9) & 1)
        {
            GetWorld()->Listen();
        } */
        
        Sleep(1000 / 30);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

