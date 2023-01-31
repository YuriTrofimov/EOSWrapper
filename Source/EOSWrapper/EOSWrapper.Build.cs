// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

using System;
using System.IO;
using UnrealBuildTool;

public class EOSWrapper : ModuleRules
{
	public EOSWrapper(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDefinitions.Add("WITH_EOS_SDK=1");
		//PublicDefinitions.Add("WITH_EOS_RTC=0");

		PublicDependencyModuleNames.AddRange(new string[] { "OnlineSubsystemUtils" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreOnline", "CoreUObject", "Engine", "EOSSDK", "EOSShared", "Json", "OnlineBase", "OnlineSubsystem", "Sockets", "NetCore" });

		PrivateDefinitions.Add("USE_XBL_XSTS_TOKEN=" + (bUseXblXstsToken ? "1" : "0"));
		PrivateDefinitions.Add("USE_PSN_ID_TOKEN=" + (bUsePsnIdToken ? "1" : "0"));
		PrivateDefinitions.Add("ADD_USER_LOGIN_INFO=" + (bAddUserLoginInfo ? "1" : "0"));
	}

	protected virtual bool bUseXblXstsToken
	{
		get {
			return false;
		}
	}

	protected virtual bool bUsePsnIdToken
	{
		get {
			return false;
		}
	}

	protected virtual bool bAddUserLoginInfo
	{
		get {
			return false;
		}
	}
}