// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#if WITH_EOS_SDK

DECLARE_LOG_CATEGORY_EXTERN(EOSWrapperSDKManager, Log, All);

#include "CoreMinimal.h"
#include "eos_types.h"
#include "eos_logging.h"

/**
 *
 */
class EOSWRAPPER_API FWrapperSDKManager
{
public:
	FWrapperSDKManager();
	~FWrapperSDKManager();

	bool Initialize();
	void Shutdown();

	FString GetProductVersion();

	FORCEINLINE EOS_HPlatform GetPlatformHandle() const { return PlatformHandle; }

private:
	EOS_HPlatform PlatformHandle = nullptr;
	FString ProductVersion;

	void SetupTimer();
	void ClearTimer();
	bool Tick(float);
	void Tick();
	static void EOS_CALL EOSLogMessageReceived(const EOS_LogMessage* Message);

	/** Are we currently initialized */
	bool bInitialized = false;

	/** Handle to ticker delegate for Tick() */
	FTSTicker::FDelegateHandle TickerHandle;

	EOS_EApplicationStatus CachedApplicationStatus = EOS_EApplicationStatus::EOS_AS_Foreground;
	EOS_ENetworkStatus CachedNetworkStatus = EOS_ENetworkStatus::EOS_NS_Online;

	// Config
	/** Interval between platform ticks. 0 means we tick every frame. */
	double ConfigTickIntervalSeconds = 0.1f;
};

#endif
