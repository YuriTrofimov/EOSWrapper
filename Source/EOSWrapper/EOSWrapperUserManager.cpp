// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOSWrapperUserManager.h"
#include "EOSWrapperSettings.h"
#include "EOSWrapperTypes.h"
#include "eos_auth.h"
#include "eos_connect.h"
#include "eos_presence.h"
#include "eos_presence_types.h"
#include "eos_userinfo.h"
#include "eos_userinfo_types.h"
#include "OnlineSubsystem.h"

#pragma optimize("", off)

typedef TEOSCallback<EOS_Auth_OnLoginCallback, EOS_Auth_LoginCallbackInfo, FEOSWrapperUserManager> FLoginCallback;
typedef TEOSCallback<EOS_Connect_OnLoginCallback, EOS_Connect_LoginCallbackInfo, FEOSWrapperUserManager> FConnectLoginCallback;
typedef TEOSCallback<EOS_Auth_OnDeletePersistentAuthCallback, EOS_Auth_DeletePersistentAuthCallbackInfo, FEOSWrapperUserManager> FDeletePersistentAuthCallback;
typedef TEOSCallback<EOS_Auth_OnLogoutCallback, EOS_Auth_LogoutCallbackInfo, FEOSWrapperUserManager> FLogoutCallback;

typedef TEOSGlobalCallback<EOS_Connect_OnAuthExpirationCallback, EOS_Connect_AuthExpirationCallbackInfo, FEOSWrapperUserManager> FRefreshAuthCallback;
typedef TEOSGlobalCallback<EOS_Presence_OnPresenceChangedCallback, EOS_Presence_PresenceChangedCallbackInfo, FEOSWrapperUserManager> FPresenceChangedCallback;
typedef TEOSGlobalCallback<EOS_Friends_OnFriendsUpdateCallback, EOS_Friends_OnFriendsUpdateInfo, FEOSWrapperUserManager> FFriendsStatusUpdateCallback;
typedef TEOSGlobalCallback<EOS_Auth_OnLoginStatusChangedCallback, EOS_Auth_LoginStatusChangedCallbackInfo, FEOSWrapperUserManager> FLoginStatusChangedCallback;

// Chose arbitrarily since the SDK doesn't define it
#define EOS_MAX_TOKEN_SIZE 4096

struct FAuthCredentials : public EOS_Auth_Credentials
{
	FAuthCredentials() : EOS_Auth_Credentials()
	{
		ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
		Id = IdAnsi;
		Token = TokenAnsi;

		FMemory::Memset(IdAnsi, 0, sizeof(IdAnsi));
		FMemory::Memset(TokenAnsi, 0, sizeof(TokenAnsi));
	}

	FAuthCredentials(const FAuthCredentials& Other)
	{
		ApiVersion = Other.ApiVersion;
		Id = IdAnsi;
		Token = TokenAnsi;
		Type = Other.Type;
		SystemAuthCredentialsOptions = Other.SystemAuthCredentialsOptions;
		ExternalType = Other.ExternalType;

		FCStringAnsi::Strncpy(IdAnsi, Other.IdAnsi, EOS_OSS_STRING_BUFFER_LENGTH);
		FCStringAnsi::Strncpy(TokenAnsi, Other.TokenAnsi, EOS_MAX_TOKEN_SIZE);
	}

	FAuthCredentials& operator=(FAuthCredentials& Other)
	{
		ApiVersion = Other.ApiVersion;
		Type = Other.Type;
		SystemAuthCredentialsOptions = Other.SystemAuthCredentialsOptions;
		ExternalType = Other.ExternalType;

		FCStringAnsi::Strncpy(IdAnsi, Other.IdAnsi, EOS_OSS_STRING_BUFFER_LENGTH);
		FCStringAnsi::Strncpy(TokenAnsi, Other.TokenAnsi, EOS_MAX_TOKEN_SIZE);

		return *this;
	}

	FAuthCredentials(EOS_EExternalCredentialType InExternalType, const FExternalAuthToken& AuthToken) : EOS_Auth_Credentials()
	{
		if (AuthToken.HasTokenData())
		{
			Init(InExternalType, AuthToken.TokenData);
		}
		else if (AuthToken.HasTokenString())
		{
			Init(InExternalType, AuthToken.TokenString);
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("FAuthCredentials object cannot be constructed with invalid FExternalAuthToken parameter"));
		}
	}

	void Init(EOS_EExternalCredentialType InExternalType, const FString& InTokenString)
	{
		ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
		Type = EOS_ELoginCredentialType::EOS_LCT_ExternalAuth;
		ExternalType = InExternalType;
		Id = IdAnsi;
		Token = TokenAnsi;

		FCStringAnsi::Strncpy(TokenAnsi, TCHAR_TO_UTF8(*InTokenString), InTokenString.Len() + 1);
	}

	void Init(EOS_EExternalCredentialType InExternalType, const TArray<uint8>& InToken)
	{
		ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
		Type = EOS_ELoginCredentialType::EOS_LCT_ExternalAuth;
		ExternalType = InExternalType;
		Id = IdAnsi;
		Token = TokenAnsi;

		uint32_t InOutBufferLength = EOS_MAX_TOKEN_SIZE;
		EOS_ByteArray_ToString(InToken.GetData(), InToken.Num(), TokenAnsi, &InOutBufferLength);
	}

	char IdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];
	char TokenAnsi[EOS_MAX_TOKEN_SIZE];
};

