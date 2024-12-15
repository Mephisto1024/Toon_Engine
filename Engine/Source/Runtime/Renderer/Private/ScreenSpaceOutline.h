#pragma once
#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderParameterStruct.h"


FRDGTextureDesc GetToonOutlineTextureDesc(FIntPoint Extent, ETextureCreateFlags CreateFlags);
FRDGTextureRef CreateToonOutlineTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, ETextureCreateFlags CreateFlags);

class FScreenSpaceOutLinePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FScreenSpaceOutLinePS);
	SHADER_USE_PARAMETER_STRUCT(FScreenSpaceOutLinePS, FGlobalShader);

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceOptimization);
		OutEnvironment.SetDefine(TEXT("ScreenSpaceOutLine"), 1);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	   SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, NormalTexture)
	   SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ToonOutlineDataTexture)
	   SHADER_PARAMETER_SAMPLER(SamplerState, PointClampSampler)
	   RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};

class FOutlineCombinePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FOutlineCombinePS);
	SHADER_USE_PARAMETER_STRUCT(FOutlineCombinePS, FGlobalShader);

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceOptimization);
		OutEnvironment.SetDefine(TEXT("FOutlineCombinePS"), 1);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, BaseColorTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ToonOutlineTexture)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ToonOutlineDataTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, PointClampSampler)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};





