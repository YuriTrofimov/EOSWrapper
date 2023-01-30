// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "WrapperSDKManager.h"

#include "EOSWrapperSettings.h"
#include "EOSWrapperTypes.h"

#if WITH_EOS_SDK
#include "eos_sdk.h"
#include "eos_init.h"
#include "eos_logging.h"

DEFINE_LOG_CATEGORY(EOSWrapperSDKManager);

FWrapperSDKManager::FWrapperSDKManager()
{
	ProductVersion = TEXT("1.0");
}

FWrapperSDKManager::~FWrapperSDKManager()
{
}

bool FWrapperSDKManager::Initialize()
{
	if (bInitialized) return true;

	UE_LOG(EOSWrapperSDKManager, Display, TEXT("EOSWrapperSubsystem INITIALIZE EOS SDK"));
	EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);

	FEOSWrapperSettings Settings = UEOSWrapperSettings::GetSettings();

	EOS_InitializeOptions SDKOptions = {};
	SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	SDKOptions.AllocateMemoryFunction = nullptr;
	SDKOptions.ReallocateMemoryFunction = nullptr;
	SDKOptions.ReleaseMemoryFunction = nullptr;
	SDKOptions.ProductName = TCHAR_TO_ANSI(TEXT("OfficeProject"));
	SDKOptions.ProductVersion = TCHAR_TO_ANSI(*ProductVersion);
	SDKOptions.Reserved = nullptr;
	SDKOptions.SystemInitializeOptions = nullptr;
	SDKOptions.OverrideThreadAffinity = nullptr;

	EOS_EResult InitResult = EOS_Initialize(&SDKOptions);
	if (InitResult != EOS_EResult::EOS_Success)
	{
		UE_LOG(EOSWrapperSDKManager, Warning, TEXT("EOS_Initialize failed error"));
	}

#if UE_SERVER
		FString ProductId = TEXT("b592962ba3834c42a85866aa344e410c");
		FString SandboxId = TEXT("232af36c61c84711a41b15f455053564");
		FString ClientId = TEXT("xyza7891BE28P0kgqAL0zlHh2h8w5JQc");
		FString ClientSecret = TEXT("DWdlDHRx3CYcxOLELULPLl7ePE8zdQs9s0y6LyIjOgQ");
		FString EncryptionKey = TEXT("3273357638792F423F4528482B4D6251655468576D597133743677397A244326");
		FString DeploymentId = TEXT("fc423573e558485d85347f1cb6c8f1c3");
#else
	FString ProductId = TEXT("b592962ba3834c42a85866aa344e410c");
	FString SandboxId = TEXT("232af36c61c84711a41b15f455053564");
	FString ClientId = TEXT("xyza7891iKQkmri5BVE7aRwbm15eiFsU");
	FString ClientSecret = TEXT("hwaakpaw9E7iIcJ9ib67jZGiq3NUFNwpqJHRtmDJPN8");
	FString EncryptionKey = TEXT("3273357638792F423F4528482B4D6251655468576D597133743677397A244326");
	FString DeploymentId = TEXT("fc423573e558485d85347f1cb6c8f1c3");
#endif

	// #if UE_SERVER
	// 	FString ProductId = Settings.ServerArtifacts.ProductId;
	// 	FString SandboxId = Settings.ServerArtifacts.SandboxId;
	// 	FString ClientId = Settings.ServerArtifacts.ClientId;
	// 	FString ClientSecret = Settings.ServerArtifacts.ClientSecret;
	// 	FString EncryptionKey = Settings.ServerArtifacts.EncryptionKey;
	// 	FString DeploymentId = Settings.ServerArtifacts.DeploymentId;
	// #else
	// 	FEOSArtifactSettings ClientArtifacts;
	// 	UEOSWrapperSettings::GetSettingsForArtifact(Settings.DefaultArtifactName, ClientArtifacts);
	//
	// 	FString ProductId = ClientArtifacts.ProductId;
	// 	FString SandboxId = ClientArtifacts.SandboxId;
	// 	FString ClientId = ClientArtifacts.ClientId;
	// 	FString ClientSecret = ClientArtifacts.ClientSecret;
	// 	FString EncryptionKey = ClientArtifacts.EncryptionKey;
	// 	FString DeploymentId = ClientArtifacts.DeploymentId;
	// #endif

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
	PlatformOptions.OverrideCountryCode = nullptr;
	PlatformOptions.OverrideLocaleCode = nullptr;

