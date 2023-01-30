// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOSWrapperModule.h"
#include "EOSWrapperSettings.h"
#include "EOSWrapperSubsystem.h"
#include "EOSWrapperTypes.h"
#include "Misc/LazySingleton.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#endif

#define LOCTEXT_NAMESPACE "EOSWrapper"

/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FOnlineFactoryEOSWrapper : public IOnlineFactory
{
public:
	FOnlineFactoryEOSWrapper()
	{
	}

	virtual ~FOnlineFactoryEOSWrapper()
	{
	}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FEOSWrapperSubsystemPtr OnlineSub = MakeShared<FEOSWrapperSubsystem, ESPMode::ThreadSafe>(InstanceName);
		if (!OnlineSub->Init())
		{
			UE_LOG_ONLINE(Warning, TEXT("EOS WRAPPER API failed to initialize!"));
			OnlineSub->Shutdown();
			OnlineSub = nullptr;
		}

		return OnlineSub;
	}
};

void FEOSWrapperModule::StartupModule()
{
	if (IsRunningCommandlet())
	{
		return;
	}

	EOSWrapperFactory = new FOnlineFactoryEOSWrapper();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(EOS_SUBSYSTEM, EOSWrapperFactory);

#if WITH_EOS_SDK
	// Have to call this as early as possible in order to hook the rendering device
	FEOSWrapperSubsystem::ModuleInit();
#endif

#if WITH_EDITOR
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FEOSWrapperModule::OnPostEngineInit);
	FCoreDelegates::OnPreExit.AddRaw(this, &FEOSWrapperModule::OnPreExit);
#endif
}

#if WITH_EDITOR
void FEOSWrapperModule::OnPostEngineInit()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "EOS Wrapper", LOCTEXT("OSSEOSSettingsName", "Online Subsystem EOS"),
			LOCTEXT("OSSEOSSettingsDescription", "Configure Online Subsystem EOS"), GetMutableDefault<UEOSWrapperSettings>());
	}
}
#endif

#if WITH_EDITOR
void FEOSWrapperModule::OnPreExit()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "EOS Wrapper");
	}
}
#endif

void FEOSWrapperModule::ShutdownModule()
{
	if (IsRunningCommandlet())
	{
		return;
	}

#if WITH_EDITOR
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	FCoreDelegates::OnPreExit.RemoveAll(this);
#endif

#if WITH_EOS_SDK
	FEOSWrapperSubsystem::ModuleShutdown();
#endif

	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(EOS_SUBSYSTEM);

	delete EOSWrapperFactory;
	EOSWrapperFactory = nullptr;

	TLazySingleton<FUniqueNetIdEOSRegistry>::TearDown();
}

IMPLEMENT_MODULE(FEOSWrapperModule, EOSWrapper)

#undef LOCTEXT_NAMESPACE