FEOSWrapperUserManager::~FEOSWrapperUserManager()
{
	Shutdown();
}

void FEOSWrapperUserManager::Initialize() {}

void FEOSWrapperUserManager::Shutdown() {}

bool FEOSWrapperUserManager::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	LocalUserNumToLastLoginCredentials.Emplace(LocalUserNum, MakeShared<FOnlineAccountCredentials>(AccountCredentials));

	FEOSWrapperSettings Settings = UEOSWrapperSettings::GetSettings();

	// Are we configured to run at all?
	if (!EOSSubsystem->bIsDefaultOSS && !EOSSubsystem->bIsPlatformOSS && !Settings.bUseEAS && !Settings.bUseEOSConnect)
	{
		UE_LOG_ONLINE(Warning, TEXT("Neither EAS nor EOS are configured to be used. Failed to login in user (%d)"), LocalUserNum);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), FString(TEXT("Not configured")));
		return true;
	}

	// See if we are configured to just use EOS and not EAS
	// if (!EOSSubsystem->bIsDefaultOSS && !EOSSubsystem->bIsPlatformOSS && !Settings.bUseEAS && Settings.bUseEOSConnect)
	// {
	// 	// Call the EOS + Platform login path
	// 	return ConnectLoginNoEAS(LocalUserNum);
	// }

	// We don't support offline logged in, so they are either logged in or not
	if (GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn)
	{
		UE_LOG_ONLINE(Warning, TEXT("User (%d) already logged in."), LocalUserNum);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), FString(TEXT("Already logged in")));
		return true;
	}

	// See if we are logging in using platform credentials to link to EAS
	// if (!EOSSubsystem->bIsDefaultOSS && !EOSSubsystem->bIsPlatformOSS && Settings.bUseEAS)
	// {
	// 	LoginViaExternalAuth(LocalUserNum);
	// 	return true;
	// }

	EOS_Auth_LoginOptions LoginOptions = {};
	LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
	LoginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

	// FPlatformEOSHelpersPtr EOSHelpers = EOSSubsystem->GetEOSHelpers();

	FAuthCredentials Credentials;
	LoginOptions.Credentials = &Credentials;
	//	EOSHelpers->PlatformAuthCredentials(Credentials);

	bool bIsPersistentLogin = false;

	if (AccountCredentials.Type == TEXT("exchangecode"))
	{
		// This is how the Epic launcher will pass credentials to you
		FCStringAnsi::Strncpy(Credentials.TokenAnsi, TCHAR_TO_UTF8(*AccountCredentials.Token), EOS_MAX_TOKEN_SIZE);
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_ExchangeCode;
	}
	else if (AccountCredentials.Type == TEXT("developer"))
	{
		// This is auth via the EOS auth tool
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
		FCStringAnsi::Strncpy(Credentials.IdAnsi, TCHAR_TO_UTF8(*AccountCredentials.Id), EOS_OSS_STRING_BUFFER_LENGTH);
		FCStringAnsi::Strncpy(Credentials.TokenAnsi, TCHAR_TO_UTF8(*AccountCredentials.Token), EOS_MAX_TOKEN_SIZE);
	}
	else if (AccountCredentials.Type == TEXT("password"))
	{
		// This is using a direct username / password. Restricted and not generally available.
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Password;
		FCStringAnsi::Strncpy(Credentials.IdAnsi, TCHAR_TO_UTF8(*AccountCredentials.Id), EOS_OSS_STRING_BUFFER_LENGTH);
		FCStringAnsi::Strncpy(Credentials.TokenAnsi, TCHAR_TO_UTF8(*AccountCredentials.Token), EOS_MAX_TOKEN_SIZE);
	}
	else if (AccountCredentials.Type == TEXT("accountportal"))
	{
		// This is auth via the EOS Account Portal
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;
	}
	else if (AccountCredentials.Type == TEXT("persistentauth"))
	{
		// Use locally stored token managed by EOSSDK keyring to attempt login.
		Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_PersistentAuth;

		// Id and Token must be null when using EOS_ELoginCredentialType::EOS_LCT_PersistentAuth
		Credentials.Id = nullptr;
		Credentials.Token = nullptr;

		// Store selection of persistent auth.
		// The persistent auth token is handled by the EOSSDK. On a login failure the persistent token may need to be deleted if it is invalid.
		bIsPersistentLogin = true;
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("Unable to Login() user (%d) due to missing auth parameters"), LocalUserNum);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), FString(TEXT("Missing auth parameters")));
		return false;
	}

	FLoginCallback* CallbackObj = new FLoginCallback(AsWeak());
	CallbackObj->CallbackLambda = [this, LocalUserNum, bIsPersistentLogin](const EOS_Auth_LoginCallbackInfo* Data) {
		if (Data->ResultCode == EOS_EResult::EOS_Success)
		{
			// Continue the login process by getting the product user id for EAS only
			ConnectLoginEAS(LocalUserNum, Data->LocalUserId);
		}
		else
		{
			auto TriggerLoginFailure = [this, LocalUserNum, LoginResultCode = Data->ResultCode]() {
				FString ErrorString = FString::Printf(TEXT("Login(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(LoginResultCode)));
				UE_LOG_ONLINE(Warning, TEXT("%s"), *ErrorString);
				TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), ErrorString);
			};

			const bool bShouldRemoveCachedToken =
				Data->ResultCode == EOS_EResult::EOS_InvalidAuth || Data->ResultCode == EOS_EResult::EOS_AccessDenied || Data->ResultCode == EOS_EResult::EOS_Auth_InvalidToken;

			// Check for invalid persistent login credentials.
			if (bIsPersistentLogin && bShouldRemoveCachedToken)
			{
				FDeletePersistentAuthCallback* DeleteAuthCallbackObj = new FDeletePersistentAuthCallback(AsWeak());
				DeleteAuthCallbackObj->CallbackLambda = [this, LocalUserNum, TriggerLoginFailure](const EOS_Auth_DeletePersistentAuthCallbackInfo* Data) {
					// Deleting the auth token is best effort.
					TriggerLoginFailure();
				};

				EOS_Auth_DeletePersistentAuthOptions DeletePersistentAuthOptions;
				DeletePersistentAuthOptions.ApiVersion = EOS_AUTH_DELETEPERSISTENTAUTH_API_LATEST;
				DeletePersistentAuthOptions.RefreshToken = nullptr;
				EOS_Auth_DeletePersistentAuth(EOSSubsystem->GetAuthHandle(), &DeletePersistentAuthOptions, (void*)DeleteAuthCallbackObj, DeleteAuthCallbackObj->GetCallbackPtr());
			}
			else
			{
				TriggerLoginFailure();
			}
		}
	};
	// Perform the auth call
	EOS_Auth_Login(EOSSubsystem->GetAuthHandle(), &LoginOptions, (void*)CallbackObj, CallbackObj->GetCallbackPtr());
	return true;
}

