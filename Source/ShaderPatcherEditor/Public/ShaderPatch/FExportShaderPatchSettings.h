#pragma once 

#include "HotPatcherLog.h"
#include "CreatePatch/HotPatcherSettingBase.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherLog.h"
#include "ETargetPlatform.h"

// engine header
#include "Misc/FileHelper.h"
#include "CoreMinimal.h"

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetStringLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "FExportShaderPatchSettings.generated.h"

USTRUCT()
struct SHADERPATCHEREDITOR_API FShaderPatchPakConf
{
	GENERATED_USTRUCT_BODY()
	FShaderPatchPakConf()
	{
		FString AppName = FApp::GetProjectName();
		ConfigName = FString::Printf(TEXT("%s_Patch"),*AppName);
		MountPoint = FString::Printf(TEXT("../../../%s/ShaderLibs"),*AppName);
	}
	
	UPROPERTY(EditAnywhere)
	FString ConfigName;
	UPROPERTY(EditAnywhere)
	FString MountPoint;
};

USTRUCT()
struct SHADERPATCHEREDITOR_API FShaderPatchConf
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
		ETargetPlatform Platform = ETargetPlatform::None;
	UPROPERTY(EditAnywhere)
    	TArray<FDirectoryPath> OldMetadataDir;
    UPROPERTY(EditAnywhere)
    	FDirectoryPath NewMetadataDir;
    UPROPERTY(EditAnywhere)
   		bool bNativeFormat = false;
	// since UE 4.26 (Below 4.26 this parameter has no effect)
	UPROPERTY(EditAnywhere)
		bool bDeterministicShaderCodeOrder = true;
	UPROPERTY(EditAnywhere,meta=(EditCondition="!bNativeFormat"))
		bool bCreatePak = false;
	UPROPERTY(EditAnywhere,meta=(EditCondition="bCreatePak"))
		FShaderPatchPakConf PakConfig;
};

/** Singleton wrapper to allow for using the setting structure in SSettingsView */
USTRUCT()
struct SHADERPATCHEREDITOR_API FExportShaderPatchSettings:public FPatcherEntitySettingBase
{
	GENERATED_USTRUCT_BODY()
public:
	FExportShaderPatchSettings()
	{
		SavePath.Path = TEXT("[PROJECTDIR]/Saved/HotPatcher/ShaderPatcher");
	}
	virtual ~FExportShaderPatchSettings(){};

	FORCEINLINE static FExportShaderPatchSettings* Get()
	{
		static FExportShaderPatchSettings StaticIns;

		return &StaticIns;
	}
	
	UPROPERTY(EditAnywhere, Category="Version")
	FString VersionID;
	
	UPROPERTY(EditAnywhere, Category="Config")
	TArray<FShaderPatchConf> ShaderPatchConfigs;

	UPROPERTY(EditAnywhere, Category="SaveTo")
	bool bStorageConfig = true;
	
	UPROPERTY(EditAnywhere, Category="SaveTo")
	FDirectoryPath SavePath;

	FString GetSaveAbsPath();
	
	UPROPERTY(EditAnywhere, Category="Advanced")
	bool bStandaloneMode = false;

	FString GetCombinedAdditionalCommandletArgs()const {return TEXT("");}
};


