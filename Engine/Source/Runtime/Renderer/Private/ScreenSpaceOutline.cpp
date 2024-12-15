#include "ScreenSpaceOutline.h"
#include "DeferredShadingRenderer.h"
#include "PixelShaderUtils.h"

//这个宏请放在cpp文件中
IMPLEMENT_GLOBAL_SHADER(FScreenSpaceOutLinePS, "/Engine/Private/ScreenSpaceOutLine.usf", "MainPS", SF_Pixel);

void FDeferredShadingSceneRenderer::RenderScreenSpaceOutLinePass(FRDGBuilder& GraphBuilder,FSceneTextures& SceneTextures)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDeferredShadingSceneRenderer::RenderScreenSpaceOutLinePass);
	RDG_EVENT_SCOPE(GraphBuilder, "ScreenSpaceOutLinePass");
    
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];

		RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
		RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);
		View.BeginRenderView();
		
		//GetScreenSpaceOutLineParameters--Start
		FScreenSpaceOutLinePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FScreenSpaceOutLinePS::FParameters>();
		PassParameters->SceneDepthTexture = SceneTextures.Depth.Target;
		PassParameters->NormalTexture = SceneTextures.GBufferA;
		PassParameters->ToonOutlineDataTexture = SceneTextures.ToonOutlineData;
		PassParameters->PointClampSampler = TStaticSamplerState<SF_Point>::GetRHI();
		if (!HasBeenProduced(SceneTextures.ToonOutline))
		{
			// 如果ToonBuffer没被创建，在这里创建
			const FSceneTexturesConfig& Config = View.GetSceneTexturesConfig();
			SceneTextures.ToonOutline = CreateToonOutlineTexture(GraphBuilder, Config.Extent, GFastVRamConfig.ToonOutline);
		}
		PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.ToonOutline, ERenderTargetLoadAction::ELoad);
		//GetScreenSpaceOutLineParameters--End
		
		TShaderMapRef<FScreenSpaceOutLinePS> PixelShader(View.ShaderMap);
		ClearUnusedGraphResources(PixelShader, PassParameters);
       
		GraphBuilder.AddPass(
		   RDG_EVENT_NAME("ScreenSpaceOutline"),
		   PassParameters,
		   ERDGPassFlags::Raster,
		   [&View, PassParameters,PixelShader](FRHICommandList& RHICmdList)
		   {
			  check(PixelShader.IsValid());
		   	  RHICmdList.SetViewport((float)View.ViewRect.Min.X, (float)View.ViewRect.Min.Y, 0.0f, (float)View.ViewRect.Max.X, (float)View.ViewRect.Max.Y, 1.0f);
		   	
			  FGraphicsPipelineStateInitializer GraphicsPSOInit;
			  FPixelShaderUtils::InitFullscreenPipelineState(RHICmdList, View.ShaderMap, PixelShader, /* out */GraphicsPSOInit);
			  GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			  GraphicsPSOInit.bDepthBounds = false;
			  GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
             
			  SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
			  SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);

		   	  FPixelShaderUtils::DrawFullscreenTriangle(RHICmdList);
		   });
	}
    
}

//[Toon-Pipeline][Add-Begin] 增加ToonOutlineBuffer step2
FRDGTextureDesc GetToonOutlineTextureDesc(FIntPoint Extent, ETextureCreateFlags CreateFlags)
{
	//输入的参数：
	//Extent：贴图尺寸；PF_B8G8R8A8：贴图格式，表示RGBA各个通道均为8bit
	//FClearValueBinding::Black:清除值，表示清除贴图时将其清除为黑色
	//TexCreate_UAV：Unordered Access View，允许在着色器中进行随机读写操作
	//TexCreate_RenderTargetable：表示纹理可作为渲染目标使用
	//TexCreate_ShaderResource：表示纹理可作为着色器资源，可以在着色器中进行采样等操作
	return FRDGTextureDesc(FRDGTextureDesc::Create2D(Extent, PF_B8G8R8A8, FClearValueBinding::Black, TexCreate_UAV | TexCreate_RenderTargetable | TexCreate_ShaderResource | CreateFlags));
}

FRDGTextureRef CreateToonOutlineTexture(FRDGBuilder& GraphBuilder, FIntPoint Extent, ETextureCreateFlags CreateFlags)
{
	return GraphBuilder.CreateTexture(GetToonOutlineTextureDesc(Extent, CreateFlags), TEXT("ToonOutlineTexture"));
}
//[Toon-Pipeline][Add-End]

/*----CombineOutline----*/
IMPLEMENT_GLOBAL_SHADER(FOutlineCombinePS, "/Engine/Private/OutlineCombine.usf", "MainPS", SF_Pixel);

void FDeferredShadingSceneRenderer::RenderOutlineCombinePass(FRDGBuilder& GraphBuilder,
	FSceneTextures& SceneTextures)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDeferredShadingSceneRenderer::RenderOutlineCombinePass);
	RDG_EVENT_SCOPE(GraphBuilder, "OutlineCombinePass");
    
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];

		RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
		RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);
		View.BeginRenderView();
		
		//GetScreenSpaceOutLineParameters--Start
		FOutlineCombinePS::FParameters* PassParameters = GraphBuilder.AllocParameters<FOutlineCombinePS::FParameters>();
		PassParameters->BaseColorTexture = SceneTextures.GBufferC;
		PassParameters->ToonOutlineTexture = SceneTextures.ToonOutline;
		PassParameters->ToonOutlineDataTexture = SceneTextures.ToonOutlineData;
		PassParameters->PointClampSampler = TStaticSamplerState<SF_Point>::GetRHI();
		
		PassParameters->RenderTargets[0] = FRenderTargetBinding(SceneTextures.GBufferC, ERenderTargetLoadAction::ELoad);
		//GetScreenSpaceOutLineParameters--End
		
		TShaderMapRef<FOutlineCombinePS> PixelShader(View.ShaderMap);
		ClearUnusedGraphResources(PixelShader, PassParameters);
       
		GraphBuilder.AddPass(
		   RDG_EVENT_NAME("OutlineCombine"),
		   PassParameters,
		   ERDGPassFlags::Raster,
		   [&View, PassParameters,PixelShader](FRHICommandList& RHICmdList)
		   {
			  check(PixelShader.IsValid());
		   	  RHICmdList.SetViewport((float)View.ViewRect.Min.X, (float)View.ViewRect.Min.Y, 0.0f, (float)View.ViewRect.Max.X, (float)View.ViewRect.Max.Y, 1.0f);
		   	  
			  FGraphicsPipelineStateInitializer GraphicsPSOInit;
			  FPixelShaderUtils::InitFullscreenPipelineState(RHICmdList, View.ShaderMap, PixelShader, /* out */GraphicsPSOInit);
			  GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			  GraphicsPSOInit.bDepthBounds = false;
			  GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
             
			  SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
			  SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);

		   	  FPixelShaderUtils::DrawFullscreenTriangle(RHICmdList);
		   });
	}
}