bool FEOSWrapperUserManager::Logout(int32 LocalUserNum)
{
	FUniqueNetIdEOSPtr UserId = GetLocalUniqueNetIdEOS(LocalUserNum);
	if (!UserId.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("No logged in user found for LocalUserNum=%d."), LocalUserNum);
		TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
		return false;
	}

	FLogoutCallback* CallbackObj = new FLogoutCallback(AsWeak());
	CallbackObj->CallbackLambda = [LocalUserNum, this](const EOS_Auth_LogoutCallbackInfo* Data) {
		FDeletePersistentAuthCallback* DeleteAuthCallbackObj = new FDeletePersistentAuthCallback(AsWeak());
		DeleteAuthCallbackObj->CallbackLambda = [this, LocalUserNum, LogoutResultCode = Data->ResultCode](const EOS_Auth_DeletePersistentAuthCallbackInfo* Data) {
			if (LogoutResultCode == EOS_EResult::EOS_Success)
			{
				RemoveLocalUser(LocalUserNum);
				TriggerOnLogoutCompleteDelegates(LocalUserNum, true);
			}
			else
			{
				TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
			}
		};

		EOS_Auth_DeletePersistentAuthOptions DeletePersistentAuthOptions;
		DeletePersistentAuthOptions.ApiVersion = EOS_AUTH_DELETEPERSISTENTAUTH_API_LATEST;
		DeletePersistentAuthOptions.RefreshToken = nullptr;
		EOS_Auth_DeletePersistentAuth(EOSSubsystem->GetAuthHandle(), &DeletePersistentAuthOptions, (void*)DeleteAuthCallbackObj, DeleteAuthCallbackObj->GetCallbackPtr());
	};

	EOS_Auth_LogoutOptions LogoutOptions = {};
	LogoutOptions.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
	LogoutOptions.LocalUserId = UserId->GetEpicAccountId();

	EOS_Auth_Logout(EOSSubsystem->GetAuthHandle(), &LogoutOptions, CallbackObj, CallbackObj->GetCallbackPtr());

	LocalUserNumToLastLoginCredentials.Remove(LocalUserNum);

	return true;
}

bool FEOSWrapperUserManager::AutoLogin(int32 LocalUserNum)
{
	return false;
}

TSharedPtr<FUserOnlineAccount> FEOSWrapperUserManager::GetUserAccount(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> Result;

	const FUniqueNetIdEOS& EOSID = FUniqueNetIdEOS::Cast(UserId);
	const FUserOnlineAccountEOSRef* FoundUserAccount = StringToUserAccountMap.Find(EOSID.ToString());
	if (FoundUserAccount != nullptr)
	{
		return *FoundUserAccount;
	}

	return nullptr;
}

