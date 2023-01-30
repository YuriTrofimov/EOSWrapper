// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EOSSharedTypes.h"
#include "EOSWrapperTypes.h"

#if WITH_EOS_SDK
#include "eos_types.h"
#include "eos_lobby_types.h"
#include "eos_auth_types.h"
#include "eos_connect_types.h"
#include "eos_sessions_types.h"

static FName EOS_SESSION_ID = TEXT("EOS_SESSION_ID");
static FName EOS_LOBBY_ID = TEXT("EOS_LOBBY_ID");
TEMP_UNIQUENETIDSTRING_SUBCLASS(FUniqueNetIdEOSSession, EOS_SESSION_ID);

TEMP_UNIQUENETIDSTRING_SUBCLASS(FUniqueNetIdEOSLobby, EOS_LOBBY_ID);

struct FSessionSearchEOS
{
	EOS_HSessionSearch SearchHandle;

	FSessionSearchEOS(EOS_HSessionSearch InSearchHandle)
		: SearchHandle(InSearchHandle)
	{
	}

	virtual ~FSessionSearchEOS()
	{
		EOS_SessionSearch_Release(SearchHandle);
	}
};

struct FLobbyDetailsEOS : FNoncopyable
{
	EOS_HLobbyDetails LobbyDetailsHandle;

	FLobbyDetailsEOS(EOS_HLobbyDetails InLobbyDetailsHandle)
		: LobbyDetailsHandle(InLobbyDetailsHandle)
	{
	}

	virtual ~FLobbyDetailsEOS()
	{
		EOS_LobbyDetails_Release(LobbyDetailsHandle);
	}
};

class FEOSWrapperSubsystem;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyCreatedDelegate, const FName&)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUpdatedDelegate, const FName&)

/**
 *
 */
class EOSWRAPPER_API FEOSWrapperSessionManager : public IOnlineSession, public TSharedFromThis<FEOSWrapperSessionManager, ESPMode::ThreadSafe>
{
public:
	FEOSWrapperSessionManager() = delete;
	virtual ~FEOSWrapperSessionManager() override;
	FEOSWrapperSessionManager(FEOSWrapperSubsystem* WrapperSubsystem);

	// IOnlineSession Interface
	virtual bool CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool StartSession(FName SessionName) override;
	virtual bool UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData = true) override;
	virtual bool EndSession(FName SessionName) override;
	virtual bool DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate()) override;
	virtual bool IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId) override;
	virtual bool StartMatchmaking(const TArray<FUniqueNetIdRef>& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings,
		TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName) override;
	virtual bool CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName) override;
	virtual bool FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId,
		const FOnSingleSessionResultCompleteDelegate& CompletionDelegate) override;
	virtual bool CancelFindSessions() override;
	virtual bool PingSearchResults(const FOnlineSessionSearchResult& SearchResult) override;
	virtual bool JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<FUniqueNetIdRef>& FriendList) override;
	virtual bool SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray<FUniqueNetIdRef>& Friends) override;
	virtual bool SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray<FUniqueNetIdRef>& Friends) override;
	virtual bool GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType) override;
	virtual bool GetResolvedConnectString(const FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo) override;
	virtual FOnlineSessionSettings* GetSessionSettings(FName SessionName) override;
	virtual bool RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited) override;
	virtual bool RegisterPlayers(FName SessionName, const TArray<FUniqueNetIdRef>& Players, bool bWasInvited = false) override;
	virtual bool UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId) override;
	virtual bool UnregisterPlayers(FName SessionName, const TArray<FUniqueNetIdRef>& Players) override;
	virtual void RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual void UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual void RemovePlayerFromSession(int32 LocalUserNum, FName SessionName, const FUniqueNetId& TargetPlayerId) override;
	virtual int32 GetNumSessions() override;
	virtual void DumpSessionState() override;
	virtual FNamedOnlineSession* GetNamedSession(FName SessionName) override;
	virtual void RemoveNamedSession(FName SessionName) override;
	virtual EOnlineSessionState::Type GetSessionState(FName SessionName) const override;
	virtual bool HasPresenceSession() override;
	virtual FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSessionSettings& SessionSettings) override;
	virtual FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSession& Session) override;
	virtual FUniqueNetIdPtr CreateSessionIdFromString(const FString& SessionIdStr) override;
	// ~IOnlineSession Interface

	bool SetLobbyParameter(const FName& LobbyName, const FName& Parameter, const FString& Value);

	void Initialize(const FString& InBucketId);
	/** Session tick for various background tasks */
	void Tick(float DeltaTime);

	FOnLobbyCreatedDelegate OnLobbyCreated;
	FOnLobbyUpdatedDelegate OnLobbyUpdated;

