// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOSWrapperSubsystem.h"

#include "EOSHelpers.h"
#include "EOSWrapperSessionManager.h"
#include "EOSWrapperSettings.h"
#include "EOSWrapperUserManager.h"
#include "eos_sdk.h"
#include "IEOSSDKManager.h"

DEFINE_LOG_CATEGORY(EOSWrapperSubsystem);

#define EOS_ENCRYPTION_KEY_MAX_LENGTH 64
#define EOS_ENCRYPTION_KEY_MAX_BUFFER_LEN (EOS_ENCRYPTION_KEY_MAX_LENGTH + 1)

#pragma optimize("", off)

/** Class that holds the strings for the call duration */
struct FEOSPlatformOptions : public EOS_Platform_Options
{
	FEOSPlatformOptions()
		: EOS_Platform_Options()
	{
		ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
		ProductId = ProductIdAnsi;
		SandboxId = SandboxIdAnsi;
		DeploymentId = DeploymentIdAnsi;
		ClientCredentials.ClientId = ClientIdAnsi;
		ClientCredentials.ClientSecret = ClientSecretAnsi;
		CacheDirectory = CacheDirectoryAnsi;
		EncryptionKey = EncryptionKeyAnsi;
	}

	char ClientIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char ClientSecretAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char ProductIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char SandboxIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char DeploymentIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char CacheDirectoryAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char EncryptionKeyAnsi[EOS_ENCRYPTION_KEY_MAX_BUFFER_LEN];
};


FEOSWrapperSubsystem::FEOSWrapperSubsystem(FName InInstanceName)
	: IEOSWrapperSubsystem(EOS_SUBSYSTEM, InInstanceName)
{
	StopTicker();
}

IVoiceChatUser* FEOSWrapperSubsystem::GetVoiceChatUserInterface(const FUniqueNetId& LocalUserId)
{
	IVoiceChatUser* Result = nullptr;

	// #if WITH_EOS_RTC
	// 	if (!VoiceChatInterface)
	// 	{
	// 		if (FEOSVoiceChatFactory* EOSVoiceChatFactory = FEOSVoiceChatFactory::Get())
	// 		{
	// 			VoiceChatInterface = EOSVoiceChatFactory->CreateInstanceWithPlatform(EOSPlatformHandle);
	// 		}
	// 	}
	//
	// 	if(VoiceChatInterface && UserManager->IsLocalUser(LocalUserId))
	// 	{
	// 		if (FOnlineSubsystemEOSVoiceChatUserWrapperRef* WrapperPtr = LocalVoiceChatUsers.Find(LocalUserId.AsShared()))
	// 		{
	// 			Result = &WrapperPtr->Get();
	// 		}
	// 		else
	// 		{
	// 			FEOSVoiceChatUser* VoiceChatUser = static_cast<FEOSVoiceChatUser*>(VoiceChatInterface->CreateUser());
	// 			VoiceChatUser->Login(UserManager->GetPlatformUserIdFromUniqueNetId(LocalUserId), LexToString(FUniqueNetIdEOS::Cast(LocalUserId).GetProductUserId()), FString(), FOnVoiceChatLoginCompleteDelegate());
	//
	// 			const FOnlineSubsystemEOSVoiceChatUserWrapperRef& Wrapper = LocalVoiceChatUsers.Emplace(LocalUserId.AsShared(), MakeShared<FOnlineSubsystemEOSVoiceChatUserWrapper, ESPMode::ThreadSafe>(*VoiceChatUser));
	// 			Result = &Wrapper.Get();
	// 		}
	// 	}
	// #endif // WITH_EOS_RTC

	return Result;
}

FPlatformEOSHelpersPtr FEOSWrapperSubsystem::EOSHelpersPtr;