TArray<TSharedPtr<FUserOnlineAccount>> FEOSWrapperUserManager::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount>> Result;

	for (TMap<FString, FUserOnlineAccountEOSRef>::TConstIterator It(StringToUserAccountMap); It; ++It)
	{
		Result.Add(It.Value());
	}
	return Result;
}

FUniqueNetIdPtr FEOSWrapperUserManager::GetUniquePlayerId(int32 LocalUserNum) const
{
	return GetLocalUniqueNetIdEOS(LocalUserNum);
}

FUniqueNetIdPtr FEOSWrapperUserManager::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	// If we're passed an EOSPlus id, the first EOS_ID_BYTE_SIZE bytes are the EAS|EOS part we care about.
	Size = FMath::Min(Size, EOS_ID_BYTE_SIZE);
	return FUniqueNetIdEOSRegistry::FindOrAdd(Bytes, Size);
}

FUniqueNetIdPtr FEOSWrapperUserManager::CreateUniquePlayerId(const FString& Str)
{
	FString NetIdStr = Str;
	// If we're passed an EOSPlus id, remove the platform id and separator.
	NetIdStr.Split(EOSPLUS_ID_SEPARATOR, nullptr, &NetIdStr);
	return FUniqueNetIdEOSRegistry::FindOrAdd(NetIdStr);
}

ELoginStatus::Type FEOSWrapperUserManager::GetLoginStatus(int32 LocalUserNum) const
{
	FUniqueNetIdEOSPtr UserId = GetLocalUniqueNetIdEOS(LocalUserNum);
	if (UserId.IsValid())
	{
		return GetLoginStatus(*UserId);
	}
	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FEOSWrapperUserManager::GetLoginStatus(const FUniqueNetId& UserId) const
{
	const FUniqueNetIdEOS& EosId = FUniqueNetIdEOS::Cast(UserId);
	return GetLoginStatus(EosId);
}

FString FEOSWrapperUserManager::GetPlayerNickname(int32 LocalUserNum) const
{
	FUniqueNetIdEOSPtr UserId = GetLocalUniqueNetIdEOS(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UserId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetDisplayName();
		}
	}
	return FString();
}

FString FEOSWrapperUserManager::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid())
	{
		return UserAccount->GetDisplayName();
	}
	return FString();
}

FString FEOSWrapperUserManager::GetAuthToken(int32 LocalUserNum) const
{
	FUniqueNetIdPtr UserId = GetUniquePlayerId(LocalUserNum);
	if (UserId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UserId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetAccessToken();
		}
	}
	return FString();
}

void FEOSWrapperUserManager::GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(UserId, Privilege, (uint32)EPrivilegeResults::NoFailures);
}

FString FEOSWrapperUserManager::GetAuthType() const
{
	return TEXT("epic");
}

void FEOSWrapperUserManager::RevokeAuthToken(const FUniqueNetId& LocalUserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(LocalUserId, FOnlineError(EOnlineErrorResult::NotImplemented));
}

FPlatformUserId FEOSWrapperUserManager::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const
{
	return GetPlatformUserIdFromLocalUserNum(GetLocalUserNumFromUniqueNetId(UniqueNetId));
}

void FEOSWrapperUserManager::GetLinkedAccountAuthToken(int32 LocalUserNum, const FOnGetLinkedAccountAuthTokenCompleteDelegate& Delegate) const
{
	FExternalAuthToken ExternalToken;
	ExternalToken.TokenString = GetAuthToken(LocalUserNum);
	Delegate.ExecuteIfBound(LocalUserNum, ExternalToken.IsValid(), ExternalToken);
}

ELoginStatus::Type FEOSWrapperUserManager::GetLoginStatus(const FUniqueNetIdEOS& UserId) const
{
	const EOS_EpicAccountId AccountId = UserId.GetEpicAccountId();
	if (AccountId == nullptr)
	{
		return ELoginStatus::NotLoggedIn;
	}

	const EOS_ELoginStatus LoginStatus = EOS_Auth_GetLoginStatus(EOSSubsystem->GetAuthHandle(), AccountId);
	switch (LoginStatus)
	{
		case EOS_ELoginStatus::EOS_LS_LoggedIn:
		{
			return ELoginStatus::LoggedIn;
		}
		case EOS_ELoginStatus::EOS_LS_UsingLocalProfile:
		{
			return ELoginStatus::UsingLocalProfile;
		}
		default: return ELoginStatus::NotLoggedIn;
	}
}

FUniqueNetIdEOSPtr FEOSWrapperUserManager::GetLocalUniqueNetIdEOS(int32 LocalUserNum) const
{
	const FUniqueNetIdEOSPtr* FoundId = UserNumToNetIdMap.Find(LocalUserNum);
	if (FoundId != nullptr)
	{
		return *FoundId;
	}
	return nullptr;
}

