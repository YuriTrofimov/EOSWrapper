﻿// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

#include "EOSWrapperSettings.h"

#include "OnlineSubsystem.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "EOSWrapper"
#define INI_SECTION TEXT("/Script/EOSWrapper.EOSWrapperSettings")

inline bool IsAnsi(const FString& Source)
{
	for (const TCHAR& IterChar : Source)
	{
		if (!FChar::IsPrint(IterChar))
		{
			return false;
		}
	}
	return true;
}

inline bool IsHex(const FString& Source)
{
	for (const TCHAR& IterChar : Source)
	{
		if (!FChar::IsHexDigit(IterChar))
		{
			return false;
		}
	}
	return true;
}

inline bool ContainsWhitespace(const FString& Source)
{
	for (const TCHAR& IterChar : Source)
	{
		if (FChar::IsWhitespace(IterChar))
		{
			return true;
		}
	}
	return false;
}

inline FString StripQuotes(const FString& Source)
{
	if (Source.StartsWith(TEXT("\"")))
	{
		return Source.Mid(1, Source.Len() - 2);
	}
	return Source;
}

void FEOSArtifactSettings::ParseRawArrayEntry(const FString& RawLine)
{
	const TCHAR* Delims[4] = {TEXT("("), TEXT(")"), TEXT("="), TEXT(",")};
	TArray<FString> Values;
	RawLine.ParseIntoArray(Values, Delims, 4, false);
	for (int32 ValueIndex = 0; ValueIndex < Values.Num(); ValueIndex++)
	{
		if (Values[ValueIndex].IsEmpty())
		{
			continue;
		}

		// Parse which struct field
		if (Values[ValueIndex] == TEXT("ArtifactName"))
		{
			ArtifactName = StripQuotes(Values[ValueIndex + 1]);
		}
		else if (Values[ValueIndex] == TEXT("ClientId"))
		{
			ClientId = StripQuotes(Values[ValueIndex + 1]);
		}
		else if (Values[ValueIndex] == TEXT("ClientSecret"))
		{
			ClientSecret = StripQuotes(Values[ValueIndex + 1]);
		}
		else if (Values[ValueIndex] == TEXT("ProductId"))
		{
			ProductId = StripQuotes(Values[ValueIndex + 1]);
		}
		else if (Values[ValueIndex] == TEXT("SandboxId"))
		{
			SandboxId = StripQuotes(Values[ValueIndex + 1]);
		}
		else if (Values[ValueIndex] == TEXT("DeploymentId"))
		{
			DeploymentId = StripQuotes(Values[ValueIndex + 1]);
		}
		else if (Values[ValueIndex] == TEXT("EncryptionKey"))
		{
			EncryptionKey = StripQuotes(Values[ValueIndex + 1]);
		}
		ValueIndex++;
	}
}

FEOSArtifactSettings FEOSWrapperArtifactSettings::ToNative() const
{
	FEOSArtifactSettings Native;

	Native.ArtifactName = ArtifactName;
	Native.ClientId = ClientId;
	Native.ClientSecret = ClientSecret;
	Native.DeploymentId = DeploymentId;
	Native.EncryptionKey = EncryptionKey;
	Native.ProductId = ProductId;
	Native.SandboxId = SandboxId;

	return Native;
}

FEOSWrapperSettings UEOSWrapperSettings::GetSettings()
{
	if (UObjectInitialized())
	{
		return AutoGetSettings();
	}

	return ManualGetSettings();
}

FEOSWrapperSettings UEOSWrapperSettings::AutoGetSettings()
{
	return GetDefault<UEOSWrapperSettings>()->ToNative();
}

const FEOSWrapperSettings& UEOSWrapperSettings::ManualGetSettings()
{
	static TOptional<FEOSWrapperSettings> CachedSettings;

	if (!CachedSettings.IsSet())
	{
		CachedSettings.Emplace();

		GConfig->GetString(INI_SECTION, TEXT("CacheDir"), CachedSettings->CacheDir, GEngineIni);
		GConfig->GetString(INI_SECTION, TEXT("DefaultArtifactName"), CachedSettings->DefaultArtifactName, GEngineIni);
		GConfig->GetInt(INI_SECTION, TEXT("TickBudgetInMilliseconds"), CachedSettings->TickBudgetInMilliseconds, GEngineIni);
		GConfig->GetInt(INI_SECTION, TEXT("TitleStorageReadChunkLength"), CachedSettings->TitleStorageReadChunkLength, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bEnableOverlay"), CachedSettings->bEnableOverlay, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bEnableSocialOverlay"), CachedSettings->bEnableSocialOverlay, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bEnableEditorOverlay"), CachedSettings->bEnableEditorOverlay, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bShouldEnforceBeingLaunchedByEGS"), CachedSettings->bShouldEnforceBeingLaunchedByEGS, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bUseEAS"), CachedSettings->bUseEAS, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bUseEOSConnect"), CachedSettings->bUseEOSConnect, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bUseEOSSessions"), CachedSettings->bUseEOSSessions, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bMirrorStatsToEOS"), CachedSettings->bMirrorStatsToEOS, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bMirrorAchievementsToEOS"), CachedSettings->bMirrorAchievementsToEOS, GEngineIni);
		GConfig->GetBool(INI_SECTION, TEXT("bMirrorPresenceToEAS"), CachedSettings->bMirrorPresenceToEAS, GEngineIni);
		// Artifacts explicitly skipped
		GConfig->GetArray(INI_SECTION, TEXT("TitleStorageTags"), CachedSettings->TitleStorageTags, GEngineIni);
	}

	return *CachedSettings;
}