void FEOSWrapperSubsystem::ModuleInit()
{
	LLM_SCOPE(ELLMTag::RealTimeCommunications);

	EOSHelpersPtr = MakeShareable(new FPlatformEOSHelpers());

	const FName EOSSharedModuleName = TEXT("EOSShared");
	if (!FModuleManager::Get().IsModuleLoaded(EOSSharedModuleName))
	{
		FModuleManager::Get().LoadModuleChecked(EOSSharedModuleName);
	}
	IEOSSDKManager* EOSSDKManager = IEOSSDKManager::Get();
	if (!EOSSDKManager)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: Missing IEOSSDKManager modular feature."));
		return;
	}

	EOS_EResult InitResult = EOSSDKManager->Initialize();
	if (InitResult != EOS_EResult::EOS_Success)
	{
		UE_LOG_ONLINE(Error, TEXT("FOnlineSubsystemEOS: failed to initialize the EOS SDK with result code (%d)"), InitResult);
		return;
	}
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
		UE_LOG_ONLINE(Warning, TEXT("FEOSWrapperSubsystem::Init() relaunching artifact (%s) via the store"), *ArtifactName);
		FPlatformProcess::LaunchURL(*FString::Printf(TEXT("com.epicgames.launcher://store/product/%s?action=launch&silent=true"), *ArtifactName), nullptr, nullptr);
		FPlatformMisc::RequestExit(false);
		return false;
	}

	EOSSDKManager = IEOSSDKManager::Get();
	if (!EOSSDKManager)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem::Init() failed to get EOSSDKManager interface"));
		return false;
	}

	if (!PlatformCreate())
	{
		return false;
	}

	// Get handles for later use
	AuthHandle = EOS_Platform_GetAuthInterface(*EOSPlatformHandle);
	if (AuthHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get auth handle"));
		return false;
	}
	UserInfoHandle = EOS_Platform_GetUserInfoInterface(*EOSPlatformHandle);
	if (UserInfoHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get user info handle"));
		return false;
	}
	UIHandle = EOS_Platform_GetUIInterface(*EOSPlatformHandle);
	if (UIHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get UI handle"));
		return false;
	}
	FriendsHandle = EOS_Platform_GetFriendsInterface(*EOSPlatformHandle);
	if (FriendsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get friends handle"));
		return false;
	}
	PresenceHandle = EOS_Platform_GetPresenceInterface(*EOSPlatformHandle);
	if (PresenceHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get presence handle"));
		return false;
	}
	ConnectHandle = EOS_Platform_GetConnectInterface(*EOSPlatformHandle);
	if (ConnectHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get connect handle"));
		return false;
	}
	SessionsHandle = EOS_Platform_GetSessionsInterface(*EOSPlatformHandle);
	if (SessionsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get sessions handle"));
		return false;
	}
	StatsHandle = EOS_Platform_GetStatsInterface(*EOSPlatformHandle);
	if (StatsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get stats handle"));
		return false;
	}
	LeaderboardsHandle = EOS_Platform_GetLeaderboardsInterface(*EOSPlatformHandle);
	if (LeaderboardsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get leaderboards handle"));
		return false;
	}
	MetricsHandle = EOS_Platform_GetMetricsInterface(*EOSPlatformHandle);
	if (MetricsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get metrics handle"));
		return false;
	}
	AchievementsHandle = EOS_Platform_GetAchievementsInterface(*EOSPlatformHandle);
	if (AchievementsHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get achievements handle"));
		return false;
	}
	// Disable ecom if not part of EGS
	// if (bWasLaunchedByEGS)
	// {
	// 	EcomHandle = EOS_Platform_GetEcomInterface(*EOSPlatformHandle);
	// 	if (EcomHandle == nullptr)
	// 	{
	// 		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get ecom handle"));
	// 		return false;
	// 	}
	// 	StoreInterfacePtr = MakeShareable(new FOnlineStoreEOS(this));
	// }
	TitleStorageHandle = EOS_Platform_GetTitleStorageInterface(*EOSPlatformHandle);
	if (TitleStorageHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get title storage handle"));
		return false;
	}
	PlayerDataStorageHandle = EOS_Platform_GetPlayerDataStorageInterface(*EOSPlatformHandle);
	if (PlayerDataStorageHandle == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem: failed to init EOS platform, couldn't get player data storage handle"));
		return false;
	}

	UserManager = MakeShareable(new FEOSWrapperUserManager(this));
	UserManager->Initialize();

	SessionManager = MakeShareable(new FEOSWrapperSessionManager(this));
	SessionManager->Initialize(EOSSDKManager->GetProductName() + TEXT("_") + FString::FromInt(GetBuildUniqueId()));

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

	StartTicker();

	bInitialized = true;
	return true;
}

bool FEOSWrapperSubsystem::Shutdown()
{
	if (!bInitialized) return false;

	UE_LOG_ONLINE(VeryVerbose, TEXT("FEOSWrapperSubsystem::Shutdown()"));

	// EOS-22677 workaround: Make sure tick is called at least once before shutting down.
	if (EOSPlatformHandle)
	{
		EOS_Platform_Tick(*EOSPlatformHandle);
	}

	StopTicker();

	// if (SocketSubsystem)
	// {
	// 	SocketSubsystem->Shutdown();
	// 	SocketSubsystem = nullptr;
	// }

	// Release our ref to the interfaces. May still exist since they can be aggregated
	UserManager = nullptr;
	SessionManager = nullptr;
	// StatsInterfacePtr = nullptr;
	// LeaderboardsInterfacePtr = nullptr;
	// AchievementsInterfacePtr = nullptr;
	// StoreInterfacePtr = nullptr;
	// TitleFileInterfacePtr = nullptr;
	// UserCloudInterfacePtr = nullptr;

	// #if WITH_EOS_RTC
	// 	for (TPair<FUniqueNetIdRef, FOnlineSubsystemEOSVoiceChatUserWrapperRef>& Pair : LocalVoiceChatUsers)
	// 	{
	// 		FOnlineSubsystemEOSVoiceChatUserWrapperRef& VoiceChatUserWrapper = Pair.Value;
	// 		VoiceChatInterface->ReleaseUser(&VoiceChatUserWrapper->VoiceChatUser);
	// 	}
	// 	LocalVoiceChatUsers.Reset();
	// 	VoiceChatInterface = nullptr;
	// #endif

	EOSPlatformHandle = nullptr;

	return FOnlineSubsystemImpl::Shutdown();

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

	SessionManager->Tick(DeltaTime);
	FOnlineSubsystemImpl::Tick(DeltaTime);

	return true;
}

