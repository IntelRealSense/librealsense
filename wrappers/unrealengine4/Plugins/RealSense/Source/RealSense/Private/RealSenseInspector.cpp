#include "RealSenseInspector.h"

#define MAX_BUFFER_U16 0xFFFF

template<typename T>
inline UTexture2D* Get(TUniquePtr<T>& Dtex) { return Dtex.Get() ? Dtex.Get()->GetTextureObject() : nullptr; }

inline float GetDepthScale(rs2::device dev) {
	for (auto& sensor : dev.query_sensors()) {
		if (auto depth = sensor.as<rs2::depth_sensor>()) {
			return depth.get_depth_scale();
		}
	}
	throw rs2::error("Depth not supported");
}

ARealSenseInspector::ARealSenseInspector(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	REALSENSE_TRACE(TEXT("+ARealSenseInspector %p"), this);

	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	PclMesh = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent"));
	PclMesh->SetVisibility(false);
	PclMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	PclMesh->Mobility = EComponentMobility::Movable;
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
	PclMesh->SetGenerateOverlapEvents(false);
#else
	PclMesh->bGenerateOverlapEvents = false;
#endif
	PclMesh->SetupAttachment(RootComponent);
}

ARealSenseInspector::~ARealSenseInspector()
{
	REALSENSE_TRACE(TEXT("~ARealSenseInspector %p"), this);

	Stop();
}