FEOSWrapperSettings UEOSWrapperSettings::ToNative() const
{
	FEOSWrapperSettings Native;

	Native.CacheDir = CacheDir;
	Native.DefaultArtifactName = DefaultArtifactName;
	Native.TickBudgetInMilliseconds = TickBudgetInMilliseconds;
	Native.TitleStorageReadChunkLength = TitleStorageReadChunkLength;
	Native.bEnableOverlay = bEnableOverlay;
	Native.bEnableSocialOverlay = bEnableSocialOverlay;
	Native.bEnableEditorOverlay = bEnableEditorOverlay;
	Native.bShouldEnforceBeingLaunchedByEGS = bShouldEnforceBeingLaunchedByEGS;
	Native.bUseEAS = bUseEAS;
	Native.bUseEOSConnect = bUseEOSConnect;
	Native.bUseEOSSessions = bUseEOSSessions;
	Native.bMirrorStatsToEOS = bMirrorStatsToEOS;
	Native.bMirrorAchievementsToEOS = bMirrorAchievementsToEOS;
	Native.bMirrorPresenceToEAS = bMirrorPresenceToEAS;
	Algo::Transform(Artifacts, Native.Artifacts, &FEOSWrapperArtifactSettings::ToNative);
	Native.TitleStorageTags = TitleStorageTags;
	Native.ServerArtifacts = ServerArtifacts.ToNative();
	return Native;
}

bool UEOSWrapperSettings::GetSettingsForArtifact(const FString& ArtifactName, FEOSArtifactSettings& OutSettings)
{
	if (UObjectInitialized())
	{
		return AutoGetSettingsForArtifact(ArtifactName, OutSettings);
	}
	return ManualGetSettingsForArtifact(ArtifactName, OutSettings);
}

bool UEOSWrapperSettings::ManualGetSettingsForArtifact(const FString& ArtifactName, FEOSArtifactSettings& OutSettings)
{
	static TOptional<FString> CachedDefaultArtifactName;
	static TOptional<TArray<FEOSArtifactSettings>> CachedArtifactSettings;

	if (!CachedDefaultArtifactName.IsSet())
	{
		CachedDefaultArtifactName.Emplace();

		GConfig->GetString(INI_SECTION, TEXT("DefaultArtifactName"), *CachedDefaultArtifactName, GEngineIni);
	}

	if (!CachedArtifactSettings.IsSet())
	{
		CachedArtifactSettings.Emplace();

		TArray<FString> Artifacts;
		GConfig->GetArray(INI_SECTION, TEXT("Artifacts"), Artifacts, GEngineIni);
		for (const FString& Line : Artifacts)
		{
			FEOSArtifactSettings Artifact;
			Artifact.ParseRawArrayEntry(Line);
			CachedArtifactSettings->Add(Artifact);
		}
	}

	FString ArtifactNameOverride;
	// Figure out which config object we are loading
	FParse::Value(FCommandLine::Get(), TEXT("EOSArtifactNameOverride="), ArtifactNameOverride);
	if (ArtifactNameOverride.IsEmpty())
	{
		ArtifactNameOverride = ArtifactName;
	}

	// Search by name and then default if not found
	for (const FEOSArtifactSettings& Artifact : *CachedArtifactSettings)
	{
		if (Artifact.ArtifactName == ArtifactNameOverride)
		{
			OutSettings = Artifact;
			return true;
		}
	}

	for (const FEOSArtifactSettings& Artifact : *CachedArtifactSettings)
	{
		if (Artifact.ArtifactName == *CachedDefaultArtifactName)
		{
			OutSettings = Artifact;
			return true;
		}
	}

	return false;
}