bool FEOSWrapperUserManager::ConnectLoginEAS(int32 LocalUserNum, EOS_EpicAccountId AccountId)
{
	EOS_Auth_Token* AuthToken = nullptr;
	EOS_Auth_CopyUserAuthTokenOptions CopyOptions = {};
	CopyOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

	EOS_EResult CopyResult = EOS_Auth_CopyUserAuthToken(EOSSubsystem->GetAuthHandle(), &CopyOptions, AccountId, &AuthToken);
	if (CopyResult == EOS_EResult::EOS_Success)
	{
		EOS_Connect_Credentials Credentials = {};
		Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
		Credentials.Token = AuthToken->AccessToken;

		EOS_Connect_LoginOptions Options = {};
		Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;

		FConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());
		CallbackObj->CallbackLambda = [LocalUserNum, AccountId, this](const EOS_Connect_LoginCallbackInfo* Data) {
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				// We have an account mapping, skip to final login
				FullLoginCallback(LocalUserNum, AccountId, Data->LocalUserId);
			}
			else if (Data->ResultCode == EOS_EResult::EOS_InvalidUser)
			{
				// We need to create the mapping for this user using the continuation token
				CreateConnectedLogin(LocalUserNum, AccountId, Data->ContinuanceToken);
			}
			else
			{
				UE_LOG_ONLINE(Error, TEXT("ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
				Logout(LocalUserNum);
			}
		};
		EOS_Connect_Login(EOSSubsystem->GetConnectHandle(), &Options, CallbackObj, CallbackObj->GetCallbackPtr());

		EOS_Auth_Token_Release(AuthToken);
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(CopyResult)));
		Logout(LocalUserNum);
	}
	return true;
}

void FEOSWrapperUserManager::FullLoginCallback(int32 LocalUserNum, EOS_EpicAccountId AccountId, EOS_ProductUserId UserId)
{
	// Add our login status changed callback if not already set
	if (LoginNotificationId == 0)
	{
		FLoginStatusChangedCallback* CallbackObj = new FLoginStatusChangedCallback(AsWeak());
		LoginNotificationCallback = CallbackObj;
		CallbackObj->CallbackLambda = [this](const EOS_Auth_LoginStatusChangedCallbackInfo* Data) { LoginStatusChanged(Data); };

		EOS_Auth_AddNotifyLoginStatusChangedOptions Options = {};
		Options.ApiVersion = EOS_AUTH_ADDNOTIFYLOGINSTATUSCHANGED_API_LATEST;
		LoginNotificationId = EOS_Auth_AddNotifyLoginStatusChanged(EOSSubsystem->GetAuthHandle(), &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	}
	// Register for friends updates if not set yet
	// if (FriendsNotificationId == 0)
	// {
	// 	FFriendsStatusUpdateCallback* CallbackObj = new FFriendsStatusUpdateCallback(AsWeak());
	// 	FriendsNotificationCallback = CallbackObj;
	// 	CallbackObj->CallbackLambda = [LocalUserNum, this](const EOS_Friends_OnFriendsUpdateInfo* Data)
	// 	{
	// 		FriendStatusChanged(Data);
	// 	};
	//
	// 	EOS_Friends_AddNotifyFriendsUpdateOptions Options = { };
	// 	Options.ApiVersion = EOS_FRIENDS_ADDNOTIFYFRIENDSUPDATE_API_LATEST;
	// 	FriendsNotificationId = EOS_Friends_AddNotifyFriendsUpdate(EOSSubsystem->FriendsHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	// }
	// Register for presence updates if not set yet
	// if (PresenceNotificationId == 0)
	// {
	// 	FPresenceChangedCallback* CallbackObj = new FPresenceChangedCallback(AsWeak());
	// 	PresenceNotificationCallback = CallbackObj;
	// 	CallbackObj->CallbackLambda = [LocalUserNum, this](const EOS_Presence_PresenceChangedCallbackInfo* Data)
	// 	{
	// 		if (EpicAccountIdToOnlineUserMap.Contains(Data->PresenceUserId))
	// 		{
	// 			// Update the presence data to the most recent
	// 			UpdatePresence(Data->PresenceUserId);
	// 			return;
	// 		}
	// 	};
	//
	// 	EOS_Presence_AddNotifyOnPresenceChangedOptions Options = { };
	// 	Options.ApiVersion = EOS_PRESENCE_ADDNOTIFYONPRESENCECHANGED_API_LATEST;
	// 	PresenceNotificationId = EOS_Presence_AddNotifyOnPresenceChanged(EOSSubsystem->PresenceHandle, &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	// }
	// Add auth refresh notification if not set for this user yet
	if (!LocalUserNumToConnectLoginNotifcationMap.Contains(LocalUserNum))
	{
		FNotificationIdCallbackPair* NotificationPair = new FNotificationIdCallbackPair();
		LocalUserNumToConnectLoginNotifcationMap.Emplace(LocalUserNum, NotificationPair);

		FRefreshAuthCallback* CallbackObj = new FRefreshAuthCallback(AsWeak());
		NotificationPair->Callback = CallbackObj;
		CallbackObj->CallbackLambda = [LocalUserNum, this](const EOS_Connect_AuthExpirationCallbackInfo* Data) { RefreshConnectLogin(LocalUserNum); };

		EOS_Connect_AddNotifyAuthExpirationOptions Options = {};
		Options.ApiVersion = EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST;
		NotificationPair->NotificationId = EOS_Connect_AddNotifyAuthExpiration(EOSSubsystem->GetConnectHandle(), &Options, CallbackObj, CallbackObj->GetCallbackPtr());
	}

	AddLocalUser(LocalUserNum, AccountId, UserId);
	FUniqueNetIdEOSPtr UserNetId = GetLocalUniqueNetIdEOS(LocalUserNum);
	check(UserNetId.IsValid());

	TriggerOnLoginCompleteDelegates(LocalUserNum, true, *UserNetId, FString());
	TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::NotLoggedIn, ELoginStatus::LoggedIn, *UserNetId);
}

