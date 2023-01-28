// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once
#include "EOSWrapperShared.h"
#include "Interfaces/OnlinePresenceInterface.h"

#if WITH_EOS_SDK

#include "CoreMinimal.h"
#include "eos_common.h"

#ifndef OSS_UNIQUEID_REDACT
#define OSS_UNIQUEID_REDACT(UniqueId, x) (x)
#endif

#define EOS_OSS_STRING_BUFFER_LENGTH 256 + 1  // 256 plus null terminator
#define EOSPLUS_ID_SEPARATOR TEXT("_+_")
#define EOS_ID_SEPARATOR TEXT("|")
#define ID_HALF_BYTE_SIZE 16
#define EOS_ID_BYTE_SIZE (ID_HALF_BYTE_SIZE * 2)

typedef TSharedPtr<const class FUniqueNetIdEOS> FUniqueNetIdEOSPtr;
typedef TSharedRef<const class FUniqueNetIdEOS> FUniqueNetIdEOSRef;

/**
 * Unique net id wrapper for a EOS account ids.
 */
class FUniqueNetIdEOS : public FUniqueNetId
{
public:
	static const FUniqueNetIdEOS& Cast(const FUniqueNetId& NetId);

	/** global static instance of invalid (zero) id */
	static const FUniqueNetIdEOSRef& EmptyId();

	static FName GetTypeStatic();

	virtual FName GetType() const override;
	virtual const uint8* GetBytes() const override;
	virtual int32 GetSize() const override;
	virtual bool IsValid() const override;
	virtual uint32 GetTypeHash() const override;
	virtual FString ToString() const override;
	virtual FString ToDebugString() const override;

	const EOS_EpicAccountId GetEpicAccountId() const { return EpicAccountId; }

	const EOS_ProductUserId GetProductUserId() const { return ProductUserId; }

private:
	EOS_EpicAccountId EpicAccountId = nullptr;
	EOS_ProductUserId ProductUserId = nullptr;
	uint8 RawBytes[EOS_ID_BYTE_SIZE] = {0};

	friend class FUniqueNetIdEOSRegistry;

	template <typename... TArgs>
	static FUniqueNetIdEOSRef Create(TArgs&&... Args)
	{
		return MakeShareable(new FUniqueNetIdEOS(Forward<TArgs>(Args)...));
	}

	FUniqueNetIdEOS() = default;

	explicit FUniqueNetIdEOS(const uint8* Bytes, int32 Size);
	explicit FUniqueNetIdEOS(EOS_EpicAccountId InEpicAccountId, EOS_ProductUserId InProductUserId);
};

class FUniqueNetIdEOSRegistry
{
public:
	static FUniqueNetIdEOSPtr FindOrAdd(const FString& NetIdStr) { return Get().FindOrAddImpl(NetIdStr); }
	static FUniqueNetIdEOSPtr FindOrAdd(const uint8* Bytes, int32 Size) { return Get().FindOrAddImpl(Bytes, Size); }
	static FUniqueNetIdEOSPtr FindOrAdd(EOS_EpicAccountId EpicAccountId, EOS_ProductUserId ProductUserId) { return Get().FindOrAddImpl(EpicAccountId, ProductUserId); }

private:
	FRWLock Lock;
	TMap<EOS_EpicAccountId, FUniqueNetIdEOSRef> EasToNetId;
	TMap<EOS_ProductUserId, FUniqueNetIdEOSRef> PuidToNetId;

	static FUniqueNetIdEOSRegistry& Get();

	FUniqueNetIdEOSPtr FindOrAddImpl(const FString& NetIdStr);
	FUniqueNetIdEOSPtr FindOrAddImpl(const uint8* Bytes, int32 Size);
	FUniqueNetIdEOSPtr FindOrAddImpl(EOS_EpicAccountId EpicAccountId, EOS_ProductUserId ProductUserId);
};

#ifndef AUTH_ATTR_REFRESH_TOKEN
#define AUTH_ATTR_REFRESH_TOKEN TEXT("refresh_token")
#endif
#ifndef AUTH_ATTR_ID_TOKEN
#define AUTH_ATTR_ID_TOKEN TEXT("id_token")
#endif
#ifndef AUTH_ATTR_AUTHORIZATION_CODE
#define AUTH_ATTR_AUTHORIZATION_CODE TEXT("authorization_code")
#endif

#define USER_ATTR_DISPLAY_NAME TEXT("display_name")
#define USER_ATTR_COUNTRY TEXT("country")
#define USER_ATTR_LANG TEXT("language")

/** Used to update all types of FOnlineUser classes, irrespective of leaf most class type */
class IAttributeAccessInterface
{
public:
	virtual void SetInternalAttribute(const FString& AttrName, const FString& AttrValue) {}

	virtual FUniqueNetIdEOSPtr GetUniqueNetIdEOS() const { return FUniqueNetIdEOSPtr(); }
};

