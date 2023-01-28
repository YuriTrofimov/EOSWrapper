// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#if WITH_EOS_SDK
#include "eos_types.h"
#include "eos_lobby_types.h"
#include "eos_auth_types.h"
#include "eos_connect_types.h"

#define EOS_OSS_STRING_BUFFER_LENGTH 256 + 1  // 256 plus null terminator

/**
 *
 */
class EOSWRAPPER_API FEOSWrapperLobby : public TSharedFromThis<FEOSWrapperLobby, ESPMode::ThreadSafe>
{
public:
	FEOSWrapperLobby();	 // = delete
	virtual ~FEOSWrapperLobby();

	void CreateLobby();

	void Tick();
	void ConnectLogin();

private:
	// EOS Lobbies
	EOS_HPlatform PlatformHandle = nullptr;
	/** Handle for Auth interface */
	EOS_HAuth AuthHandle;
	/** Handle for Connect interface */
	EOS_HConnect ConnectHandle;
	EOS_EpicAccountId CurrentUserID;
	EOS_ProductUserId ProductUserID;
	bool bLobbyCreated = false;
	static void EOS_CALL OnCreateLobbyFinished(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	static void EOS_CALL LoginCompleteCallbackFn(const EOS_Auth_LoginCallbackInfo* Data);
	static void EOS_CALL ConnectLoginCompleteCb(const EOS_Connect_LoginCallbackInfo* Data);
};

#endif
