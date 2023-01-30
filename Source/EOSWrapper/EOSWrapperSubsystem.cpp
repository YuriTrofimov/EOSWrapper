// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOSWrapperSubsystem.h"
#include "EOSWrapperSessionManager.h"
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

	const FString BucketID = TEXT("OfficeProject_0");
	FCStringAnsi::Strncpy(BucketIdAnsi, TCHAR_TO_UTF8(*BucketID), EOS_OSS_STRING_BUFFER_LENGTH);

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
	ensure(SDKManager);
	if (!SDKManager) return false;
	SDKManager->Initialize();

	AuthHandle = EOS_Platform_GetAuthInterface(SDKManager->GetPlatformHandle());
	ensure(AuthHandle != nullptr);
	if (!AuthHandle) return false;

	ConnectHandle = EOS_Platform_GetConnectInterface(SDKManager->GetPlatformHandle());
	ensure(ConnectHandle != nullptr);
	if (!ConnectHandle) return false;

	UserInfoHandle = EOS_Platform_GetUserInfoInterface(SDKManager->GetPlatformHandle());
	ensure(UserInfoHandle != nullptr);
	if (!UserInfoHandle) return false;

	FriendsHandle = EOS_Platform_GetFriendsInterface(SDKManager->GetPlatformHandle());
	ensure(FriendsHandle != nullptr);
	if (!FriendsHandle) return false;

	UIHandle = EOS_Platform_GetUIInterface(SDKManager->GetPlatformHandle());
	ensure(UIHandle != nullptr);
	if (!UIHandle) return false;

	PresenceHandle = EOS_Platform_GetPresenceInterface(SDKManager->GetPlatformHandle());
	ensure(PresenceHandle != nullptr);
	if (!PresenceHandle) return false;

	SessionsHandle = EOS_Platform_GetSessionsInterface(SDKManager->GetPlatformHandle());
	ensure(SessionsHandle != nullptr);
	if (!SessionsHandle) return false;

	MetricsHandle = EOS_Platform_GetMetricsInterface(SDKManager->GetPlatformHandle());
	ensure(MetricsHandle != nullptr);
	if (!MetricsHandle) return false;

	LobbyHandle = EOS_Platform_GetLobbyInterface(SDKManager->GetPlatformHandle());
	ensure(LobbyHandle != nullptr);
	if (!LobbyHandle) return false;

	UserManager = MakeShareable(new FEOSWrapperUserManager(this));
	UserManager->Initialize();

	LobbyManager = MakeShareable(new FEOSWrapperSessionManager(this));
	LobbyManager->Initialize();

	StartTicker();

	bInitialized = true;
	return true;
}

bool FEOSWrapperSubsystem::Shutdown()
{
	if (!bInitialized) return false;

	auto PlatformHandle = SDKManager->GetPlatformHandle();
	if (PlatformHandle)
	{
		EOS_Platform_Tick(PlatformHandle);
	}

	StopTicker();

	if (LobbyManager)
	{
		LobbyManager->Shutdown();
		LobbyManager.Reset();
	}

	if (UserManager)
	{
		UserManager->Shutdown();
		UserManager.Reset();
	}

	if (SDKManager)
	{
		SDKManager->Shutdown();
		SDKManager.Reset();
	}

	// We set the product id
	FString ArtifactName;
	FParse::Value(FCommandLine::Get(), TEXT("EpicApp="), ArtifactName);
	FEOSArtifactSettings ArtifactSettings;
	if (UEOSWrapperSettings::GetSettingsForArtifact(ArtifactName, ArtifactSettings))
	{
		ProductId = ArtifactSettings.ProductId;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FEOSWrapperSubsystem::Init] Failed to find artifact settings object for artifact (%s). ProductIdAnsi not set."), *ArtifactName);
	}

	bInitialized = false;
	return true;
}

FString FEOSWrapperSubsystem::GetAppId() const
{
	return TEXT("");
}

bool FEOSWrapperSubsystem::Tick(float DeltaTime)
{
	if (!bTickerStarted)
	{
		return true;
	}

	SDKManager->Tick(DeltaTime);
	LobbyManager->Tick(DeltaTime);
	FOnlineSubsystemImpl::Tick(DeltaTime);

	return true;
}

IOnlineSessionPtr FEOSWrapperSubsystem::GetSessionInterface() const
{
	return LobbyManager;
}

IOnlineFriendsPtr FEOSWrapperSubsystem::GetFriendsInterface() const
{
	return UserManager;
}

IOnlineSharedCloudPtr FEOSWrapperSubsystem::GetSharedCloudInterface() const
{
	UE_LOG_ONLINE(Error, TEXT("Shared Cloud Interface Requested"));
	return nullptr;
}

IOnlineUserCloudPtr FEOSWrapperSubsystem::GetUserCloudInterface() const
{
	return nullptr;
}

IOnlineEntitlementsPtr FEOSWrapperSubsystem::GetEntitlementsInterface() const
{
	UE_LOG_ONLINE(Error, TEXT("Entitlements Interface Requested"));
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
	return UserManager;
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
	return UserManager;
}

IOnlinePresencePtr FEOSWrapperSubsystem::GetPresenceInterface() const
{
	return UserManager;
}

FText FEOSWrapperSubsystem::GetOnlineServiceName() const
{
	return NSLOCTEXT("EOSWrapper", "OnlineServiceName", "EOSWrapper");
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

FEOSWrapperSessionManager* FEOSWrapperSubsystem::GetLobbyManager() const
{
	if (LobbyManager)
		return LobbyManager.Get();
	return nullptr;
}

#pragma optimize("", on)
