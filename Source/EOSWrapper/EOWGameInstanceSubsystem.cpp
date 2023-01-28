// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOWGameInstanceSubsystem.h"
#include "EOSWrapperModule.h"
#include "EOSWrapperUserManager.h"

void UEOWGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const FEOSWrapperModule* WrapperModule = static_cast<FEOSWrapperModule*>(FModuleManager::Get().GetModule("EOSWrapper"));
	if (!WrapperModule) return;

	//	EOSSubsystem = WrapperModule->EOSWrapperSubsystem.Get();
}

void UEOWGameInstanceSubsystem::Deinitialize()
{
	Super::Deinitialize();
	EOSSubsystem = nullptr;
}

void UEOWGameInstanceSubsystem::DeveloperLogin(FString Host, FString Account)
{
	if (!EOSSubsystem) return;
	auto* UserManager = EOSSubsystem->GetUserManager();
	if (!UserManager) return;

	//	UserManager->DeveloperLogin(Host, Account);
}

void UEOWGameInstanceSubsystem::IsUserLoggedIn(bool& LoggedIn, int32 LocalUserNum) const
{
	LoggedIn = false;

	if (!EOSSubsystem) return;
	const auto* UserManager = EOSSubsystem->GetUserManager();
	if (!UserManager) return;

	const auto& LoginStatus = UserManager->GetLoginStatus(LocalUserNum);
	LoggedIn = LoginStatus == ELoginStatus::LoggedIn;
}