private:
	EOS_HLobby LobbyHandle;
	
	void RegisterLobbyNotifications();
	void RegisterLocalPlayers(class FNamedOnlineSession* Session);

	// Lobby session callbacks and methods
	FCallbackBase* LobbyCreatedCallback;
	FCallbackBase* LobbySearchFindCallback;
	FCallbackBase* LobbyJoinedCallback;
	FCallbackBase* LobbyLeftCallback;
	FCallbackBase* LobbyDestroyedCallback;
	FCallbackBase* LobbySendInviteCallback;

	uint32 FindLobbySession(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings);
	void StartLobbySearch(int32 SearchingPlayerNum, EOS_HLobbySearch LobbySearchHandle, const TSharedRef<FOnlineSessionSearch>& SearchSettings,
		const FOnSingleSessionResultCompleteDelegate& CompletionDelegate);
	uint32 CreateLobbySession(int32 HostingPlayerNum, FNamedOnlineSession* Session);
	uint32 UpdateLobbySession(FNamedOnlineSession* Session);
	uint32 JoinLobbySession(int32 PlayerNum, FNamedOnlineSession* Session, const FOnlineSession* SearchSession);
	uint32 StartLobbySession(FNamedOnlineSession* Session);
	uint32 EndLobbySession(FNamedOnlineSession* Session);
	uint32 DestroyLobbySession(FNamedOnlineSession* Session, const FOnDestroySessionCompleteDelegate& CompletionDelegate);
	EOS_ELobbyPermissionLevel GetLobbyPermissionLevelFromSessionSettings(const FOnlineSessionSettings& SessionSettings);
	uint32_t GetLobbyMaxMembersFromSessionSettings(const FOnlineSessionSettings& SessionSettings);
	FOnlineSession* GetOnlineSessionFromLobbyId(const FUniqueNetIdEOSLobby& LobbyId);
	bool SendLobbyInvite(FName SessionName, EOS_ProductUserId SenderId, EOS_ProductUserId ReceiverId);
	FNamedOnlineSession* GetNamedSessionFromLobbyId(const FUniqueNetIdEOSLobby& LobbyId);
	FOnlineSessionSearchResult* GetSearchResultFromLobbyId(const FUniqueNetIdEOSLobby& LobbyId);

	// Lobby notification callbacks and methods
	EOS_NotificationId LobbyUpdateReceivedId;
	FCallbackBase* LobbyUpdateReceivedCallback;
	EOS_NotificationId LobbyMemberUpdateReceivedId;
	FCallbackBase* LobbyMemberUpdateReceivedCallback;
	EOS_NotificationId LobbyMemberStatusReceivedId;
	FCallbackBase* LobbyMemberStatusReceivedCallback;
	EOS_NotificationId LobbyInviteAcceptedId;
	FCallbackBase* LobbyInviteAcceptedCallback;
	EOS_NotificationId JoinLobbyAcceptedId;
	FCallbackBase* JoinLobbyAcceptedCallback;

	void OnLobbyUpdateReceived(const EOS_LobbyId& LobbyId);
	void OnLobbyMemberUpdateReceived(const EOS_LobbyId& LobbyId, const EOS_ProductUserId& TargetUserId);
	void OnMemberStatusReceived(const EOS_LobbyId& LobbyId, const EOS_ProductUserId& TargetUserId, EOS_ELobbyMemberStatus CurrentStatus);
	void OnLobbyInviteAccepted(const char* InviteId, const EOS_ProductUserId& LocalUserId, const EOS_ProductUserId& TargetUserId);
	void OnJoinLobbyAccepted(const EOS_ProductUserId& LocalUserId, const EOS_UI_EventId& UiEventId);

	bool AddOnlineSessionMember(FName SessionName, const FUniqueNetIdRef& PlayerId);
	bool RemoveOnlineSessionMember(FName SessionName, const FUniqueNetIdRef& PlayerId);
	bool SendSessionInvite(FName SessionName, EOS_ProductUserId SenderId, EOS_ProductUserId ReceiverId);

	void AddSearchResult(EOS_HSessionDetails SessionHandle, const TSharedRef<FOnlineSessionSearch>& SearchSettings);
	void AddSearchAttribute(EOS_HSessionSearch SearchHandle, const EOS_Sessions_AttributeData* Attribute, EOS_EOnlineComparisonOp ComparisonOp);
	void CopySearchResult(EOS_HSessionDetails SessionHandle, EOS_SessionDetails_Info* SessionInfo, FOnlineSession& SessionSettings);
	void CopyAttributes(EOS_HSessionDetails SessionHandle, FOnlineSession& OutSession);

	void SetPermissionLevel(EOS_HSessionModification SessionModHandle, FNamedOnlineSession* Session);
	void SetMaxPlayers(EOS_HSessionModification SessionModHandle, FNamedOnlineSession* Session);
	void SetInvitesAllowed(EOS_HSessionModification SessionModHandle, FNamedOnlineSession* Session);
	void SetJoinInProgress(EOS_HSessionModification SessionModHandle, FNamedOnlineSession* Session);
	void AddAttribute(EOS_HSessionModification SessionModHandle, const EOS_Sessions_AttributeData* Attribute);
	void SetAttributes(EOS_HSessionModification SessionModHandle, FNamedOnlineSession* Session);
	typedef TEOSCallback<EOS_Sessions_OnUpdateSessionCallback, EOS_Sessions_UpdateSessionCallbackInfo, FEOSWrapperSessionManager> FUpdateSessionCallback;
	uint32 SharedSessionUpdate(EOS_HSessionModification SessionModHandle, FNamedOnlineSession* Session, FUpdateSessionCallback* Callback);


	void BeginSessionAnalytics(FNamedOnlineSession* Session);
	void EndSessionAnalytics();

	// Methods to update an API Lobby from an OSS Lobby
	void SetLobbyPermissionLevel(EOS_HLobbyModification LobbyModificationHandle, FNamedOnlineSession* Session);
	void SetLobbyMaxMembers(EOS_HLobbyModification LobbyModificationHandle, FNamedOnlineSession* Session);
	void SetLobbyAttributes(EOS_HLobbyModification LobbyModificationHandle, FNamedOnlineSession* Session);
	void AddLobbyAttribute(EOS_HLobbyModification LobbyModificationHandle, const EOS_Lobby_AttributeData* Attribute);
	void AddLobbyMemberAttribute(EOS_HLobbyModification LobbyModificationHandle, const EOS_Lobby_AttributeData* Attribute);

	// Methods to update an OSS Lobby from an API Lobby
	typedef TFunction<void(bool bWasSuccessful)> FOnCopyLobbyDataCompleteCallback;
	void CopyLobbyData(const TSharedRef<FLobbyDetailsEOS>& LobbyDetails, EOS_LobbyDetails_Info* LobbyDetailsInfo, FOnlineSession& OutSession, const FOnCopyLobbyDataCompleteCallback& Callback);
	void CopyLobbyAttributes(const TSharedRef<FLobbyDetailsEOS>& LobbyDetails, FOnlineSession& OutSession);
	void CopyLobbyMemberAttributes(const FLobbyDetailsEOS& LobbyDetails, const EOS_ProductUserId& TargetUserId, FSessionSettings& OutSessionSettings);
	void AddLobbySearchAttribute(EOS_HLobbySearch LobbySearchHandle, const EOS_Lobby_AttributeData* Attribute, EOS_EOnlineComparisonOp ComparisonOp);
	void AddLobbySearchResult(const TSharedRef<FLobbyDetailsEOS>& LobbyDetails, const TSharedRef<FOnlineSessionSearch>& SearchSettings, const FOnCopyLobbyDataCompleteCallback& Callback);
	void UpdateOrAddLobbyMember(const FUniqueNetIdEOSLobbyRef& LobbyNetId, const FUniqueNetIdEOSRef& PlayerId);

	void TickLanTasks(float DeltaTime);

	// EOS Sessions
	uint32 CreateEOSSession(int32 HostingPlayerNum, FNamedOnlineSession* Session);
	uint32 JoinEOSSession(int32 PlayerNum, FNamedOnlineSession* Session, const FOnlineSession* SearchSession);
	uint32 StartEOSSession(FNamedOnlineSession* Session);
	uint32 UpdateEOSSession(FNamedOnlineSession* Session);
	uint32 EndEOSSession(FNamedOnlineSession* Session);
	uint32 DestroyEOSSession(FNamedOnlineSession* Session, const FOnDestroySessionCompleteDelegate& CompletionDelegate);
	uint32 FindEOSSession(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings);
	bool SendEOSSessionInvite(FName SessionName, EOS_ProductUserId SenderId, EOS_ProductUserId ReceiverId);
	void FindEOSSessionById(int32 SearchingPlayerNum, const FUniqueNetId& SessionId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegate);
	
	/** Cached pointer to owning subsystem */
	FEOSWrapperSubsystem* EOSSubsystem;
	TArray<TSharedRef<FLobbyDetailsEOS>> PendingLobbySearchResults;
	TMap<FString, TSharedRef<FLobbyDetailsEOS>> LobbySearchResultsCache;
	/** Current search object */
	TSharedPtr<FOnlineSessionSearch> CurrentSessionSearch;
	/** Current search start time. */
	double SessionSearchStartInSeconds;

	bool bLobbyCreated = false;
	bool bIsDedicatedServer = false;
	bool bIsUsingP2PSockets = false;

	/** Critical sections for thread safe operation of lobby sessions list */
	mutable FCriticalSection LobbyLock;

	/* Array pf known lobby sessions */
	TArray<FNamedOnlineSession> LobbySessions;
	/** The last accepted invite search. It searches by session id */
	TSharedPtr<FOnlineSessionSearch> LastInviteSearch;
	/** EOS handle wrapper to hold onto it for scope of the search */
	TSharedPtr<FSessionSearchEOS> CurrentSearchHandle;

	/** Notification state for SDK events */
	EOS_NotificationId SessionInviteAcceptedId;
	FCallbackBase* SessionInviteAcceptedCallback;
};

typedef TSharedPtr<FEOSWrapperSessionManager, ESPMode::ThreadSafe> FEOSWrapperSessionManagerPtr;
typedef TWeakPtr<FEOSWrapperSessionManager, ESPMode::ThreadSafe> FEOSWrapperSessionManagerWeakPtr;
typedef TWeakPtr<const FEOSWrapperSessionManager, ESPMode::ThreadSafe> FEOSWrapperSessionManagerConstWeakPtr;

#endif