bool ARealSenseInspector::Start()
{
	SCOPED_PROFILER;

	try
	{
		FScopeLock Lock(&StateMx);

		REALSENSE_TRACE(TEXT("ARealSenseInspector::Start %p"), this);

		if (RsPipeline.Get() || StartedFlag)
		{
			throw std::runtime_error("Already started");
		}

		PclMesh->SetVisibility(false);

		Context = IRealSensePlugin::Get().GetContext();
		if (!Context)
		{
			throw std::runtime_error("GetContext failed");
		}

		rs2::context_ref RsContext(Context->GetHandle());
		rs2::config RsConfig;

		const bool IsPlaybackMode = (PipelineMode == ERealSensePipelineMode::PlaybackFile);
		if (IsPlaybackMode)
		{
			REALSENSE_TRACE(TEXT("enable_device_from_file: %s"), *CaptureFile);
			RsConfig.enable_device_from_file(TCHAR_TO_ANSI(*CaptureFile));
		}
		else
		{
			if (Context->Devices.Num() == 0)
			{
				Context->QueryDevices();
				if (Context->Devices.Num() == 0)
				{
					throw std::runtime_error("No devices available");
				}
			}

			ActiveDevice = DeviceSerial.IsEmpty() ? Context->GetDeviceById(0) : Context->FindDeviceBySerial(DeviceSerial);
			if (!ActiveDevice)
			{
				throw std::runtime_error("Device not found");
			}

			REALSENSE_TRACE(TEXT("enable_device: SN=%s"), *ActiveDevice->Serial);
			RsConfig.enable_device(std::string(TCHAR_TO_ANSI(*ActiveDevice->Serial)));
		}

		if (bEnableRawDepth || bEnableColorizedDepth || bEnablePcl)
		{
			if (!IsPlaybackMode)
			{
				REALSENSE_TRACE(TEXT("enable_stream DEPTH %dx%d @%d"), DepthConfig.Width, DepthConfig.Height, DepthConfig.Rate);

				EnsureProfileSupported(ActiveDevice, ERealSenseStreamType::STREAM_DEPTH, ERealSenseFormatType::FORMAT_Z16, DepthConfig);
				RsConfig.enable_stream(RS2_STREAM_DEPTH, DepthConfig.Width, DepthConfig.Height, RS2_FORMAT_Z16, DepthConfig.Rate);

				if (bAlignDepthToColor)
				{
					RsAlign.Reset(new rs2::align(RS2_STREAM_COLOR));
				}
			}

			if (bEnableRawDepth)
			{
				// PF_G16 = DXGI_FORMAT_R16_UNORM ; A single-component, 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel.
				// PF_R16F = DXGI_FORMAT_R16_FLOAT ; A single-component, 16-bit floating-point format that supports 16 bits for the red channel.
				// PF_R16_UINT = DXGI_FORMAT_R16_UINT ; A single-component, 16-bit unsigned-integer format that supports 16 bits for the red channel.
				DepthRawDtex.Reset(new FDynamicTexture(TEXT("DepthRaw"), DepthConfig.Width, DepthConfig.Height, PF_G16, TextureCompressionSettings::TC_Displacementmap));
			}

			if (bEnableColorizedDepth)
			{
				DepthColorizedDtex.Reset(new FDynamicTexture(TEXT("DepthColorized"), DepthConfig.Width, DepthConfig.Height, PF_R8G8B8A8, TextureCompressionSettings::TC_VectorDisplacementmap));
			}
		}

		if (bEnableColor || bAlignDepthToColor)
		{
			if (!IsPlaybackMode)
			{
				REALSENSE_TRACE(TEXT("enable_stream COLOR %dx%d @%d"), ColorConfig.Width, ColorConfig.Height, ColorConfig.Rate);

				EnsureProfileSupported(ActiveDevice, ERealSenseStreamType::STREAM_COLOR, ERealSenseFormatType::FORMAT_RGBA8, ColorConfig);
				RsConfig.enable_stream(RS2_STREAM_COLOR, ColorConfig.Width, ColorConfig.Height, RS2_FORMAT_RGBA8, ColorConfig.Rate);
			}

			if (bEnableColor)
			{
				ColorDtex.Reset(new FDynamicTexture(TEXT("Color"), ColorConfig.Width, ColorConfig.Height, PF_R8G8B8A8, TextureCompressionSettings::TC_VectorDisplacementmap));
			}
		}

		if (bEnableInfrared)
		{
			if (!IsPlaybackMode)
			{
				REALSENSE_TRACE(TEXT("enable_stream INFRARED %dx%d @%d"), InfraredConfig.Width, InfraredConfig.Height, InfraredConfig.Rate);

				EnsureProfileSupported(ActiveDevice, ERealSenseStreamType::STREAM_INFRARED, ERealSenseFormatType::FORMAT_Y8, InfraredConfig);
				RsConfig.enable_stream(RS2_STREAM_INFRARED, InfraredConfig.Width, InfraredConfig.Height, RS2_FORMAT_Y8, InfraredConfig.Rate);
			}

			// PF_G8 = DXGI_FORMAT_R8_UNORM ; A single-component, 8-bit unsigned-normalized-integer format that supports 8 bits for the red channel.
			InfraredDtex.Reset(new FDynamicTexture(TEXT("Infrared"), InfraredConfig.Width, InfraredConfig.Height, PF_G8, TextureCompressionSettings::TC_Displacementmap));
		}

		if (bEnablePcl)
		{
			RsPointCloud.Reset(new rs2::pointcloud());
			RsPoints.Reset(new rs2::points());
		}
		PclCalculateFlag = bEnablePcl;

		if (PipelineMode == ERealSensePipelineMode::RecordFile)
		{
			REALSENSE_TRACE(TEXT("enable_record_to_file: %s"), *CaptureFile);
			RsConfig.enable_record_to_file(TCHAR_TO_ANSI(*CaptureFile));
		}

		FlushRenderingCommands(); // wait for textures
		DepthRawTexture = Get(DepthRawDtex);
		DepthColorizedTexture = Get(DepthColorizedDtex);
		ColorTexture = Get(ColorDtex);
		InfraredTexture = Get(InfraredDtex);

		REALSENSE_TRACE(TEXT("Start pipeline"));
		RsPipeline.Reset(new rs2::pipeline());
		rs2::pipeline_profile RsProfile = RsPipeline->start(RsConfig);
		DepthScale = GetDepthScale(RsProfile.get_device());
		REALSENSE_TRACE(TEXT("DepthScale=%f"), DepthScale);
		StartedFlag = true;

		REALSENSE_TRACE(TEXT("Start worker"));
		Worker.Reset(new FRealSenseInspectorWorker(this));
		FString ThreadName(FString::Printf(TEXT("FRealSenseInspectorWorker_%s"), *FGuid::NewGuid().ToString()));
		Thread.Reset(FRunnableThread::Create(Worker.Get(), *ThreadName, 0, TPri_Normal));
		if (!Thread.Get())
		{
			throw std::runtime_error("CreateThread failed");
		}
	}
	catch (const rs2::error& ex)
	{
		auto what(uestr(ex.what()));
		auto func(uestr(ex.get_failed_function()));
		auto args(uestr(ex.get_failed_args()));

		REALSENSE_ERR(TEXT("ARealSenseInspector::Start exception: %s (FUNC %s ; ARGS %s ; TYPE %d"), *what, *func, *args, (int)ex.get_type());
		StartedFlag = false;
	}
	catch (const std::exception& ex)
	{
		REALSENSE_ERR(TEXT("ARealSenseInspector::Start exception: %s"), ANSI_TO_TCHAR(ex.what()));
		StartedFlag = false;
	}

	if (!StartedFlag)
	{
		Stop();
	}

	return StartedFlag ? true : false;
}

