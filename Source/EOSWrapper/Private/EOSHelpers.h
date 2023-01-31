// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#if WITH_EOS_SDK

#if defined(EOS_PLATFORM_BASE_FILE_NAME)
#include EOS_PLATFORM_BASE_FILE_NAME
#endif

#include "eos_auth_types.h"
#include "eos_types.h"
#include "Interfaces/OnlineExternalUIInterface.h"

class FEOSWrapperSubsystem;
using IEOSPlatformHandlePtr = TSharedPtr<class IEOSPlatformHandle, ESPMode::ThreadSafe>;

class FEOSHelpers
{
public:
	virtual ~FEOSHelpers() = default;

	virtual void PlatformAuthCredentials(EOS_Auth_Credentials& Credentials);
	virtual void PlatformTriggerLoginUI(FEOSWrapperSubsystem* EOSSubsystem, const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate);
	virtual IEOSPlatformHandlePtr CreatePlatform(EOS_Platform_Options& PlatformOptions);

protected:
	/** Shared LoginUI logic that can be used by platforms that support LoginUI */
	void ShowAccountPortalUI(FEOSWrapperSubsystem* InEOSSubsystem, const int ControllerIndex, const FOnLoginUIClosedDelegate& Delegate);

private:
	/** Completion handler for ShowAccountPortalUI */
	void OnAccountPortalLoginComplete(int ControllerIndex, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& ErrorString, FEOSWrapperSubsystem* InEOSSubsystem,
		const FOnLoginUIClosedDelegate LoginUIClosedDelegate, FDelegateHandle* LoginDelegateHandle) const;
};

#endif	// WITH_EOS_SDK