void FEOSWrapperUserManager::CreateConnectedLogin(int32 LocalUserNum, EOS_EpicAccountId AccountId, EOS_ContinuanceToken Token) {}

void FEOSWrapperUserManager::RefreshConnectLogin(int32 LocalUserNum)
{
	if (!UserNumToAccountIdMap.Contains(LocalUserNum))
	{
		UE_LOG_ONLINE(Error, TEXT("Can't refresh ConnectLogin(%d) since (%d) is not logged in"), LocalUserNum, LocalUserNum);
		return;
	}

	const FEOSWrapperSettings Settings = UEOSWrapperSettings::GetSettings();
	if (Settings.bUseEAS)
	{
		EOS_EpicAccountId AccountId = UserNumToAccountIdMap[LocalUserNum];
		EOS_Auth_Token* AuthToken = nullptr;
		EOS_Auth_CopyUserAuthTokenOptions CopyOptions = {};
		CopyOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

		EOS_EResult CopyResult = EOS_Auth_CopyUserAuthToken(EOSSubsystem->GetAuthHandle(), &CopyOptions, AccountId, &AuthToken);
		if (CopyResult == EOS_EResult::EOS_Success)
		{
			// We update the auth token cached in the user account, along with the user information
			const FUniqueNetIdEOSPtr UniqueNetId = UserNumToNetIdMap.FindChecked(LocalUserNum);
			const FUserOnlineAccountEOSRef UserAccountRef = StringToUserAccountMap.FindChecked(UniqueNetId->ToString());
			UserAccountRef->SetAuthAttribute(AUTH_ATTR_ID_TOKEN, AuthToken->AccessToken);
			UpdateUserInfo(UserAccountRef, AccountId, AccountId);

			EOS_Connect_Credentials Credentials = {};
			Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
			Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
			Credentials.Token = AuthToken->AccessToken;

			EOS_Connect_LoginOptions Options = {};
			Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
			Options.Credentials = &Credentials;

			FConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());
			CallbackObj->CallbackLambda = [LocalUserNum, AccountId, this](const EOS_Connect_LoginCallbackInfo* Data) {
				if (Data->ResultCode != EOS_EResult::EOS_Success)
				{
					UE_LOG_ONLINE(Error, TEXT("Failed to refresh ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
					Logout(LocalUserNum);
				}
			};
			EOS_Connect_Login(EOSSubsystem->GetConnectHandle(), &Options, CallbackObj, CallbackObj->GetCallbackPtr());

			EOS_Auth_Token_Release(AuthToken);
		}
		else
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to refresh ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(CopyResult)));
			Logout(LocalUserNum);
		}
	}
	else
	{
		// // Not using EAS so grab the platform auth token
		// GetPlatformAuthToken(LocalUserNum,
		// 	FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateLambda([this, WeakThis = AsWeak()](int32 LocalUserNum, bool bWasSuccessful, const FExternalAuthToken& AuthToken)
		// 	{
		// 		if (FUserManagerEOSPtr StrongThis = WeakThis.Pin())
		// 		{
		// 			if (!bWasSuccessful || !AuthToken.IsValid())
		// 			{
		// 				UE_LOG_ONLINE(Error, TEXT("ConnectLoginNoEAS(%d) failed due to the platform OSS giving an empty auth token"), LocalUserNum);
		// 				Logout(LocalUserNum);
		// 				return;
		// 			}
		//
		// 			// Now login into our EOS account
		// 			check(LocalUserNumToLastLoginCredentials.Contains(LocalUserNum));
		// 			const FOnlineAccountCredentials& Creds = *LocalUserNumToLastLoginCredentials[LocalUserNum];
		// 			EOS_EExternalCredentialType CredType = ToEOS_EExternalCredentialType(GetPlatformOSS()->GetSubsystemName(), Creds);
		// 			FConnectCredentials Credentials(CredType, AuthToken);
		// 			EOS_Connect_LoginOptions Options = { };
		// 			Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		// 			Options.Credentials = &Credentials;
		//
		// 			FConnectLoginCallback* CallbackObj = new FConnectLoginCallback(AsWeak());
		// 			CallbackObj->CallbackLambda = [this, LocalUserNum](const EOS_Connect_LoginCallbackInfo* Data)
		// 			{
		// 				if (Data->ResultCode != EOS_EResult::EOS_Success)
		// 				{
		// 					UE_LOG_ONLINE(Error, TEXT("Failed to refresh ConnectLogin(%d) failed with EOS result code (%s)"), LocalUserNum, ANSI_TO_TCHAR(EOS_EResult_ToString(Data->ResultCode)));
		// 					Logout(LocalUserNum);
		// 				}
		// 			};
		// 			EOS_Connect_Login(EOSSubsystem->GetConnectHandle(), &Options, CallbackObj, CallbackObj->GetCallbackPtr());
		// 		}
		// 	}));
	}
}

