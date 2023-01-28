// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#include "CoreMinimal.h"
#include "EOSWrapperSubsystem.h"
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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void DeveloperLogin(FString Host, FString Account);

	/* Проверка на то, что пользователь залогинился а текущей онлайн системе */
	UFUNCTION(BlueprintPure, Category = "Online|Game|Login")
	void IsUserLoggedIn(bool& LoggedIn, int32 LocalUserNum = 0) const;

private:
	/** Reference to the main EOS subsystem */
	FEOSWrapperSubsystem* EOSSubsystem;
};