void ARealSenseInspector::Stop()
{
	SCOPED_PROFILER;

	try
	{
		FScopeLock Lock(&StateMx);

		StartedFlag = false;

		if (RsPipeline.Get())
		{
			REALSENSE_TRACE(TEXT("Stop pipeline"));
			try {
				NAMED_PROFILER("rs2::pipeline::stop");
				RsPipeline->stop();
			}
			catch (...) {}
		}

		if (Thread.Get())
		{
			REALSENSE_TRACE(TEXT("Join thread"));
			{
				NAMED_PROFILER("JoinRealSenseThread");
				Thread->WaitForCompletion();
			}

			REALSENSE_TRACE(TEXT("Reset thread"));
			Thread.Reset();
		}

		Worker.Reset();
		RsPipeline.Reset();
		RsAlign.Reset();
		RsPoints.Reset();
		RsPointCloud.Reset();

		REALSENSE_TRACE(TEXT("Flush rendering commands"));
		{
			NAMED_PROFILER("FlushRenderingCommands");

			ENQUEUE_RENDER_COMMAND(FlushCommand)(
				[](FRHICommandListImmediate& RHICmdList)
				{
					GRHICommandList.GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);
					RHIFlushResources();
					GRHICommandList.GetImmediateCommandList().ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);
				});
			FlushRenderingCommands();
		}

		REALSENSE_TRACE(TEXT("Destroy dynamic textures"));
		{
			NAMED_PROFILER("DestroyDynamicTextures");
			DepthRawDtex.Reset();
			DepthColorizedDtex.Reset();
			ColorDtex.Reset();
			InfraredDtex.Reset();
		}

		ActiveDevice = nullptr;
		DepthRawTexture = nullptr;
		DepthColorizedTexture = nullptr;
		ColorTexture = nullptr;
		InfraredTexture = nullptr;
	}
	catch (const std::exception& ex)
	{
		REALSENSE_ERR(TEXT("ARealSenseInspector::Stop exception: %s"), ANSI_TO_TCHAR(ex.what()));
	}

	#if defined(PROF_ENABLED)
	Profiler::GetInstance()->LogSummary();
	#endif
}

void ARealSenseInspector::ThreadProc()
{
	REALSENSE_TRACE(TEXT("Enter ARealSenseInspector::ThreadProc %p"), this);

	try
	{
		while (StartedFlag)
		{
			if (bEnablePolling)
			{
				PollFrames();
			}
			else
			{
				WaitFrames();
			}
		}
	}
	catch (const std::exception& ex)
	{
		REALSENSE_ERR(TEXT("ARealSenseInspector::ThreadProc exception: %s"), ANSI_TO_TCHAR(ex.what()));
		StartedFlag = false;
	}

	REALSENSE_TRACE(TEXT("Leave ARealSenseInspector::ThreadProc %p"), this);
}

