// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#include "CoreMinimal.h"
#include "EOSHelpers.h"
#include "IEOSWrapperSubsystem.h"

#include COMPILED_PLATFORM_HEADER(EOSHelpers.h)

DECLARE_LOG_CATEGORY_EXTERN(EOSWrapperSubsystem, Log, All);

#if WITH_EOS_SDK

#include "eos_sdk.h"

class FEOSWrapperUserManager;
class IEOSSDKManager;
typedef TSharedPtr<class FEOSWrapperUserManager, ESPMode::ThreadSafe> FEOSWrapperUserManagerPtr;
typedef TSharedPtr<class FEOSWrapperSessionManager, ESPMode::ThreadSafe> FEOSWrapperSessionManagerPtr;
using IEOSPlatformHandlePtr = TSharedPtr<class IEOSPlatformHandle, ESPMode::ThreadSafe>;
typedef TSharedPtr<FPlatformEOSHelpers, ESPMode::ThreadSafe> FPlatformEOSHelpersPtr;

#define EOS_OSS_STRING_BUFFER_LENGTH 256 + 1  // 256 plus null terminator

/**
 *
 */
class EOSWRAPPER_API FEOSWrapperSubsystem : public IEOSWrapperSubsystem
{
public:
	virtual ~FEOSWrapperSubsystem() = default;

	/** Only the factory makes instances */
	FEOSWrapperSubsystem() = delete;
	explicit FEOSWrapperSubsystem(FName InInstanceName);

	// IEOSWrapperSubsystem
	virtual IVoiceChatUser* GetVoiceChatUserInterface(const FUniqueNetId& LocalUserId) override;
	virtual IEOSPlatformHandlePtr GetEOSPlatformHandle() const override { return EOSPlatformHandle; };

	/** Used to be called before RHIInit() */
	static void ModuleInit();
	static void ModuleShutdown();

	FPlatformEOSHelpersPtr GetEOSHelpers() { return EOSHelpersPtr; };

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;

	// FTSTickerObjectBase
	virtual bool Tick(float DeltaTime) override;

	// IOnlineSubsystem
	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual FText GetOnlineServiceName() const override;
	virtual IOnlineStatsPtr GetStatsInterface() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual void ReloadConfigs(const TSet<FString>& ConfigSections) override;

	virtual IOnlineGroupsPtr GetGroupsInterface() const override { return nullptr; }
	virtual IOnlinePartyPtr GetPartyInterface() const override { return nullptr; }
	virtual IOnlineTimePtr GetTimeInterface() const override { return nullptr; }
	virtual IOnlineEventsPtr GetEventsInterface() const override { return nullptr; }
	virtual IOnlineSharingPtr GetSharingInterface() const override { return nullptr; }
	virtual IOnlineMessagePtr GetMessageInterface() const override { return nullptr; }
	virtual IOnlineChatPtr GetChatInterface() const override { return nullptr; }
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override { return nullptr; }
	virtual IOnlineTournamentPtr GetTournamentInterface() const override { return nullptr; }
	//~IOnlineSubsystem

	bool bWasLaunchedByEGS = false;
	bool bIsDefaultOSS = false;
	bool bIsPlatformOSS = false;

	FString ProductId;

	void ReleaseVoiceChatUserInterface(const FUniqueNetId& LocalUserId);

	EOS_HAuth GetAuthHandle() { return AuthHandle; }
	EOS_HConnect GetConnectHandle() { return ConnectHandle; }
	EOS_HUserInfo GetUserInfoHandle() { return UserInfoHandle; }
	EOS_HFriends GetFriendsHandle() { return FriendsHandle; }
	EOS_HUI GetUIHandle() { return UIHandle; }
	EOS_HPresence GetPresenceHandle() { return PresenceHandle; }
	EOS_HSessions GetSessionsHandle() { return SessionsHandle; }
	EOS_HMetrics GetMetricsHandle() { return MetricsHandle; }

	IEOSSDKManager* EOSSDKManager;
	FEOSWrapperUserManagerPtr UserManager;
	FEOSWrapperSessionManagerPtr SessionManager;
	IEOSPlatformHandlePtr EOSPlatformHandle;
	static FPlatformEOSHelpersPtr EOSHelpersPtr;

private:
	bool PlatformCreate();

	/** EOS handles */
	EOS_HAuth AuthHandle = nullptr;
	EOS_HUI UIHandle = nullptr;
	EOS_HFriends FriendsHandle = nullptr;
	EOS_HUserInfo UserInfoHandle = nullptr;
	EOS_HPresence PresenceHandle = nullptr;
	EOS_HConnect ConnectHandle = nullptr;
	EOS_HSessions SessionsHandle = nullptr;
	EOS_HStats StatsHandle = nullptr;
	EOS_HLeaderboards LeaderboardsHandle = nullptr;
	EOS_HMetrics MetricsHandle = nullptr;
	EOS_HAchievements AchievementsHandle = nullptr;
	EOS_HEcom EcomHandle = nullptr;
	EOS_HTitleStorage TitleStorageHandle = nullptr;
	EOS_HPlayerDataStorage PlayerDataStorageHandle = nullptr;

	bool bInitialized = false;
};

#endif

typedef TSharedPtr<FEOSWrapperSubsystem, ESPMode::ThreadSafe> FEOSWrapperSubsystemPtr;
