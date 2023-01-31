// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EOWGameInstanceSubsystem.generated.h"

/**
 *
 */
UCLASS()
class EOSWRAPPER_API UEOWGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTokenValidationComplete, bool, bValid, const FString&, Token);

	UPROPERTY(BlueprintAssignable)
	FOnTokenValidationComplete OnTokenValidationComplete;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void GetUserToken(int32 LocalUserNum, FString& Token, FString& UserAccountString);
	void ValidateUserAuthToken(const FString& Token, const FString& AccountIDString) const;

protected:
	class FEOSWrapperSubsystem* OnlineSubsystem;
};
