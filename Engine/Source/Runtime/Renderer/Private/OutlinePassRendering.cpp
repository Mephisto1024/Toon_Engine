#pragma once
#include "OutlinePassRendering.h"
#include "ScenePrivate.h"
#include "Materials/MaterialRenderProxy.h"
#include "MeshPassProcessor.inl"
#include "DeferredShadingRenderer.h"
IMPLEMENT_MATERIAL_SHADER_TYPE(, FOutlineVS, TEXT("/Engine/Private/Outline.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(,FOutlinePS, TEXT("/Engine/Private/Outline.usf"), TEXT("MainPS"), SF_Pixel);

bool GetOutlinePassShader(const FMaterial& Material, FVertexFactoryType* VertexFactoryType,
	TShaderRef<FOutlineVS>& VertexShader, TShaderRef<FOutlinePS>& PixelShader)
{
	FMaterialShaderTypes ShaderTypes;
	ShaderTypes.AddShaderType<FOutlineVS>();
	ShaderTypes.AddShaderType<FOutlinePS>();
	FMaterialShaders Shaders;
	if(!Material.TryGetShaders(ShaderTypes,VertexFactoryType,Shaders))
	{
		return false;
	}
	Shaders.TryGetVertexShader(VertexShader);
	Shaders.TryGetPixelShader(PixelShader);
	return true;
}

void SetupOutlinePassState(FMeshPassProcessorRenderState& DrawRenderState)
{
	// Disable color writes, enable depth tests and writes.
	DrawRenderState.SetBlendState(TStaticBlendState<CW_RGB>::GetRHI());
	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI());
}

FMeshPassProcessor* CreateOutlinePassProcessor(ERHIFeatureLevel::Type FeatureLevel, const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	FMeshPassProcessorRenderState OutlinePassState;
	SetupOutlinePassState(OutlinePassState);
	return new FOutlinePassMeshProcessor(Scene, FeatureLevel, InViewIfDynamicMeshCommand, OutlinePassState,InDrawListContext);
}

// RegisterToonPass会将CreateToonPassProcessor函数的地址写入FPassProcessorManager的一个Table里，Table的下标是EShadingPath和EMeshPass
// 这个Table包括了所有Pass的CreatePassProcessor函数，之后引擎就可以根据EShadingPath和EMeshPass找到对应pass的CreatePassProcessor函数
REGISTER_MESHPASSPROCESSOR_AND_PSOCOLLECTOR(OutlinePass,CreateOutlinePassProcessor,EShadingPath::Deferred,EMeshPass::OutlinePass,EMeshPassFlags::MainView)

FOutlinePassMeshProcessor::FOutlinePassMeshProcessor(const FScene* Scene, ERHIFeatureLevel::Type InFeatureLevel,
	const FSceneView* InViewIfDynamicMeshCommand, const FMeshPassProcessorRenderState& InPassDrawRenderState,
	FMeshPassDrawListContext* InDrawListContext)
		:FMeshPassProcessor(Scene,Scene->GetFeatureLevel(),InViewIfDynamicMeshCommand,InDrawListContext)
		,PassDrawRenderState(InPassDrawRenderState)

{
}

void FOutlinePassMeshProcessor::AddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, int32 StaticMeshId)
{
	const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;
    	while (MaterialRenderProxy)
    	{
    		const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
    		if (Material)
    		{
    			if (TryAddMeshBatch(MeshBatch, BatchElementMask, PrimitiveSceneProxy, StaticMeshId, *MaterialRenderProxy, *Material))
    			{
    				break;
    			}
    		}
    
    		MaterialRenderProxy = MaterialRenderProxy->GetFallback(FeatureLevel);
    	}
}

bool FOutlinePassMeshProcessor::TryAddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, int32 StaticMeshId,
	const FMaterialRenderProxy& MaterialRenderProxy, const FMaterial& Material)
{
	const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
	const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(Material, OverrideSettings);
	const ERasterizerCullMode MeshCullMode = CM_CCW;
	return Process(MeshBatch, BatchElementMask, PrimitiveSceneProxy, StaticMeshId, MaterialRenderProxy, Material, MeshFillMode, MeshCullMode);
}

