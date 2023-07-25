#include "ShaderPatch/FExportShaderPatchSettings.h"

FString FExportShaderPatchSettings::GetSaveAbsPath()
{
	return FPaths::Combine(UFlibPatchParserHelper::ReplaceMark(SavePath.Path));
}