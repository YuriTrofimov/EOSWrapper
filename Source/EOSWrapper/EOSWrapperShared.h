// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#if WITH_EOS_SDK

#include "Containers/UnrealString.h"
#include "Logging/LogMacros.h"

#if defined(EOS_PLATFORM_BASE_FILE_NAME)
#include EOS_PLATFORM_BASE_FILE_NAME
#endif
#include "eos_base.h"
#include "eos_common.h"
#include "eos_version.h"

#if defined(DISABLE_EOSVOICECHAT_ENGINE)
#define WITH_EOS_RTC 0
#else
#define WITH_EOS_RTC WITH_EOS_SDK && (EOS_MAJOR_VERSION >= 1 && EOS_MINOR_VERSION >= 13)
#endif

#define EOS_ENUM_FORWARD_DECL(name) enum class name : int32_t;
EOS_ENUM_FORWARD_DECL(EOS_EApplicationStatus);
EOS_ENUM_FORWARD_DECL(EOS_EAuthScopeFlags);
EOS_ENUM_FORWARD_DECL(EOS_EAuthTokenType);
EOS_ENUM_FORWARD_DECL(EOS_EDesktopCrossplayStatus);
EOS_ENUM_FORWARD_DECL(EOS_EFriendsStatus);
EOS_ENUM_FORWARD_DECL(EOS_ELoginCredentialType);
EOS_ENUM_FORWARD_DECL(EOS_ENetworkStatus);
EOS_ENUM_FORWARD_DECL(EOS_Presence_EStatus);
#undef EOS_ENUM_FORWARD_DECL

DECLARE_LOG_CATEGORY_EXTERN(LogEOSSDK, Log, All);

EOSWRAPPER_API FString LexToString(const EOS_EResult EosResult);
EOSWRAPPER_API FString LexToString(const EOS_ProductUserId UserId);
EOSWRAPPER_API void LexFromString(EOS_ProductUserId& UserId, const TCHAR* String);
inline EOS_ProductUserId EOSProductUserIdFromString(const TCHAR* String)
{
	EOS_ProductUserId UserId;
	LexFromString(UserId, String);
	return UserId;
}

EOSWRAPPER_API FString LexToString(const EOS_EpicAccountId AccountId);

EOSWRAPPER_API const TCHAR* LexToString(const EOS_EApplicationStatus ApplicationStatus);
EOSWRAPPER_API const TCHAR* LexToString(const EOS_EAuthTokenType AuthTokenType);
EOSWRAPPER_API const TCHAR* LexToString(const EOS_EDesktopCrossplayStatus DesktopCrossplayStatus);
EOSWRAPPER_API const TCHAR* LexToString(const EOS_EExternalAccountType ExternalAccountType);
EOSWRAPPER_API const TCHAR* LexToString(const EOS_EFriendsStatus FriendStatus);
EOSWRAPPER_API const TCHAR* LexToString(const EOS_ELoginStatus LoginStatus);
EOSWRAPPER_API const TCHAR* LexToString(const EOS_ENetworkStatus NetworkStatus);
EOSWRAPPER_API const TCHAR* LexToString(const EOS_Presence_EStatus PresenceStatus);

EOSWRAPPER_API bool LexFromString(EOS_EAuthScopeFlags& OutEnum, const TCHAR* InString);
EOSWRAPPER_API bool LexFromString(EOS_EExternalCredentialType& OutEnum, const TCHAR* InString);
EOSWRAPPER_API bool LexFromString(EOS_ELoginCredentialType& OutEnum, const TCHAR* InString);

/** Used to store a pointer to the EOS callback object without knowing type */
class EOSWRAPPER_API FCallbackBase
{
public:
	virtual ~FCallbackBase() {}
};

/** Class to handle all callbacks generically using a lambda to process callback results */
template <typename CallbackFuncType, typename CallbackParamType, typename OwningType, typename CallbackReturnType = void, typename... CallbackExtraParams>
class TEOSGlobalCallback : public FCallbackBase
{
public:
	TFunction<CallbackReturnType(const CallbackParamType*, CallbackExtraParams... ExtraParams)> CallbackLambda;

	TEOSGlobalCallback(TWeakPtr<OwningType> InOwner) : FCallbackBase(), Owner(InOwner) {}
	virtual ~TEOSGlobalCallback() = default;

	CallbackFuncType GetCallbackPtr() { return &CallbackImpl; }

	/** Is this callback intended for the game thread */
	bool bIsGameThreadCallback = true;

private:
	/** The object that needs to be checked for lifetime before calling the callback */
	TWeakPtr<OwningType> Owner;

	static CallbackReturnType EOS_CALL CallbackImpl(const CallbackParamType* Data, CallbackExtraParams... ExtraParams)
	{
		TEOSGlobalCallback* CallbackThis = (TEOSGlobalCallback*)Data->ClientData;
		check(CallbackThis);

		if (CallbackThis->bIsGameThreadCallback)
		{
			check(IsInGameThread());
		}

		if (CallbackThis->Owner.IsValid())
		{
			check(CallbackThis->CallbackLambda);

			if constexpr (std::is_void<CallbackReturnType>::value)
			{
				CallbackThis->CallbackLambda(Data, ExtraParams...);
			}
			else
			{
				return CallbackThis->CallbackLambda(Data, ExtraParams...);
			}
		}

		if constexpr (!std::is_void<CallbackReturnType>::value)
		{
			// we need to return _something_ to compile.
			return CallbackReturnType{};
		}
	}
};

#endif	// WITH_EOS_SDK