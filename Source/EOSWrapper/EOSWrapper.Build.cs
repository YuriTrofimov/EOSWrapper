// Copyright:       Copyright (C) 2023 Yuri Trofimov
// Source Code:     https://github.com/YuriTrofimov/EOSWrapper

using System.IO;
using UnrealBuildTool;

public class EOSWrapper : ModuleRules
{
	public EOSWrapper(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "CoreOnline", "OnlineSubsystem", "OnlineSubsystemUtils", "OnlineBase",
			"Sockets"
		});
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDefinitions.Add("WITH_EOS_SDK=1");
		PublicDefinitions.Add("WITH_EOS_SDK=1");

		string EosPath = Path.Combine(ModuleDirectory, "EOS", "SDK");
		string EosIncludePath = Path.Combine(EosPath, "Include");

		PublicIncludePaths.AddRange(new[]
		{
			EosIncludePath
		});

		PublicDependencyModuleNames.AddRange(new[] { "Core" });

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string EosLibPath = Path.Combine(EosPath, "Lib");
			PublicAdditionalLibraries.Add(Path.Combine(EosLibPath, "EOSSDK-Win64-Shipping.lib"));

			// load on first call
			PublicDelayLoadDLLs.Add("EOSSDK-Win64-Shipping.dll");

			string EosDllbPath = Path.Combine(EosPath, "Bin");
			RuntimeDependencies.Add(Path.Combine(EosDllbPath, "EOSSDK-Win64-Shipping.dll"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string EosLibPath = Path.Combine(EosPath, "Bin");
			PublicAdditionalLibraries.Add(Path.Combine(EosLibPath, "libEOSSDK-Linux-Shipping.so"));

			// load on first call
			PublicDelayLoadDLLs.Add("libEOSSDK-Linux-Shipping.so");

			string EosDllbPath = Path.Combine(EosPath, "Bin");
			RuntimeDependencies.Add(Path.Combine(EosDllbPath, "libEOSSDK-Linux-Shipping.so"));
		}
	}
}