void FEOSWrapperUserManager::UpdateUserInfo(IAttributeAccessInterfaceRef AttriubteAccessRef, EOS_EpicAccountId LocalId, EOS_EpicAccountId AccountId)
{
	EOS_UserInfo_CopyUserInfoOptions Options = {};
	Options.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
	Options.LocalUserId = LocalId;
	Options.TargetUserId = AccountId;

	EOS_UserInfo* UserInfo = nullptr;

	// EOS_EResult CopyResult = EOS_UserInfo_CopyUserInfo(EOSSubsystem->GetUserInfoHandle(), &Options, &UserInfo);
	// if (CopyResult == EOS_EResult::EOS_Success)
	// {
	// 	AttributeAccessRef->SetInternalAttribute(USER_ATTR_DISPLAY_NAME, UTF8_TO_TCHAR(UserInfo->DisplayName));
	// 	AttributeAccessRef->SetInternalAttribute(USER_ATTR_COUNTRY, UTF8_TO_TCHAR(UserInfo->Country));
	// 	AttributeAccessRef->SetInternalAttribute(USER_ATTR_LANG, UTF8_TO_TCHAR(UserInfo->PreferredLanguage));
	// 	EOS_UserInfo_Release(UserInfo);
	// }
}

void FEOSWrapperUserManager::LoginStatusChanged(const EOS_Auth_LoginStatusChangedCallbackInfo* Data)
{
	if (Data->CurrentStatus == EOS_ELoginStatus::EOS_LS_NotLoggedIn)
	{
		if (AccountIdToUserNumMap.Contains(Data->LocalUserId))
		{
			int32 LocalUserNum = AccountIdToUserNumMap[Data->LocalUserId];
			FUniqueNetIdEOSPtr UserNetId = UserNumToNetIdMap[LocalUserNum];
			TriggerOnLoginStatusChangedDelegates(LocalUserNum, ELoginStatus::LoggedIn, ELoginStatus::NotLoggedIn, *UserNetId);
			// Need to remove the local user
			RemoveLocalUser(LocalUserNum);

			// Clean up user based notifies if we have no logged in users
			if (UserNumToNetIdMap.Num() == 0)
			{
				if (LoginNotificationId > 0)
				{
					// Remove the callback
					EOS_Auth_RemoveNotifyLoginStatusChanged(EOSSubsystem->GetAuthHandle(), LoginNotificationId);
					delete LoginNotificationCallback;
					LoginNotificationCallback = nullptr;
					LoginNotificationId = 0;
				}
				// if (FriendsNotificationId > 0)
				// {
				// 	EOS_Friends_RemoveNotifyFriendsUpdate(EOSSubsystem->FriendsHandle, FriendsNotificationId);
				// 	delete FriendsNotificationCallback;
				// 	FriendsNotificationCallback = nullptr;
				// 	FriendsNotificationId = 0;
				// }
				// if (PresenceNotificationId > 0)
				// {
				// 	EOS_Presence_RemoveNotifyOnPresenceChanged(EOSSubsystem->PresenceHandle, PresenceNotificationId);
				// 	delete PresenceNotificationCallback;
				// 	PresenceNotificationCallback = nullptr;
				// 	PresenceNotificationId = 0;
				// }
				// Remove the per user connect login notification
				if (LocalUserNumToConnectLoginNotifcationMap.Contains(LocalUserNum))
				{
					FNotificationIdCallbackPair* NotificationPair = LocalUserNumToConnectLoginNotifcationMap[LocalUserNum];
					LocalUserNumToConnectLoginNotifcationMap.Remove(LocalUserNum);

					EOS_Connect_RemoveNotifyAuthExpiration(EOSSubsystem->GetConnectHandle(), NotificationPair->NotificationId);

					delete NotificationPair;
				}
			}
		}
	}
}

int32 FEOSWrapperUserManager::GetLocalUserNumFromUniqueNetId(const FUniqueNetId& NetId) const
{
	const FUniqueNetIdEOS& EosId = FUniqueNetIdEOS::Cast(NetId);

	const EOS_EpicAccountId AccountId = EosId.GetEpicAccountId();
	if (const int32* UserNum = AccountIdToUserNumMap.Find(AccountId))
	{
		return *UserNum;
	}

	const EOS_ProductUserId ProductUserId = EosId.GetProductUserId();
	if (const int32* UserNum = ProductUserIdToUserNumMap.Find(ProductUserId))
	{
		return *UserNum;
	}

	// Use the default user if we can't find the person that they want
	return DefaultLocalUser;
}

