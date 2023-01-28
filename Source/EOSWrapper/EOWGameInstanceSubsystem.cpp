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
}

void UEOWGameInstanceSubsystem::IsUserLoggedIn(bool& LoggedIn, int32 LocalUserNum) const
{
	LoggedIn = false;
	if (!EOSSubsystem) return;
}
