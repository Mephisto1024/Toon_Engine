#pragma once
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "MeshPassProcessor.h"

class FOutlineVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FOutlineVS,MeshMaterial);
protected:

	FOutlineVS() {}

	FOutlineVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		OutlineSize.Bind(Initializer.ParameterMap , TEXT("OutlineSize"));
	}

public:
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{}
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return Parameters.MaterialParameters.bOutlined;
	}
	

	void GetShaderBindings(
		const FScene* Scene,
		ERHIFeatureLevel::Type FeatureLevel,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		const FMeshPassProcessorRenderState& DrawRenderState,
		const FMeshMaterialShaderElementData& ShaderElementData,
		FMeshDrawSingleShaderBindings& ShaderBindings) const
	{
		FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material, DrawRenderState, ShaderElementData, ShaderBindings);
		ShaderBindings.Add(OutlineSize , Material.GetOutlineSize());
	}
private:
	LAYOUT_FIELD(FShaderParameter , OutlineSize);
};

class FOutlinePS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FOutlinePS,MeshMaterial);
public:
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{}
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return Parameters.MaterialParameters.bOutlined;
	}

	FOutlinePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		OutlineColor.Bind(Initializer.ParameterMap , TEXT("OutlineColor"));
	}
	
	FOutlinePS() {}

	void GetShaderBindings(
		const FScene* Scene,
		ERHIFeatureLevel::Type FeatureLevel,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		const FMeshPassProcessorRenderState& DrawRenderState,
		const FMeshMaterialShaderElementData& ShaderElementData,
		FMeshDrawSingleShaderBindings& ShaderBindings) const
	{
		FMeshMaterialShader::GetShaderBindings(Scene, FeatureLevel, PrimitiveSceneProxy, MaterialRenderProxy, Material, DrawRenderState, ShaderElementData, ShaderBindings);
		FLinearColor MaterialOutlineColor = Material.GetOutlineColor();
		FVector3f ShaderOutlineColor = FVector3f(MaterialOutlineColor.R,MaterialOutlineColor.G,MaterialOutlineColor.B);
		ShaderBindings.Add(OutlineColor,ShaderOutlineColor);
	}
	LAYOUT_FIELD(FShaderParameter , OutlineColor);
};

/*----------------------------------------------------*/

class FOutlineShaderElementData : public FMeshMaterialShaderElementData
{
public:
	float ParameterValue;
};

bool GetOutlinePassShader(
	const FMaterial& Material,
	FVertexFactoryType* VertexFactoryType,
	TShaderRef<FOutlineVS>& VertexShader,
	TShaderRef<FOutlinePS>& PixelShader
);

/*----------------------------------------------------*/

class FOutlinePassMeshProcessor : public FMeshPassProcessor
{
public:

	FOutlinePassMeshProcessor(const FScene* Scene, ERHIFeatureLevel::Type InFeatureLevel, const FSceneView* InViewIfDynamicMeshCommand, const FMeshPassProcessorRenderState& InPassDrawRenderState, FMeshPassDrawListContext* InDrawListContext);

	virtual void AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1) override final;
	//virtual void CollectPSOInitializers(const FSceneTexturesConfig& SceneTexturesConfig, const FMaterial& Material, const FPSOPrecacheVertexFactoryData& VertexFactoryData, const FPSOPrecacheParams& PreCacheParams, TArray<FPSOPrecacheData>& PSOInitializers) override final;

	FMeshPassProcessorRenderState PassDrawRenderState;
	EShadingPath ShadingPath;

private:
	bool TryAddMeshBatch(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material);

	bool Process(
		const FMeshBatch& MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId,
		const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
		const FMaterial& RESTRICT MaterialResource,
		ERasterizerFillMode MeshFillMode,
		ERasterizerCullMode MeshCullMode);
};