bool UEOSWrapperSettings::AutoGetSettingsForArtifact(const FString& ArtifactName, FEOSArtifactSettings& OutSettings)
{
	const UEOSWrapperSettings* This = GetDefault<UEOSWrapperSettings>();
	FString ArtifactNameOverride;
	// Figure out which config object we are loading
	FParse::Value(FCommandLine::Get(), TEXT("EOSArtifactNameOverride="), ArtifactNameOverride);
	if (ArtifactNameOverride.IsEmpty())
	{
		ArtifactNameOverride = ArtifactName;
	}
	for (const FEOSWrapperArtifactSettings& Artifact : This->Artifacts)
	{
		if (Artifact.ArtifactName == ArtifactNameOverride)
		{
			OutSettings = Artifact.ToNative();
			return true;
		}
	}
	for (const FEOSWrapperArtifactSettings& Artifact : This->Artifacts)
	{
		if (Artifact.ArtifactName == This->DefaultArtifactName)
		{
			OutSettings = Artifact.ToNative();
			return true;
		}
	}
	UE_LOG_ONLINE(Error, TEXT("UEOSSettings::AutoGetSettingsForArtifact() failed due to missing config object specified. Check your project settings"));
	return false;
}

#if WITH_EDITOR
void UEOSWrapperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property == nullptr)
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
		return;
	}

	// Turning off the overlay in general turns off the social overlay too
	if (PropertyChangedEvent.Property->GetFName() == FName(TEXT("bEnableOverlay")))
	{
		if (!bEnableOverlay)
		{
			bEnableSocialOverlay = false;
			bEnableEditorOverlay = false;
		}
	}

	// Turning on the social overlay requires the base overlay too
	if (PropertyChangedEvent.Property->GetFName() == FName(TEXT("bEnableSocialOverlay")))
	{
		if (bEnableSocialOverlay)
		{
			bEnableOverlay = true;
		}
	}

	if (PropertyChangedEvent.MemberProperty != nullptr && PropertyChangedEvent.MemberProperty->GetFName() == FName(TEXT("Artifacts")) &&
		(PropertyChangedEvent.ChangeType & EPropertyChangeType::ValueSet))
	{
		// Loop through all entries validating them
		for (FEOSWrapperArtifactSettings& Artifact : Artifacts)
		{
			if (!Artifact.ClientId.IsEmpty())
			{
				if (!Artifact.ClientId.StartsWith(TEXT("xyz")))
				{
					FMessageDialog::Open(
						EAppMsgType::Ok, LOCTEXT("ClientIdInvalidMsg", "Client ids created after SDK version 1.5 start with xyz. Double check that you did not use your BPT Client Id instead."));
				}
				if (!IsAnsi(Artifact.ClientId) || ContainsWhitespace(Artifact.ClientId))
				{
					FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ClientIdNotAnsiMsg", "Client ids must contain ANSI printable characters only with no whitespace"));
					Artifact.ClientId.Empty();
				}
			}

			if (!Artifact.ClientSecret.IsEmpty())
			{
				if (!IsAnsi(Artifact.ClientSecret) || ContainsWhitespace(Artifact.ClientSecret))
				{
					FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ClientSecretNotAnsiMsg", "ClientSecret must contain ANSI printable characters only with no whitespace"));
					Artifact.ClientSecret.Empty();
				}
			}

			if (!Artifact.EncryptionKey.IsEmpty())
			{
				if (!IsHex(Artifact.EncryptionKey) || Artifact.EncryptionKey.Len() != 64)
				{
					FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("EncryptionKeyNotHexMsg", "EncryptionKey must contain 64 hex characters"));
					Artifact.EncryptionKey.Empty();
				}
			}
		}
	}

	// Turning off EAS disables presence mirroring too
	if (PropertyChangedEvent.Property->GetFName() == FName(TEXT("bUseEAS")))
	{
		if (!bUseEAS)
		{
			bMirrorPresenceToEAS = false;
		}
	}

	// Turning on presence requires EAS
	if (PropertyChangedEvent.Property->GetFName() == FName(TEXT("bMirrorPresenceToEAS")))
	{
		if (bMirrorPresenceToEAS)
		{
			bUseEAS = true;
		}
	}

	// Turning off EAS disables presence mirroring too
	if (PropertyChangedEvent.Property->GetFName() == FName(TEXT("bUseEOSConnect")))
	{
		if (!bUseEOSConnect)
		{
			bMirrorAchievementsToEOS = false;
			bMirrorStatsToEOS = false;
			bUseEOSSessions = false;
		}
	}

	// These all require EOS turned on if they are on
	if (PropertyChangedEvent.Property->GetFName() == FName(TEXT("bMirrorAchievementsToEOS")) || PropertyChangedEvent.Property->GetFName() == FName(TEXT("bMirrorStatsToEOS")) ||
		PropertyChangedEvent.Property->GetFName() == FName(TEXT("bUseEOSSessions")))
	{
		if (bMirrorAchievementsToEOS || bMirrorStatsToEOS || bUseEOSSessions)
		{
			bUseEOSConnect = true;
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif

#undef LOCTEXT_NAMESPACE
