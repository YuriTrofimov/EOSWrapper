// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#include "CoreMinimal.h"
#include "EOSWrapperSubsystem.h"

/**
 *
 */
class FEOSWrapperModule : public IModuleInterface
{
private:
	/** Class responsible for creating instance(s) of the subsystem */
	class FOnlineFactoryEOSWrapper* EOSWrapperFactory;

public:
	FEOSWrapperModule() : EOSWrapperFactory(nullptr) {}

	virtual ~FEOSWrapperModule() {}

#if WITH_EDITOR
	void OnPostEngineInit();
	void OnPreExit();
#endif

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual bool SupportsDynamicReloading() override { return false; }

	virtual bool SupportsAutomaticShutdown() override { return false; }
};
