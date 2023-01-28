// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOSWrapperLobby.h"

#if WITH_EOS_SDK
#include "eos_logging.h"
#include "eos_lobby.h"
#include "eos_lobby_types.h"
#include "eos_init.h"
#include "eos_sdk.h"
#include "eos_auth.h"

#pragma optimize("", off)

/** This is the game name plus version in ansi done once for optimization */
char BucketIdAnsi[EOS_OSS_STRING_BUFFER_LENGTH];

FEOSWrapperLobby::FEOSWrapperLobby()
{
	FString ProductId = TEXT("b592962ba3834c42a85866aa344e410c");
	FString SandboxId = TEXT("232af36c61c84711a41b15f455053564");
	FString ClientId = TEXT("xyza7891iKQkmri5BVE7aRwbm15eiFsU");
	FString ClientSecret = TEXT("hwaakpaw9E7iIcJ9ib67jZGiq3NUFNwpqJHRtmDJPN8");
	FString EncryptionKey = TEXT("3273357638792F423F4528482B4D6251655468576D597133743677397A244326");
	FString DeploymentId = TEXT("fc423573e558485d85347f1cb6c8f1c3");

	// FString ProductId = TEXT("b592962ba3834c42a85866aa344e410c");
	// FString SandboxId = TEXT("232af36c61c84711a41b15f455053564");
	// FString ClientId = TEXT("xyza7891BE28P0kgqAL0zlHh2h8w5JQc");
	// FString ClientSecret = TEXT("DWdlDHRx3CYcxOLELULPLl7ePE8zdQs9s0y6LyIjOgQ");
	// FString EncryptionKey = TEXT("3273357638792F423F4528482B4D6251655468576D597133743677397A244326");
	// FString DeploymentId = TEXT("fc423573e558485d85347f1cb6c8f1c3");

	FString CacheDirBase = FPlatformProcess::UserDir();

	const FTCHARToUTF8 Utf8ProductId(*ProductId);
	const FTCHARToUTF8 Utf8SandboxId(*SandboxId);
	const FTCHARToUTF8 Utf8ClientId(*ClientId);
	const FTCHARToUTF8 Utf8ClientSecret(*ClientSecret);
	const FTCHARToUTF8 Utf8EncryptionKey(*EncryptionKey);
	const FTCHARToUTF8 Utf8DeploymentId(*DeploymentId);
	const FTCHARToUTF8 Utf8CacheDirectory(*(CacheDirBase / TEXT("OnlineServicesEOS")));

	EOS_Platform_Options PlatformOptions = {};
	PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
	PlatformOptions.Reserved = nullptr;
	PlatformOptions.bIsServer = EOS_FALSE;	// EOS_TRUE;//
	PlatformOptions.OverrideCountryCode = nullptr;
	PlatformOptions.OverrideLocaleCode = nullptr;

	// Can't check GIsEditor here because it is too soon!
	if (!IsRunningGame())
	{
		PlatformOptions.Flags = EOS_PF_LOADING_IN_EDITOR;
	}
	else
	{
		PlatformOptions.Flags = EOS_PF_WINDOWS_ENABLE_OVERLAY_D3D9 | EOS_PF_WINDOWS_ENABLE_OVERLAY_D3D10 | EOS_PF_WINDOWS_ENABLE_OVERLAY_OPENGL;
		// Enable overlay support for D3D9/10 and OpenGL. This sample uses D3D11 or SDL.
	}

	PlatformOptions.ProductId = Utf8ProductId.Get();
	PlatformOptions.SandboxId = Utf8SandboxId.Get();
	PlatformOptions.DeploymentId = Utf8DeploymentId.Get();
	PlatformOptions.ClientCredentials.ClientId = Utf8ClientId.Get();
	PlatformOptions.ClientCredentials.ClientSecret = Utf8ClientSecret.Get();
	PlatformOptions.EncryptionKey = Utf8EncryptionKey.Get();
	PlatformOptions.CacheDirectory = Utf8CacheDirectory.Get();

	PlatformHandle = EOS_Platform_Create(&PlatformOptions);

	if (!PlatformHandle) return;

	AuthHandle = EOS_Platform_GetAuthInterface(PlatformHandle);
	check(AuthHandle != nullptr);

	ConnectHandle = EOS_Platform_GetConnectInterface(PlatformHandle);
	check(ConnectHandle != nullptr);

	FString Portal = TEXT("localhost:8082");
	FString Account = TEXT("Viki");

	EOS_Auth_Credentials Credentials = {};
	Credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
	Credentials.Id = TCHAR_TO_ANSI(*Portal);
	Credentials.Token = TCHAR_TO_ANSI(*Account);
	Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;

	EOS_Auth_LoginOptions LoginOptions;
	// memset(&LoginOptions, 0, sizeof(LoginOptions));
	LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
	LoginOptions.Credentials = &Credentials;
	LoginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

	EOS_Auth_Login(AuthHandle, &LoginOptions, this, LoginCompleteCallbackFn);
}

FEOSWrapperLobby::~FEOSWrapperLobby() {}

