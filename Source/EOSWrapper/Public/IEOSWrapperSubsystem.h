// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#include "OnlineSubsystemImpl.h"

class FUniqueNetId;
class IVoiceChatUser;
using IEOSPlatformHandlePtr = TSharedPtr<class IEOSPlatformHandle, ESPMode::ThreadSafe>;

/**
 *	OnlineSubsystemEOS - Implementation of the online subsystem for EOS services
 */
class EOSWRAPPER_API IEOSWrapperSubsystem : public FOnlineSubsystemImpl
{
public:
	IEOSWrapperSubsystem(FName InSubsystemName, FName InInstanceName) : FOnlineSubsystemImpl(InSubsystemName, InInstanceName) {}
	virtual ~IEOSWrapperSubsystem() = default;

	virtual IVoiceChatUser* GetVoiceChatUserInterface(const FUniqueNetId& LocalUserId) = 0;
	virtual IEOSPlatformHandlePtr GetEOSPlatformHandle() const = 0;
};