IOnlineSessionPtr FEOSWrapperSubsystem::GetSessionInterface() const
{
	return SessionManager;
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

bool FEOSWrapperSubsystem::PlatformCreate()
{
	FString ArtifactName;
	FParse::Value(FCommandLine::Get(), TEXT("EpicApp="), ArtifactName);
	// Find the settings for this artifact
	FEOSArtifactSettings ArtifactSettings;
	if (!UEOSWrapperSettings::GetSettingsForArtifact(ArtifactName, ArtifactSettings))
	{
		UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem::PlatformCreate() failed to find artifact settings object for artifact (%s)"), *ArtifactName);
		return false;
	}

	// Create platform instance
	FEOSPlatformOptions PlatformOptions;
	FCStringAnsi::Strncpy(PlatformOptions.ClientIdAnsi, TCHAR_TO_UTF8(*ArtifactSettings.ClientId), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.ClientSecretAnsi, TCHAR_TO_UTF8(*ArtifactSettings.ClientSecret), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.ProductIdAnsi, TCHAR_TO_UTF8(*ArtifactSettings.ProductId), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.SandboxIdAnsi, TCHAR_TO_UTF8(*ArtifactSettings.SandboxId), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.DeploymentIdAnsi, TCHAR_TO_UTF8(*ArtifactSettings.DeploymentId), EOS_OSS_STRING_BUFFER_LENGTH);
	PlatformOptions.bIsServer = IsRunningDedicatedServer() ? EOS_TRUE : EOS_FALSE;
	PlatformOptions.Reserved = nullptr;
	FEOSWrapperSettings EOSSettings = UEOSWrapperSettings::GetSettings();
	uint64 OverlayFlags = 0;
	if (!EOSSettings.bEnableOverlay)
	{
		OverlayFlags |= EOS_PF_DISABLE_OVERLAY;
	}
	if (!EOSSettings.bEnableSocialOverlay)
	{
		OverlayFlags |= EOS_PF_DISABLE_SOCIAL_OVERLAY;
	}
#if WITH_EDITOR
	if (!EOSSettings.bEnableEditorOverlay)
	{
		OverlayFlags |= EOS_PF_LOADING_IN_EDITOR;
	}
#endif

	// Don't allow the overlay to be used in the editor when running PIE.
	const bool bEditorOverlayAllowed = EOSSettings.bEnableEditorOverlay && InstanceName == FOnlineSubsystemImpl::DefaultInstanceName;
	const bool bOverlayAllowed = IsRunningGame() || bEditorOverlayAllowed;

	PlatformOptions.Flags = bOverlayAllowed ? OverlayFlags : EOS_PF_DISABLE_OVERLAY;
	// Make the cache directory be in the user's writable area

	const FString CacheDir = EOSSDKManager->GetCacheDirBase() / ArtifactName / EOSSettings.CacheDir;
	FCStringAnsi::Strncpy(PlatformOptions.CacheDirectoryAnsi, TCHAR_TO_UTF8(*CacheDir), EOS_OSS_STRING_BUFFER_LENGTH);
	FCStringAnsi::Strncpy(PlatformOptions.EncryptionKeyAnsi, TCHAR_TO_UTF8(*ArtifactSettings.EncryptionKey), EOS_ENCRYPTION_KEY_MAX_BUFFER_LEN);

#if WITH_EOS_RTC
	EOS_Platform_RTCOptions RtcOptions = { 0 };
	RtcOptions.ApiVersion = EOS_PLATFORM_RTCOPTIONS_API_LATEST;
	RtcOptions.PlatformSpecificOptions = nullptr;
	PlatformOptions.RTCOptions = &RtcOptions;
#endif

	IEOSSDKManager* SDKManager = IEOSSDKManager::Get();
	if (ensure(SDKManager))
	{
		EOSPlatformHandle = SDKManager->CreatePlatform(PlatformOptions);
		return true;
	}

	UE_LOG_ONLINE(Error, TEXT("FEOSWrapperSubsystem::PlatformCreate() failed to init EOS platform"));
	return false;
}

#pragma optimize("", on)