bool FOutlinePassMeshProcessor::Process(const FMeshBatch& MeshBatch, uint64 BatchElementMask,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy, int32 StaticMeshId,
	const FMaterialRenderProxy& MaterialRenderProxy, const FMaterial& MaterialResource,
	ERasterizerFillMode MeshFillMode, ERasterizerCullMode MeshCullMode)
{
	if (Scene->GetShadingPath()==EShadingPath::Deferred)
	{
		const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;
		
		TMeshProcessorShaders<FOutlineVS,FOutlinePS> OutlinePassShaders;
		
		if (!GetOutlinePassShader(
			MaterialResource,
			VertexFactory->GetType(),
			OutlinePassShaders.VertexShader,
			OutlinePassShaders.PixelShader
			))
		{
			return false;
		}

		FOutlineShaderElementData ShaderElementData;
		ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);

		const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(OutlinePassShaders.VertexShader, OutlinePassShaders.PixelShader);
		
		PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>().GetRHI());
		
		BuildMeshDrawCommands(
			MeshBatch,
			BatchElementMask,
			PrimitiveSceneProxy,
			MaterialRenderProxy,
			MaterialResource,
			PassDrawRenderState,
			OutlinePassShaders,
			MeshFillMode,
			MeshCullMode,
			SortKey,
			EMeshPassFeatures::Default,
			ShaderElementData);
	}
	return true;
}

DECLARE_CYCLE_STAT(TEXT("OutlinePass"), STAT_CLP_OutlinePass, STATGROUP_ParallelCommandListMarkers);

BEGIN_SHADER_PARAMETER_STRUCT(FOutlinePassParameters, )
	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
	SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams, InstanceCullingDrawParams)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

FOutlinePassParameters* GetOutlinePassParameters(FRDGBuilder& GraphBuilder, const FViewInfo& View, FSceneTextures& SceneTextures)
{
	FOutlinePassParameters* PassParameters = GraphBuilder.AllocParameters<FOutlinePassParameters>();
	PassParameters->View = View.ViewUniformBuffer;
	
	// 设置RenderTarget
	PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.Color.Target, ERenderTargetLoadAction::ELoad);
	//注释了试试
	PassParameters->RenderTargets.DepthStencil = FDepthStencilBinding(
		SceneTextures.Depth.Target,
		ERenderTargetLoadAction::ELoad,
		ERenderTargetLoadAction::ELoad,
		FExclusiveDepthStencil::DepthWrite_StencilNop
	);
	
	return PassParameters;
}

void FDeferredShadingSceneRenderer::RenderOutlinePass(FRDGBuilder& GraphBuilder, FSceneTextures& SceneTextures)
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];
		RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
		RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);

		FMeshPassProcessorRenderState DrawRenderState;
		SetupOutlinePassState(DrawRenderState);

		const bool bShouldRenderView = View.ShouldRenderView() ;
		if (bShouldRenderView)
		{
			View.BeginRenderView();

			FOutlinePassParameters* PassParameters = GetOutlinePassParameters(GraphBuilder, View, SceneTextures);
			
			View.ParallelMeshDrawCommandPasses[EMeshPass::OutlinePass].BuildRenderingCommands(GraphBuilder, Scene->GPUScene, PassParameters->InstanceCullingDrawParams);

			GraphBuilder.AddPass(
				RDG_EVENT_NAME("OutlinePass"),
				PassParameters,
				ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
				[this, &View, PassParameters](const FRDGPass* InPass, FRHICommandListImmediate& RHICmdList)
			{
				FRDGParallelCommandListSet ParallelCommandListSet(InPass, RHICmdList, GET_STATID(STAT_CLP_OutlinePass), View, FParallelCommandListBindings(PassParameters));
				ParallelCommandListSet.SetHighPriority();
				View.ParallelMeshDrawCommandPasses[EMeshPass::OutlinePass].DispatchDraw(&ParallelCommandListSet, RHICmdList, &PassParameters->InstanceCullingDrawParams);
			});
			
		}
	}
}
