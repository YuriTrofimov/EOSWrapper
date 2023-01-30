// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#if WITH_EOS_SDK

#include "EOSHelpers.h"

class FWindowsEOSHelpers : public FEOSHelpers
{
public:
	virtual ~FWindowsEOSHelpers() = default;

	virtual IEOSPlatformHandlePtr CreatePlatform(EOS_Platform_Options& PlatformOptions) override;
};

using FPlatformEOSHelpers = FWindowsEOSHelpers;

#endif // WITH_EOS_SDK