void ARealSenseInspector::PollFrames()
{
	const double t0 = FPlatformTime::Seconds();

	rs2::frameset Frameset;
	bool GotFrames;

	try
	{
		NAMED_PROFILER("rs2::pipeline::poll_for_frames");
		GotFrames = RsPipeline->poll_for_frames(&Frameset);
	}
	catch (const rs2::error& ex)
	{
		REALSENSE_TRACE(TEXT("poll_for_frames failed: %s"), ANSI_TO_TCHAR(ex.what()));
		GotFrames = false;
	}

	if (GotFrames)
	{
		ProcessFrameset(&Frameset);
	}

	const double t1 = FPlatformTime::Seconds();
	const double dt = t1 - t0;
	if (dt < PollFrameRate)
	{
		FPlatformProcess::Sleep(PollFrameRate - dt);
	}
}

void ARealSenseInspector::WaitFrames()
{
	rs2::frameset Frameset;
	bool GotFrames;

	try
	{
		NAMED_PROFILER("rs2::pipeline::wait_for_frames");
		Frameset = RsPipeline->wait_for_frames((unsigned int)(WaitFrameTimeout * 1000.0f));
		GotFrames = true;
	}
	catch (const rs2::error& ex)
	{
		REALSENSE_TRACE(TEXT("wait_for_frames failed: %s"), ANSI_TO_TCHAR(ex.what()));
		GotFrames = false;
	}

	if (GotFrames)
	{
		ProcessFrameset(&Frameset);
	}
}

void ARealSenseInspector::ProcessFrameset(rs2::frameset* Frameset)
{
	SCOPED_PROFILER;

	FScopedTryLock PclScopedMx;

	if (bEnableRawDepth || bEnableColorizedDepth || bEnablePcl)
	{
		rs2::depth_frame DepthFrame = (bAlignDepthToColor && RsAlign.Get()) ? RsAlign->process(*Frameset).get_depth_frame() : Frameset->get_depth_frame();

		if (bEnableRawDepth && DepthRawDtex.Get())
		{
			NAMED_PROFILER("UpdateDepthRaw");
			DepthRawDtex->Update(DepthFrame);
		}

		if (bEnableColorizedDepth && DepthColorizedDtex.Get())
		{
			NAMED_PROFILER("UpdateDepthColorized");
			auto* Tud = DepthColorizedDtex->AllocBuffer();
			rs2_utils::colorize_depth((rs2_utils::depth_pixel*)Tud->Data, DepthFrame, (int)DepthColormap, DepthMin, DepthMax, DepthScale, bEqualizeHistogram);
			DepthColorizedDtex->EnqueUpdateCommand(Tud);
		}

		if (bEnablePcl && RsPointCloud.Get() && PclCalculateFlag)
		{
			if (PclScopedMx.TryLock(&PointCloudMx))
			{
				NAMED_PROFILER("rs2::pointcloud::calculate");
				*RsPoints = RsPointCloud->calculate(DepthFrame);
			}
		}
	}

	if (bEnableColor)
	{
		auto ColorFrame = Frameset->get_color_frame();

		if (ColorDtex.Get())
		{
			NAMED_PROFILER("UpdateColor");
			ColorDtex->Update(ColorFrame);
		}

		if (PclScopedMx.IsLocked())
		{
			NAMED_PROFILER("rs2::pointcloud::map_to");
			RsPointCloud->map_to(ColorFrame);
		}
	}

	if (PclScopedMx.IsLocked())
	{
		PclScopedMx.Unlock();
		PclFramesetId = FramesetId;
		PclCalculateFlag = false;
		PclReadyFlag = true;
	}

	if (bEnableInfrared && InfraredDtex.Get())
	{
		NAMED_PROFILER("UpdateInfrared");
		InfraredDtex->Update(Frameset->get_infrared_frame());
	}

	//REALSENSE_TRACE(TEXT("Frameset %d"), FramesetId);
	FramesetId++;
}

