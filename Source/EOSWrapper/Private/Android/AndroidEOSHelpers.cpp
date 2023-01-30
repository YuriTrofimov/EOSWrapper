// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "AndroidEOSHelpers.h"

#if WITH_EOS_SDK

void FAndroidEOSHelpers::PlatformTriggerLoginUI(FEOSWrapperSubsystem* InEOSSubsystem, const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate)
{
	ShowAccountPortalUI(InEOSSubsystem, ControllerIndex, Delegate);
}

#endif
