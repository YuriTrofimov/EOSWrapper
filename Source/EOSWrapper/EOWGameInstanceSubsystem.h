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
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTokenValidationComplete, bool, bValid, const FString&, Token);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyCreatedHandler, const FName&, LobbySessionName);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyUpdatedHandler, const FName&, LobbySessionName);

	UPROPERTY(BlueprintAssignable)
	FOnTokenValidationComplete OnTokenValidationComplete;

	UPROPERTY(BlueprintAssignable)
	FOnLobbyCreatedHandler OnLobbyCreated;
	
	UPROPERTY(BlueprintAssignable)
	FOnLobbyUpdatedHandler OnLobbyUpdated;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void DeveloperLogin(FString Host, FString Account);

	/* Проверка на то, что пользователь залогинился а текущей онлайн системе */
	UFUNCTION(BlueprintPure, Category = "Online|Game|Login")
	void IsUserLoggedIn(bool& LoggedIn, int32 LocalUserNum = 0) const;

	UFUNCTION(BlueprintPure, Category = "Online|Game|Login")
	void GetUserToken(int32 LocalUserNum, FString& Token, FString& UserAccountString);

	UFUNCTION(BlueprintCallable, Category = "Online|Game|Lobby")
	void CreateLobby(int32 LocalUserNum, FName LobbyName);

	UFUNCTION(BlueprintCallable, Category = "Online|Game|Lobby")
	void SetLobbyAttributeString(FName LobbyName, FName Key, FString Value);

	void ValidateUserAuthToken(const FString& Token, const FString& AccountIDString) const;
protected:
	class FEOSWrapperSubsystem* OnlineSubsystem;

private:
	TSharedPtr<class FOnlineSessionSettings> LastSessionSettings;
	void OnLobbyUpdatedCallback(const FName& LobbySessionName);
};