void ARealSenseInspector::Tick(float DeltaSeconds)
{
	SCOPED_PROFILER;

	Super::Tick(DeltaSeconds);

	if (bEnablePcl && PclMesh && StartedFlag)
	{
		PclRenderAccum += DeltaSeconds;
		if (PclRenderAccum >= PclRenderRate)
		{
			PclRenderAccum = 0;
			PclCalculateFlag = true;
		}

		if (PclReadyFlag)
		{
			FScopedTryLock Mx;
			if (Mx.TryLock(&PointCloudMx))
			{
				UpdatePointCloud();
				PclReadyFlag = false;
			}
		}
	}
	else if (PclMesh)
	{
		PclMesh->SetVisibility(false);
	}
}

void ARealSenseInspector::UpdatePointCloud()
{
	SCOPED_PROFILER;

	const size_t NumPoints = RsPoints->size();
	const size_t DensityPoints = (size_t)FMath::RoundToInt(NumPoints * FMath::Clamp(PclDensity, 0.0f, 1.0f));
	if (!DensityPoints) { PclMesh->SetVisibility(false); return;  }

	const size_t Step = (size_t)FMath::RoundToInt(NumPoints / (float)DensityPoints);
	if (!Step) { PclMesh->SetVisibility(false); return; }

	const size_t RenderPoints = (NumPoints / Step);
	const size_t RenderVertices = RenderPoints * 4;
	const size_t RenderIndices = RenderPoints * 6;
	const size_t MaxBufferIndices = (MAX_BUFFER_U16 / 6) * 6;
	const size_t NumSections = (RenderIndices / MaxBufferIndices) + (RenderIndices % MaxBufferIndices ? 1 : 0);

	#if 1
	REALSENSE_TRACE(TEXT(
		"PCL Id=%zu NumPoints=%zu "
		"RenderPoints=%zu RenderVertices=%zu RenderIndices=%zu NumSections=%zu "
		"Density=%.3f Step=%zu"), 
		PclFramesetId, NumPoints,
		RenderPoints, RenderVertices, RenderIndices, NumSections,
		PclDensity, Step
	);
	#endif

	const rs2::vertex* SrcVertices = RsPoints->get_vertices();
	const rs2::texture_coordinate* SrcTexcoords = RsPoints->get_texture_coordinates();

	FPointCloudVertex PV[4];
	const float Size = (PclVoxelSize * 0.5f);
	size_t SectionId = 0;
	size_t PointId = 0;
	size_t VertexId = 0;
	size_t IndexId = 0;
	size_t NumInvalid = 0;

	while (PointId < NumPoints && SectionId < NumSections)
	{
		if (!PclMeshData.Contains(SectionId))
		{
			NAMED_PROFILER("AllocMeshSection");
			PclMeshData.Add(SectionId, MakeUnique<FMeshSection>());
			PclMeshData[SectionId]->PclVertices.SetNumUninitialized(MAX_BUFFER_U16, false);
			PclMeshData[SectionId]->PclIndices.SetNumUninitialized(MAX_BUFFER_U16, false);
		}

		if (!PclMesh->DoesSectionExist(SectionId))
		{
			NAMED_PROFILER("CreateMeshSection");
			PclMesh->CreateMeshSection(SectionId, PclMeshData[SectionId]->PclVertices, PclMeshData[SectionId]->PclIndices, false, EUpdateFrequency::Frequent);
			PclMesh->SetMaterial(SectionId, PclMaterial);
		}

		FPointCloudVertex* DstVertices = &(PclMeshData[SectionId]->PclVertices[0]);
		int32* DstIndices = &(PclMeshData[SectionId]->PclIndices[0]);
		VertexId = 0;
		IndexId = 0;

		{NAMED_PROFILER("FillMeshBuffers");
		while (PointId < NumPoints && IndexId + 6 <= MaxBufferIndices)
		{
			const auto V = SrcVertices[PointId];
			if (!V.z)
			{
				PointId += Step;
				NumInvalid++;
				continue;
			}

			// the positive x-axis points to the right
			// the positive y-axis points down
			// the positive z-axis points forward
			const FVector Pos = FVector(V.z, V.x, -V.y) * 100.0f * PclScale; // meters to mm

			PV[0].Position = FVector(Pos.X, Pos.Y - Size, Pos.Z - Size);
			PV[1].Position = FVector(Pos.X, Pos.Y - Size, Pos.Z + Size);
			PV[2].Position = FVector(Pos.X, Pos.Y + Size, Pos.Z + Size);
			PV[3].Position = FVector(Pos.X, Pos.Y + Size, Pos.Z - Size);

			PV[0].Normal = FVector(-1, 0, 0);
			PV[1].Normal = FVector(-1, 0, 0);
			PV[2].Normal = FVector(-1, 0, 0);
			PV[3].Normal = FVector(-1, 0, 0);

			PV[0].Tangent = FVector(0, 0, 1);
			PV[1].Tangent = FVector(0, 0, 1);
			PV[2].Tangent = FVector(0, 0, 1);
			PV[3].Tangent = FVector(0, 0, 1);

			const auto T = SrcTexcoords[PointId];
			PV[0].UV0 = FVector2D(T.u, T.v);
			PV[1].UV0 = FVector2D(T.u, T.v);
			PV[2].UV0 = FVector2D(T.u, T.v);
			PV[3].UV0 = FVector2D(T.u, T.v);

			DstVertices[0] = PV[0];
			DstVertices[1] = PV[1];
			DstVertices[2] = PV[2];
			DstVertices[3] = PV[3];

			DstIndices[0] = VertexId + 0;
			DstIndices[1] = VertexId + 2;
			DstIndices[2] = VertexId + 1;
			DstIndices[3] = VertexId + 0;
			DstIndices[4] = VertexId + 3;
			DstIndices[5] = VertexId + 2;

			DstVertices += 4;
			DstIndices += 6;
			VertexId += 4;
			IndexId += 6;
			PointId += Step;
		}}

		if (IndexId > 0)
		{
			#if 0
			REALSENSE_TRACE(TEXT("section id=%zu points=%zu vert=%zu ind=%zu total=%zu invalid=%zu"),
				SectionId, VertexId / 4, VertexId, IndexId, PointId, NumInvalid);
			#endif

			NAMED_PROFILER("UpdateMeshSection");
			PclMeshData[SectionId]->PclVertices.SetNumUninitialized(VertexId, false);
			PclMeshData[SectionId]->PclIndices.SetNumUninitialized(IndexId, false);
			PclMesh->UpdateMeshSection(SectionId, PclMeshData[SectionId]->PclVertices, PclMeshData[SectionId]->PclIndices); // TODO: SLOW AS HELL
			PclMesh->SetMeshSectionVisible(SectionId, true);

			SectionId++;
		}
	}

	const auto NumMeshSections = PclMesh->GetNumSections();
	if (NumMeshSections > SectionId)
	{
		for (size_t i = SectionId; i < NumMeshSections; ++i)
		{
			//REALSENSE_TRACE(TEXT("hide id=%zu"), i);
			PclMesh->SetMeshSectionVisible(i, false);
		}
	}

	//REALSENSE_TRACE(TEXT("total=%zu invalid=%zu"), PointId, NumInvalid);
	PclMesh->SetVisibility(SectionId != 0);
}

void ARealSenseInspector::SetPointCloudMaterial(int SectionId, UMaterialInterface* Material)
{
	PclMaterial = Material;
	if (PclMesh)
	{
		for (auto i = 0; i < PclMesh->GetNumSections(); ++i)
		{
			PclMesh->SetMaterial(i, Material);
		}
	}
}

void ARealSenseInspector::EnsureProfileSupported(URealSenseDevice* Device, ERealSenseStreamType StreamType, ERealSenseFormatType Format, FRealSenseStreamMode Mode)
{
	FRealSenseStreamProfile Profile;
	Profile.StreamType = StreamType;
	Profile.Format = Format;
	Profile.Width = Mode.Width;
	Profile.Height = Mode.Height;
	Profile.Rate = Mode.Rate;

	if (!Device->SupportsProfile(Profile))
	{
		throw std::runtime_error("Profile not supported");
	}
}
