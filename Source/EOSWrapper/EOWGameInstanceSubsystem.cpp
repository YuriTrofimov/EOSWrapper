// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOWGameInstanceSubsystem.h"
#include "EOSWrapperSessionManager.h"
#include "EOSWrapperModule.h"
#include "EOSWrapperUserManager.h"

void UEOWGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	auto* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem) return;

	OnlineSubsystem = static_cast<FEOSWrapperSubsystem*>(Subsystem);
	if (!OnlineSubsystem) return;

	OnlineSubsystem->GetLobbyManager()->OnLobbyUpdated.AddUObject(this, &UEOWGameInstanceSubsystem::OnLobbyUpdatedCallback);
}

void UEOWGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();
	if (OnlineSubsystem)
	{
		OnlineSubsystem->GetLobbyManager()->OnLobbyUpdated.RemoveAll(this);
	}
	OnlineSubsystem = nullptr;
}

void UEOWGameInstanceSubsystem::DeveloperLogin(FString Host, FString Account)
{
}

void UEOWGameInstanceSubsystem::IsUserLoggedIn(bool& LoggedIn, int32 LocalUserNum) const
{
	LoggedIn = false;
}


void UEOWGameInstanceSubsystem::GetUserToken(int32 LocalUserNum, FString& Token, FString& UserAccountString)
{
	if (!OnlineSubsystem) return;

	const IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
	if (!IdentityInterface) return;

	if (FEOSWrapperUserManager* UserManager = static_cast<FEOSWrapperUserManager*>(IdentityInterface.Get()))
	{
		UserManager->GetUserAuthToken(LocalUserNum, Token, UserAccountString);
	}
}

void UEOWGameInstanceSubsystem::CreateLobby(int32 LocalUserNum, FName LobbyName)
{
// 	if (!OnlineSubsystem) return;
//
// 	if (FEOSWrapperLobby* LobbyManager = OnlineSubsystem->GetLobbyManager())
// 	{
// 		LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
// 		LastSessionSettings->bIsDedicated = false;
// 		LastSessionSettings->bShouldAdvertise = true;
// 		LastSessionSettings->bIsLANMatch = false;
// 		LastSessionSettings->NumPrivateConnections = 2;
// 		LastSessionSettings->NumPublicConnections = 2;
// 		LastSessionSettings->bAllowJoinInProgress = true;
// 		LastSessionSettings->bAllowJoinViaPresence = true;
// 		LastSessionSettings->bUsesPresence = true;
// 		LastSessionSettings->bUseLobbiesIfAvailable = true;
// 		LastSessionSettings->bAllowInvites = true;
// 		LastSessionSettings->bUseLobbiesVoiceChatIfAvailable = false;
// 		LastSessionSettings->bUsesStats = true;
// 		LastSessionSettings->Set(SEARCH_KEYWORDS, FString("DevSession"), EOnlineDataAdvertisementType::ViaOnlineService);
//
// 		TFunction<void(bool bSuccess, EOS_LobbyId LobbyID)> Callback = [this, LobbyName](bool bSuccess, EOS_LobbyId LobbyID)
// 		{
// 			//OnTokenValidationComplete.Broadcast(bSuccess, Token);
// 		};
// //		LobbyManager->CreateLobby(LobbyName, LocalUserNum, *LastSessionSettings, Callback);
// 	}
}

void UEOWGameInstanceSubsystem::SetLobbyAttributeString(FName LobbyName, FName Key, FString Value)
{
	if (!OnlineSubsystem) return;

	if (FEOSWrapperSessionManager* LobbyManager = OnlineSubsystem->GetLobbyManager())
	{
		LobbyManager->SetLobbyParameter(LobbyName, Key, Value);
	}
}

void UEOWGameInstanceSubsystem::ValidateUserAuthToken(const FString& Token, const FString& AccountIDString) const
{
	if (!OnlineSubsystem) return;

	const IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
	if (!IdentityInterface) return;

	if (FEOSWrapperUserManager* UserManager = static_cast<FEOSWrapperUserManager*>(IdentityInterface.Get()))
	{
		TFunction<void(FString InToken, EOS_EpicAccountId AccountID, bool bSuccess)> Callback = [this, Token, AccountIDString](FString InToken, EOS_EpicAccountId AccountID, bool bSuccess)
		{
			OnTokenValidationComplete.Broadcast(bSuccess, Token);
		};
		UserManager->ValidateUserAuthToken(Token, AccountIDString, Callback);
	}
}

void UEOWGameInstanceSubsystem::OnLobbyUpdatedCallback(const FName& LobbySessionName)
{
	OnLobbyUpdated.Broadcast(LobbySessionName);
}
