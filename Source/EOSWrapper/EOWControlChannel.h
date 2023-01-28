// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#pragma once

#include "CoreMinimal.h"
#include "Engine/ControlChannel.h"
#include "UObject/Object.h"
#include "EOWControlChannel.generated.h"

/**
 * Custom control channel to handle async user authentication
 */
UCLASS()
class EOSWRAPPER_API UEOWControlChannel : public UControlChannel
{
	GENERATED_BODY()

public:
	virtual void Init(UNetConnection* InConnection, int32 InChIndex, EChannelCreateFlags CreateFlags) override;
};
