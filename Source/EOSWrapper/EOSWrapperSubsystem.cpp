// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOSWrapperSubsystem.h"

#include "EOSWrapperSettings.h"
#include "EOSWrapperUserManager.h"
#include "eos_sdk.h"

DEFINE_LOG_CATEGORY(EOSWrapperSubsystem);

#pragma optimize("", off)

FEOSWrapperSubsystem::FEOSWrapperSubsystem(FName InInstanceName)
	: FOnlineSubsystemImpl(EOS_SUBSYSTEM, InInstanceName)
{
	StopTicker();
}

void FEOSWrapperSubsystem::ModuleInit()
{
}

void FEOSWrapperSubsystem::ModuleShutdown()
{
}

bool FEOSWrapperSubsystem::Init()
{
	if (bInitialized) return false;

	// Determine if we are the default and if we're the platform OSS
	FString DefaultOSS;
	GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("DefaultPlatformService"), DefaultOSS, GEngineIni);
	FString PlatformOSS;
	GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("NativePlatformService"), PlatformOSS, GEngineIni);
	bIsDefaultOSS = DefaultOSS == TEXT("EOS");
	bIsPlatformOSS = PlatformOSS == TEXT("EOS");

	// Check for being launched by EGS
	bWasLaunchedByEGS = FParse::Param(FCommandLine::Get(), TEXT("EpicPortal"));
	const FEOSWrapperSettings EOSSettings = UEOSWrapperSettings::GetSettings();
	if (!IsRunningDedicatedServer() && IsRunningGame() && !bWasLaunchedByEGS && EOSSettings.bShouldEnforceBeingLaunchedByEGS)
	{
		FString ArtifactName;
		FParse::Value(FCommandLine::Get(), TEXT("EpicApp="), ArtifactName);
		UE_LOG_ONLINE(Warning, TEXT("FOnlineSubsystemEOS::Init() relaunching artifact (%s) via the store"), *ArtifactName);
		FPlatformProcess::LaunchURL(*FString::Printf(TEXT("com.epicgames.launcher://store/product/%s?action=launch&silent=true"), *ArtifactName), nullptr, nullptr);
		FPlatformMisc::RequestExit(false);
		return false;
	}

	SDKManager = MakeUnique<FWrapperSDKManager>();
	check(SDKManager);
	SDKManager->Initialize();

	AuthHandle = EOS_Platform_GetAuthInterface(SDKManager->GetPlatformHandle());
	check(AuthHandle != nullptr);

	ConnectHandle = EOS_Platform_GetConnectInterface(SDKManager->GetPlatformHandle());
	check(ConnectHandle != nullptr);

	UserInfoHandle = EOS_Platform_GetUserInfoInterface(SDKManager->GetPlatformHandle());
	check(UserInfoHandle != nullptr);

	FriendsHandle = EOS_Platform_GetFriendsInterface(SDKManager->GetPlatformHandle());
	check(FriendsHandle != nullptr);

	UIHandle = EOS_Platform_GetUIInterface(SDKManager->GetPlatformHandle());
	check(UIHandle != nullptr);

	PresenceHandle = EOS_Platform_GetPresenceInterface(SDKManager->GetPlatformHandle());
	check(PresenceHandle != nullptr);

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

void FEOSWrapperSubsystem::ReleaseVoiceChatUserInterface(const FUniqueNetId& LocalUserId)
{
}

FWrapperSDKManager* FEOSWrapperSubsystem::GetSDKManager() const
{
	if (SDKManager)
		return SDKManager.Get();
	return nullptr;
}

#pragma optimize("", on)