void FEOSWrapperLobby::CreateLobby()
{
	const FString BucketID = TEXT("OfficeProject_0");
	FCStringAnsi::Strncpy(BucketIdAnsi, TCHAR_TO_UTF8(*BucketID), EOS_OSS_STRING_BUFFER_LENGTH);

	// = EOSSubsystem->UserManager->GetLocalProductUserId(HostingPlayerNum);
	// const FUniqueNetIdPtr LocalUserNetId;// = EOSSubsystem->UserManager->GetLocalUniqueNetIdEOS(HostingPlayerNum);

	EOS_Lobby_CreateLobbyOptions CreateLobbyOptions = {0};
	CreateLobbyOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	CreateLobbyOptions.LocalUserId = ProductUserID;	 // LocalProductUserId;
	CreateLobbyOptions.MaxLobbyMembers = 10;
	CreateLobbyOptions.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;
	CreateLobbyOptions.bPresenceEnabled = EOS_TRUE;
	CreateLobbyOptions.bAllowInvites = EOS_TRUE;
	CreateLobbyOptions.BucketId = BucketIdAnsi;
	CreateLobbyOptions.bDisableHostMigration = EOS_FALSE;
	CreateLobbyOptions.bEnableRTCRoom = EOS_FALSE;
	CreateLobbyOptions.LocalRTCOptions = nullptr;
	CreateLobbyOptions.LobbyId = nullptr;
	CreateLobbyOptions.bEnableJoinById = 0;
	CreateLobbyOptions.bRejoinAfterKickRequiresInvite = 0;

	const EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(PlatformHandle);
	if (LobbyHandle)
	{
		EOS_Lobby_CreateLobby(LobbyHandle, &CreateLobbyOptions, this, OnCreateLobbyFinished);
	}
}

void FEOSWrapperLobby::Tick()
{
	EOS_Platform_Tick(PlatformHandle);

	const int32_t ConnectLoggedInAccountsCount = EOS_Connect_GetLoggedInUsersCount(ConnectHandle);
	if (ConnectLoggedInAccountsCount > 0 && !bLobbyCreated)
	{
		const EOS_ProductUserId LoggedInAccount = EOS_Connect_GetLoggedInUserByIndex(ConnectHandle, 0);
		ProductUserID = LoggedInAccount;
		CreateLobby();
		bLobbyCreated = true;
	}
}

void FEOSWrapperLobby::ConnectLogin()
{
	EOS_Auth_Token* UserAuthToken = nullptr;
	check(AuthHandle != nullptr);

	EOS_Auth_CopyUserAuthTokenOptions CopyTokenOptions = {0};
	CopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

	if (EOS_Auth_CopyUserAuthToken(AuthHandle, &CopyTokenOptions, CurrentUserID, &UserAuthToken) == EOS_EResult::EOS_Success)
	{
		EOS_Connect_Credentials Credentials;
		Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
		Credentials.Token = UserAuthToken->AccessToken;
		Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;

		EOS_Connect_LoginOptions Options = {0};
		Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
		Options.Credentials = &Credentials;
		Options.UserLoginInfo = nullptr;

		// Setup a context so the callback knows what AccountId is logging in.
		// std::unique_ptr<FConnectLoginContext> ClientData(new FConnectLoginContext);
		//	ClientData->AccountId = UserId.AccountId;

		check(ConnectHandle != nullptr);
		EOS_Connect_Login(ConnectHandle, &Options, this, ConnectLoginCompleteCb);
		EOS_Auth_Token_Release(UserAuthToken);
	}
}

void EOS_CALL FEOSWrapperLobby::OnCreateLobbyFinished(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	check(Data);
}

void EOS_CALL FEOSWrapperLobby::LoginCompleteCallbackFn(const EOS_Auth_LoginCallbackInfo* Data)
{
	check(Data);
	if (Data->ResultCode != EOS_EResult::EOS_Success) return;
	auto* EOSWrapperLobby = reinterpret_cast<FEOSWrapperLobby*>(Data->ClientData);

	EOS_HAuth AuthHandle = EOS_Platform_GetAuthInterface(EOSWrapperLobby->PlatformHandle);
	check(AuthHandle != nullptr);

	const int32_t AccountsCount = EOS_Auth_GetLoggedInAccountsCount(AuthHandle);
	for (int32_t AccountIdx = 0; AccountIdx < AccountsCount; ++AccountIdx)
	{
		EOS_EpicAccountId AccountId = EOS_Auth_GetLoggedInAccountByIndex(AuthHandle, AccountIdx);
		EOS_ELoginStatus LoginStatus;
		LoginStatus = EOS_Auth_GetLoginStatus(AuthHandle, Data->LocalUserId);
	}

	EOSWrapperLobby->CurrentUserID = Data->SelectedAccountId;
	EOSWrapperLobby->ConnectLogin();
}

void EOS_CALL FEOSWrapperLobby::ConnectLoginCompleteCb(const EOS_Connect_LoginCallbackInfo* Data)
{
	auto* EOSWrapperLobby = reinterpret_cast<FEOSWrapperLobby*>(Data->ClientData);
	EOSWrapperLobby->ProductUserID = Data->LocalUserId;
}

#pragma optimize("", on)
#endif