#if UE_SERVER
	PlatformOptions.bIsServer = EOS_TRUE;
	PlatformOptions.Flags = EOS_PF_DISABLE_OVERLAY;
#else
	PlatformOptions.bIsServer = EOS_FALSE;

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
#endif

	PlatformOptions.ProductId = Utf8ProductId.Get();
	PlatformOptions.SandboxId = Utf8SandboxId.Get();
	PlatformOptions.DeploymentId = Utf8DeploymentId.Get();
	PlatformOptions.ClientCredentials.ClientId = Utf8ClientId.Get();
	PlatformOptions.ClientCredentials.ClientSecret = Utf8ClientSecret.Get();
	PlatformOptions.EncryptionKey = Utf8EncryptionKey.Get();
	PlatformOptions.CacheDirectory = Utf8CacheDirectory.Get();

	PlatformHandle = EOS_Platform_Create(&PlatformOptions);

	if (!PlatformHandle)
	{
		UE_LOG(EOSWrapperSDKManager, Error, TEXT("Failed to init EOS PLATFORM"));
		return false;
	}

	EOS_EResult EosResult = EOS_Logging_SetCallback(&EOSLogMessageReceived);
	if (EosResult != EOS_EResult::EOS_Success)
	{
		UE_LOG(EOSWrapperSDKManager, Warning, TEXT("EOS_Logging_SetCallback failed error:%s"), *LexToString(EosResult));
	}

	UE_LOG(EOSWrapperSDKManager, Display, TEXT("EOSWrapperSubsystem EOS SDK INITIALIZED!"));

	EOS_Platform_SetApplicationStatus(PlatformHandle, CachedApplicationStatus);
	EOS_Platform_SetNetworkStatus(PlatformHandle, CachedNetworkStatus);
	EOS_Platform_Tick(PlatformHandle);

	bInitialized = true;
	return true;
}

void FWrapperSDKManager::Shutdown()
{
	if (!bInitialized) return;

	if (PlatformHandle != nullptr)
	{
		EOS_Platform_Release(PlatformHandle);
		PlatformHandle = nullptr;
	}

	bInitialized = false;
}

FString FWrapperSDKManager::GetProductVersion()
{
	return ProductVersion;
}

bool FWrapperSDKManager::Tick(float)
{
	if (PlatformHandle)
	{
		// LLM_SCOPE(ELLMTag::RealTimeCommunications); // TODO should really be ELLMTag::EOSSDK
		// QUICK_SCOPE_CYCLE_COUNTER(FEOSSDKManager_Tick);
		// CSV_SCOPED_TIMING_STAT_EXCLUSIVE(EOSSDK);
		EOS_Platform_Tick(PlatformHandle);
		return true;
	}

	return false;
}

void EOS_CALL FWrapperSDKManager::EOSLogMessageReceived(const EOS_LogMessage* Message)
{
#define EOSLOG(Level) UE_LOG(EOSWrapperSDKManager, Level, TEXT("%s: %s"), UTF8_TO_TCHAR(Message->Category), *MessageStr)

	FString MessageStr(UTF8_TO_TCHAR(Message->Message));
	MessageStr.TrimStartAndEndInline();

	switch (Message->Level)
	{
		case EOS_ELogLevel::EOS_LOG_Fatal: EOSLOG(Fatal);
			break;
		case EOS_ELogLevel::EOS_LOG_Error: EOSLOG(Error);
			break;
		case EOS_ELogLevel::EOS_LOG_Warning: EOSLOG(Warning);
			break;
		case EOS_ELogLevel::EOS_LOG_Info: EOSLOG(Log);
			break;
		case EOS_ELogLevel::EOS_LOG_Verbose: EOSLOG(Verbose);
			break;
		case EOS_ELogLevel::EOS_LOG_VeryVerbose: EOSLOG(VeryVerbose);
			break;
		case EOS_ELogLevel::EOS_LOG_Off:
		default:
			// do nothing
			break;
	}
#undef EOSLOG
}

#endif
