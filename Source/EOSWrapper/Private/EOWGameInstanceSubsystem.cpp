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
}

void UEOWGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();
	OnlineSubsystem = nullptr;
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
