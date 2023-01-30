// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#if WITH_EOS_SDK

#include "EOSHelpers.h"

class FAndroidEOSHelpers : public FEOSHelpers
{
public:
	virtual void PlatformTriggerLoginUI(FEOSWrapperSubsystem* EOSSubsystem, const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate) override;
};

using FPlatformEOSHelpers = FAndroidEOSHelpers;

#endif