void FEOSWrapperUserManager::RemoveLocalUser(int32 LocalUserNum)
{
	const FUniqueNetIdEOSPtr* FoundId = UserNumToNetIdMap.Find(LocalUserNum);
	if (FoundId != nullptr)
	{
		// EOSSubsystem->ReleaseVoiceChatUserInterface(**FoundId);
		//	LocalUserNumToFriendsListMap.Remove(LocalUserNum);
		const FString& NetId = (*FoundId)->ToString();
		const EOS_EpicAccountId AccountId = (*FoundId)->GetEpicAccountId();
		AccountIdToStringMap.Remove(AccountId);
		AccountIdToUserNumMap.Remove(AccountId);
		// NetIdStringToOnlineUserMap.Remove(NetId);
		StringToUserAccountMap.Remove(NetId);
		UserNumToNetIdMap.Remove(LocalUserNum);
		UserNumToAccountIdMap.Remove(LocalUserNum);
		EOS_ProductUserId UserId = UserNumToProductUserIdMap[LocalUserNum];
		ProductUserIdToUserNumMap.Remove(UserId);

		UserNumToProductUserIdMap.Remove(LocalUserNum);
	}
	// Reset this for the next user login
	if (LocalUserNum == DefaultLocalUser)
	{
		DefaultLocalUser = -1;
	}
}

void FEOSWrapperUserManager::AddLocalUser(int32 LocalUserNum, EOS_EpicAccountId EpicAccountId, EOS_ProductUserId UserId)
{
	// Set the default user to the first one that logs in
	if (DefaultLocalUser == -1)
	{
		DefaultLocalUser = LocalUserNum;
	}

	FUniqueNetIdEOSRef UserNetId = FUniqueNetIdEOSRegistry::FindOrAdd(EpicAccountId, UserId).ToSharedRef();
	const FString& NetId = UserNetId->ToString();
	FUserOnlineAccountEOSRef UserAccountRef(new FUserOnlineAccountEOS(UserNetId));

	UserNumToNetIdMap.Emplace(LocalUserNum, UserNetId);
	UserNumToAccountIdMap.Emplace(LocalUserNum, EpicAccountId);
	AccountIdToUserNumMap.Emplace(EpicAccountId, LocalUserNum);
	// NetIdStringToOnlineUserMap.Emplace(*NetId, UserAccountRef);
	// StringToUserAccountMap.Emplace(NetId, UserAccountRef);
	// AccountIdToStringMap.Emplace(EpicAccountId, NetId);
	// ProductUserIdToStringMap.Emplace(UserId, *NetId);
	// EpicAccountIdToAttributeAccessMap.Emplace(EpicAccountId, UserAccountRef);
	UserNumToProductUserIdMap.Emplace(LocalUserNum, UserId);
	ProductUserIdToUserNumMap.Emplace(UserId, LocalUserNum);

	// Init player lists
	// FFriendsListEOSRef FriendsList = MakeShareable(new FFriendsListEOS(LocalUserNum, UserNetId));
	// LocalUserNumToFriendsListMap.Emplace(LocalUserNum, FriendsList);
	// NetIdStringToFriendsListMap.Emplace(NetId, FriendsList);
	// ReadFriendsList(LocalUserNum, FString());
	//
	// FBlockedPlayersListEOSRef BlockedPlayersList = MakeShareable(new FBlockedPlayersListEOS(LocalUserNum, UserNetId));
	// LocalUserNumToBlockedPlayerListMap.Emplace(LocalUserNum, BlockedPlayersList);
	// NetIdStringToBlockedPlayerListMap.Emplace(NetId, BlockedPlayersList);
	// QueryBlockedPlayers(*UserNetId);
	//
	// FRecentPlayersListEOSRef RecentPlayersList = MakeShareable(new FRecentPlayersListEOS(LocalUserNum, UserNetId));
	// LocalUserNumToRecentPlayerListMap.Emplace(LocalUserNum, RecentPlayersList);
	// NetIdStringToRecentPlayerListMap.Emplace(NetId, RecentPlayersList);

	// Get auth token info
	EOS_Auth_Token* AuthToken = nullptr;
	EOS_Auth_CopyUserAuthTokenOptions Options = {};
	Options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

	EOS_EResult CopyResult = EOS_Auth_CopyUserAuthToken(EOSSubsystem->GetAuthHandle(), &Options, EpicAccountId, &AuthToken);
	if (CopyResult == EOS_EResult::EOS_Success)
	{
		UserAccountRef->SetAuthAttribute(AUTH_ATTR_ID_TOKEN, AuthToken->AccessToken);
		EOS_Auth_Token_Release(AuthToken);

		// UpdateUserInfo(UserAccountRef, EpicAccountId, EpicAccountId);
	}
}

#pragma optimize("", on)
