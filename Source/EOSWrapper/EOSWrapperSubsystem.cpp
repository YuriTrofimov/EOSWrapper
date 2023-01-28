// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOSWrapperSubsystem.h"
#include "EOSWrapperUserManager.h"
#include "eos_sdk.h"

DEFINE_LOG_CATEGORY(EOSWrapperSubsystem);

#pragma optimize("", off)

FEOSWrapperSubsystem::FEOSWrapperSubsystem(FName InInstanceName) : FOnlineSubsystemImpl(EOS_SUBSYSTEM, InInstanceName)
{
	StopTicker();
}

void FEOSWrapperSubsystem::ModuleInit() {}

void FEOSWrapperSubsystem::ModuleShutdown() {}

bool FEOSWrapperSubsystem::Init()
{
	if (bInitialized) return false;

	SDKManager = MakeUnique<FWrapperSDKManager>();
	check(SDKManager);
	SDKManager->Initialize();

	AuthHandle = EOS_Platform_GetAuthInterface(SDKManager->GetPlatformHandle());
	check(AuthHandle != nullptr);

	ConnectHandle = EOS_Platform_GetConnectInterface(SDKManager->GetPlatformHandle());
	check(ConnectHandle != nullptr);

	UserManager = MakeShareable(new FEOSWrapperUserManager(this));
	UserManager->Initialize();

	bInitialized = true;
	return true;
}

bool FEOSWrapperSubsystem::Shutdown()
{
	if (!bInitialized) return false;

	if (SDKManager)
	{
		SDKManager->Shutdown();
		SDKManager.Reset();
	}

	if (UserManager)
	{
		UserManager->Shutdown();
		UserManager.Reset();
	}

	bInitialized = false;
	return true;
}

FString FEOSWrapperSubsystem::GetAppId() const
{
	return TEXT("");
}

IOnlineSessionPtr FEOSWrapperSubsystem::GetSessionInterface() const
{
	return nullptr;
}

IOnlineFriendsPtr FEOSWrapperSubsystem::GetFriendsInterface() const
{
	return nullptr;
}

IOnlineSharedCloudPtr FEOSWrapperSubsystem::GetSharedCloudInterface() const
{
	return nullptr;
}

IOnlineUserCloudPtr FEOSWrapperSubsystem::GetUserCloudInterface() const
{
	return nullptr;
}

IOnlineEntitlementsPtr FEOSWrapperSubsystem::GetEntitlementsInterface() const
{
	return nullptr;
}

IOnlineLeaderboardsPtr FEOSWrapperSubsystem::GetLeaderboardsInterface() const
{
	return nullptr;
}

IOnlineVoicePtr FEOSWrapperSubsystem::GetVoiceInterface() const
{
	return nullptr;
}

IOnlineExternalUIPtr FEOSWrapperSubsystem::GetExternalUIInterface() const
{
	return nullptr;
}

IOnlineIdentityPtr FEOSWrapperSubsystem::GetIdentityInterface() const
{
	return UserManager;
}

IOnlineTitleFilePtr FEOSWrapperSubsystem::GetTitleFileInterface() const
{
	return nullptr;
}

IOnlineStoreV2Ptr FEOSWrapperSubsystem::GetStoreV2Interface() const
{
	return nullptr;
}

IOnlinePurchasePtr FEOSWrapperSubsystem::GetPurchaseInterface() const
{
	return nullptr;
}

IOnlineAchievementsPtr FEOSWrapperSubsystem::GetAchievementsInterface() const
{
	return nullptr;
}

IOnlineUserPtr FEOSWrapperSubsystem::GetUserInterface() const
{
	return nullptr;
}

IOnlinePresencePtr FEOSWrapperSubsystem::GetPresenceInterface() const
{
	return nullptr;
}

FText FEOSWrapperSubsystem::GetOnlineServiceName() const
{
	// return NSLOCTEXT("EOSWrapper", "OnlineServiceName", "EOSWrapper");
	return NSLOCTEXT("EOSWrapper", "OnlineServiceName", "EOS");
}

IOnlineStatsPtr FEOSWrapperSubsystem::GetStatsInterface() const
{
	return nullptr;
}

bool FEOSWrapperSubsystem::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return FOnlineSubsystemImpl::Exec(InWorld, Cmd, Ar);
}

void FEOSWrapperSubsystem::ReloadConfigs(const TSet<FString>& ConfigSections)
{
	FOnlineSubsystemImpl::ReloadConfigs(ConfigSections);
}

#pragma optimize("", on)