typedef TSharedPtr<IAttributeAccessInterface> IAttributeAccessInterfacePtr;
typedef TSharedRef<IAttributeAccessInterface> IAttributeAccessInterfaceRef;

/**
 * Implementation of FOnlineUser that can be shared across multiple class hiearchies
 */
template <class BaseClass, class AttributeAccessClass>
class TOnlineUserEOS : public BaseClass, public AttributeAccessClass
{
public:
	TOnlineUserEOS(const FUniqueNetIdEOSRef& InNetIdRef) : UserIdRef(InNetIdRef) {}

	virtual ~TOnlineUserEOS() {}

	// FOnlineUser
	virtual FUniqueNetIdRef GetUserId() const override { return UserIdRef; }

	virtual FString GetRealName() const override { return FString(); }

	virtual FString GetDisplayName(const FString& Platform = FString()) const override
	{
		FString ReturnValue;
		GetUserAttribute(USER_ATTR_DISPLAY_NAME, ReturnValue);
		return ReturnValue;
	}

	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const override
	{
		const FString* FoundAttr = UserAttributes.Find(AttrName);
		if (FoundAttr != nullptr)
		{
			OutAttrValue = *FoundAttr;
			return true;
		}
		return false;
	}
	//~FOnlineUser

	virtual void SetInternalAttribute(const FString& AttrName, const FString& AttrValue) { UserAttributes.Add(AttrName, AttrValue); }

	virtual FUniqueNetIdEOSPtr GetUniqueNetIdEOS() const { return UserIdRef; }

protected:
	/** User Id represented as a FUniqueNetId */
	FUniqueNetIdEOSRef UserIdRef;
	/** Additional key/value pair data related to user attribution */
	TMap<FString, FString> UserAttributes;
};

/**
 * Implementation of FUserOnlineAccount methods that adds in the online user template to complete the interface
 */
template <class BaseClass>
class TUserOnlineAccountEOS : public TOnlineUserEOS<BaseClass, IAttributeAccessInterface>
{
public:
	TUserOnlineAccountEOS(const FUniqueNetIdEOSRef& InNetIdRef) : TOnlineUserEOS<BaseClass, IAttributeAccessInterface>(InNetIdRef) {}

	// FUserOnlineAccount
	virtual FString GetAccessToken() const override
	{
		FString Token;
		GetAuthAttribute(AUTH_ATTR_ID_TOKEN, Token);
		return Token;
	}

	virtual bool GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const override
	{
		const FString* FoundAttr = AdditionalAuthData.Find(AttrName);
		if (FoundAttr != nullptr)
		{
			OutAttrValue = *FoundAttr;
			return true;
		}
		return false;
	}

	virtual bool SetUserAttribute(const FString& AttrName, const FString& AttrValue) override
	{
		const FString* FoundAttr = this->UserAttributes.Find(AttrName);
		if (FoundAttr == nullptr || *FoundAttr != AttrValue)
		{
			this->UserAttributes.Add(AttrName, AttrValue);
			return true;
		}
		return false;
	}
	//~FUserOnlineAccount

	void SetAuthAttribute(const FString& AttrName, const FString& AttrValue) { AdditionalAuthData.Add(AttrName, AttrValue); }

protected:
	/** Additional key/value pair data related to auth */
	TMap<FString, FString> AdditionalAuthData;
};

typedef TSharedRef<FOnlineUserPresence> FOnlineUserPresenceRef;

#endif

/** Class to handle all callbacks generically using a lambda to process callback results */
template <typename CallbackFuncType, typename CallbackType, typename OwningType>
class TEOSCallback : public FCallbackBase
{
public:
	TFunction<void(const CallbackType*)> CallbackLambda;

	TEOSCallback(TWeakPtr<OwningType> InOwner) : FCallbackBase(), Owner(InOwner) {}
	TEOSCallback(TWeakPtr<const OwningType> InOwner) : FCallbackBase(), Owner(InOwner) {}
	virtual ~TEOSCallback() = default;

	CallbackFuncType GetCallbackPtr() { return &CallbackImpl; }

protected:
	/** The object that needs to be checked for lifetime before calling the callback */
	TWeakPtr<const OwningType> Owner;

private:
	static void EOS_CALL CallbackImpl(const CallbackType* Data)
	{
		if (EOS_EResult_IsOperationComplete(Data->ResultCode) == EOS_FALSE)
		{
			// Ignore
			return;
		}
		check(IsInGameThread());

		TEOSCallback* CallbackThis = (TEOSCallback*)Data->ClientData;
		check(CallbackThis);

		if (CallbackThis->Owner.IsValid())
		{
			check(CallbackThis->CallbackLambda);
			CallbackThis->CallbackLambda(Data);
		}
		delete CallbackThis;
	}
};