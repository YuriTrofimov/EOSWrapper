// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#include "CoreMinimal.h"
#include "eos_auth_types.h"
#include "eos_connect_types.h"
#include "eos_friends_types.h"
#include "eos_presence_types.h"
#include "eos_ui_types.h"
#include "eos_userinfo_types.h"
#include "OnlineSubsystemImpl.h"
#include "WrapperSDKManager.h"

DECLARE_LOG_CATEGORY_EXTERN(EOSWrapperSubsystem, Log, All);

class FEOSWrapperUserManager;
typedef TSharedPtr<class FEOSWrapperUserManager, ESPMode::ThreadSafe> FEOSWrapperUserManagerPtr;

/**
 *
 */
class EOSWRAPPER_API FEOSWrapperSubsystem : public FOnlineSubsystemImpl
{
public:
	virtual ~FEOSWrapperSubsystem() = default;

	/** Only the factory makes instances */
	FEOSWrapperSubsystem() = delete;
	explicit FEOSWrapperSubsystem(FName InInstanceName);

	/** Used to be called before RHIInit() */
	static void ModuleInit();
	static void ModuleShutdown();

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;

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

	bool bWasLaunchedByEGS;
	bool bIsDefaultOSS;
	bool bIsPlatformOSS;

	FString ProductId;

	void ReleaseVoiceChatUserInterface(const FUniqueNetId& LocalUserId);

	EOS_HAuth GetAuthHandle() { return AuthHandle; }
	EOS_HConnect GetConnectHandle() { return ConnectHandle; }
	EOS_HUserInfo GetUserInfoHandle() { return UserInfoHandle; }
	EOS_HFriends GetFriendsHandle() { return FriendsHandle; }
	EOS_HUI GetUIHandle() { return UIHandle; }
	EOS_HPresence GetPresenceHandle() { return PresenceHandle; }
	
	FORCEINLINE FWrapperSDKManager* GetSDKManager() const;
	FEOSWrapperUserManagerPtr UserManager;
private:
	/** EOS handles */
	EOS_HAuth AuthHandle = nullptr;
	EOS_HConnect ConnectHandle = nullptr;
	EOS_HUserInfo UserInfoHandle = nullptr;
	EOS_HFriends FriendsHandle = nullptr;
	EOS_HUI UIHandle = nullptr;
	EOS_HPresence PresenceHandle = nullptr;

	bool bInitialized = false;
	TUniquePtr<FWrapperSDKManager> SDKManager;
};

typedef TSharedPtr<FEOSWrapperSubsystem, ESPMode::ThreadSafe> FEOSWrapperSubsystemPtr;