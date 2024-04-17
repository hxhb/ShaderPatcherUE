// Fill out your copyright notice in the Description page of Project Settings.
#include "ShaderPatch/ShaderPatchProxy.h"
#include "FlibHotPatcherCoreHelper.h"
#include "ShaderLibUtils/FlibShaderCodeLibraryHelper.h"
#include "ShaderPatch/FlibShaderPatchHelper.h"
#include "ShaderPatcherEditor.h"
#include "CreatePatch/PatcherProxy.h"

#define LOCTEXT_NAMESPACE "HotPatcherShaderPatchProxy"

bool UShaderPatchProxy::DoExport()
{
	bool bStatus = false;
	for(const auto& PlatformConfig:GetSettingObject()->ShaderPatchConfigs)
	{
		UE_LOG(LogShaderPatcherEditor,Display,TEXT("Generating Shader Patch for %s"),*THotPatcherTemplateHelper::GetEnumNameByValue(PlatformConfig.Platform));
		
		FString SaveToPath = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),GetSettingObject()->VersionID,THotPatcherTemplateHelper::GetEnumNameByValue(PlatformConfig.Platform));
		bool bCreateStatus = UFlibShaderPatchHelper::CreateShaderCodePatch(
        UFlibShaderPatchHelper::ConvDirectoryPathToStr(PlatformConfig.OldMetadataDir),
        FPaths::ConvertRelativePathToFull(PlatformConfig.NewMetadataDir.Path),
        SaveToPath,
        PlatformConfig.bNativeFormat,
        PlatformConfig.bDeterministicShaderCodeOrder
        );

		auto GetShaderPatchFormatLambda = [](const FString& ShaderPatchDir)->TMap<FName, TSet<FString>>
		{
			TMap<FName, TSet<FString>> FormatLibraryMap;
			TArray<FString> LibraryFiles;
			IFileManager::Get().FindFiles(LibraryFiles, *(ShaderPatchDir), *UFlibShaderCodeLibraryHelper::ShaderExtension);
	
			for (FString const& Path : LibraryFiles)
			{
				FString Name = FPaths::GetBaseFilename(Path);
				if (Name.RemoveFromStart(TEXT("ShaderArchive-")))
				{
					TArray<FString> Components;
					if (Name.ParseIntoArray(Components, TEXT("-")) == 2)
					{
						FName Format(*Components[1]);
						TSet<FString>& Libraries = FormatLibraryMap.FindOrAdd(Format);
						Libraries.Add(Components[0]);
					}
				}
			}
			return FormatLibraryMap;
		};
		
		if(bCreateStatus)
		{
			TMap<FName,TSet<FString>> ShaderFormatLibraryMap  = GetShaderPatchFormatLambda(SaveToPath);
			FText Msg = LOCTEXT("GeneratedShaderPatch", "Successd to Generated the Shader Patch.");
			TArray<FName> FormatNames;
			ShaderFormatLibraryMap.GetKeys(FormatNames);

			TArray<FString> ShaderLibPaths;
			for(const auto& FormatName:FormatNames)
			{
				TArray<FString> LibraryNames= ShaderFormatLibraryMap[FormatName].Array();
				for(const auto& LibrartName:LibraryNames)
				{
					FString OutputFilePath = UFlibShaderCodeLibraryHelper::GetCodeArchiveFilename(SaveToPath, LibrartName, FormatName);
					if(FPaths::FileExists(OutputFilePath))
					{
						ShaderLibPaths.Add(OutputFilePath);
						bStatus = true;
						if(!IsRunningCommandlet())
						{
							FHotPatcherDelegates::Get().GetNotifyFileGenerated().Broadcast(Msg, OutputFilePath);
						}
						else
						{
							UE_LOG(LogShaderPatcherEditor,Display,TEXT("%s"),*Msg.ToString());
						}
					}
					else
					{
						UE_LOG(LogShaderPatcherEditor,Display,TEXT("ERROR: %s not found!"),*OutputFilePath);
					}

					
				}
			}
			if(PlatformConfig.bCreatePak)
			{
				CreatePak(ShaderLibPaths,PlatformConfig);
			}
		}
	}

	if(GetSettingObject()->bStorageConfig)
	{
		FString SerializedJsonStr;
		THotPatcherTemplateHelper::TSerializeStructAsJsonString(*GetSettingObject(),SerializedJsonStr);

		FString SaveToPath = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),GetSettingObject()->VersionID,GetSettingObject()->VersionID).Append(TEXT(".json"));
		
		if (FFileHelper::SaveStringToFile(SerializedJsonStr, *SaveToPath))
		{
			FText Msg = LOCTEXT("SavedShaderPatchConfigMas", "Successd to Export the Shader Patch Config.");
			if(!IsRunningCommandlet())
			{
				FHotPatcherDelegates::Get().GetNotifyFileGenerated().Broadcast(Msg, SaveToPath);
			}
			else
			{
				UE_LOG(LogShaderPatcherEditor,Display,TEXT("%s"),*Msg.ToString());
			}
		}
	}
	return bStatus;
}

bool UShaderPatchProxy::CreatePak(const TArray<FString>& Files, const FShaderPatchConf& Conf)
{
	TSharedPtr<FExportPatchSettings> PatchSetting = MakeShareable(new FExportPatchSettings);
	PatchSetting->VersionId = Conf.PakConfig.ConfigName;
	PatchSetting->PakTargetPlatforms.Add(Conf.Platform);
	FPlatformExternAssets PakExternAssets;
	PakExternAssets.TargetPlatform = Conf.Platform;

	for(const auto& File:Files)
	{
		FExternFileInfo ExternFileInfo;
		FString Filename;
		{
			FString BaseFilename = FPaths::GetBaseFilename(File,true);
			FString Extension = FPaths::GetExtension(File,true);
			Filename = FString::Printf(TEXT("%s%s"),*BaseFilename,*Extension);
		}
		ExternFileInfo.SetFilePath(File);
		ExternFileInfo.MountPath = FString::Printf(TEXT("%s/%s"),*Conf.PakConfig.MountPoint,*Filename);
		PakExternAssets.AddExternFileToPak.Add(ExternFileInfo);
	}
	
	PatchSetting->DefaultPakListOptions.Empty();
	PatchSetting->DefaultPakListOptions.Empty();
	PatchSetting->PakPathRegular = TEXT("");
	PatchSetting->AddExternAssetsToPlatform.Add(PakExternAssets);
	PatchSetting->GetStorageOptions().bNewRelease = false;
	PatchSetting->GetStorageOptions().bDiffAnalysisResults = false;
	PatchSetting->bStandaloneMode = false;
	PatchSetting->bStorageConfig = true;
	PatchSetting->GetStorageOptions().bGeneratePakInfo = false;
	PatchSetting->GetStorageOptions().bResponseFile = false;
	PatchSetting->SavePath.Path = FPaths::Combine(GetSettingObject()->GetSaveAbsPath(),Conf.PakConfig.ConfigName);
	
	UPatcherProxy* PatcherProxy = NewObject<UPatcherProxy>();
	PatcherProxy->AddToRoot();
	PatcherProxy->Init(PatchSetting.Get());
	bool bExitStatus = PatcherProxy->DoExport();
	PatcherProxy->RemoveFromRoot();

	return bExitStatus;
}

#undef LOCTEXT_NAMESPACE