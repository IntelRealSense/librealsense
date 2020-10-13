#pragma once

#include "PCH.h"
#include "RealSenseTypes.h"
#include "RuntimeMeshComponent.h"
#include "RealSenseInspector.generated.h"

// DECLARE_RUNTIME_MESH_VERTEX(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)
DECLARE_RUNTIME_MESH_VERTEX(FPointCloudVertex, true, true, true, false, 1, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision)

UCLASS(ClassGroup="RealSense", BlueprintType)
class REALSENSE_API ARealSenseInspector : public AActor
{
	GENERATED_UCLASS_BODY()
	friend class FRealSenseInspectorWorker;
	friend class FRealSenseInspectorCustomization;

public:

	virtual ~ARealSenseInspector();

	UPROPERTY(Category="RealSense", BlueprintReadOnly)
	class URealSenseContext* Context;

	UPROPERTY(Category="RealSense", BlueprintReadOnly)
	class URealSenseDevice* ActiveDevice;

	UFUNCTION(Category="RealSense", BlueprintCallable)
	virtual bool Start();

	UFUNCTION(Category="RealSense", BlueprintCallable)
	virtual void Stop();

	// Device

	UPROPERTY(Category="Device", BlueprintReadWrite, EditAnywhere)
	FString DeviceSerial;

	UPROPERTY(Category = "Device", BlueprintReadWrite, EditAnywhere)
	ERealSensePipelineMode PipelineMode = ERealSensePipelineMode::CaptureOnly;

	UPROPERTY(Category = "Device", BlueprintReadWrite, EditAnywhere)
	FString CaptureFile;

	UPROPERTY(Category="Device", BlueprintReadWrite, EditAnywhere)
	bool bEnablePolling = false;

	UPROPERTY(Category="Device", BlueprintReadWrite, EditAnywhere, meta=(ClampMin="0.001", ClampMax="1", UIMin="0.001", UIMax="1"))
	float PollFrameRate = 1.0f / 60.0f;

	UPROPERTY(Category="Device", BlueprintReadWrite, EditAnywhere, meta=(ClampMin="0.001", ClampMax="10", UIMin="0.001", UIMax="10"))
	float WaitFrameTimeout = 1.0f;

	// Depth

	UPROPERTY(Category="DepthStream", BlueprintReadWrite, EditAnywhere)
	FRealSenseStreamMode DepthConfig;

	UPROPERTY(Category="DepthStream", BlueprintReadWrite, EditAnywhere)
	bool bEnableRawDepth = true;

	UPROPERTY(Category="DepthStream", BlueprintReadWrite, EditAnywhere)
	bool bEnableColorizedDepth = true;

	UPROPERTY(Category = "DepthStream", BlueprintReadWrite, EditAnywhere)
	bool bAlignDepthToColor = true;

	UPROPERTY(Category="DepthStream", BlueprintReadWrite, EditAnywhere)
	bool bEqualizeHistogram = true;

	UPROPERTY(Category="DepthStream", BlueprintReadWrite, EditAnywhere, meta=(ClampMin="0", ClampMax="10", UIMin="0", UIMax="10"))
	float DepthMin = 0;

	UPROPERTY(Category="DepthStream", BlueprintReadWrite, EditAnywhere, meta=(ClampMin="0.1", ClampMax="10", UIMin="0.1", UIMax="10"))
	float DepthMax = 10;

	UPROPERTY(Category="DepthStream", BlueprintReadWrite, EditAnywhere)
	ERealSenseDepthColormap DepthColormap;

	UPROPERTY(Category="DepthStream", BlueprintReadOnly)
	float DepthScale = 0.001f;

	UPROPERTY(Category="DepthStream", BlueprintReadOnly)
	UTexture2D* DepthRawTexture;

	UPROPERTY(Category="DepthStream", BlueprintReadOnly)
	UTexture2D* DepthColorizedTexture;

	// Color

	UPROPERTY(Category="ColorStream", BlueprintReadWrite, EditAnywhere)
	FRealSenseStreamMode ColorConfig;

	UPROPERTY(Category="ColorStream", BlueprintReadWrite, EditAnywhere)
	bool bEnableColor = true;

	UPROPERTY(Category="ColorStream", BlueprintReadOnly)
	UTexture2D* ColorTexture;

	// Infrared

	UPROPERTY(Category="InfraredStream", BlueprintReadWrite, EditAnywhere)
	FRealSenseStreamMode InfraredConfig;

	UPROPERTY(Category="InfraredStream", BlueprintReadWrite, EditAnywhere)
	bool bEnableInfrared = true;

	UPROPERTY(Category="InfraredStream", BlueprintReadOnly)
	UTexture2D* InfraredTexture;

	// PointCloud

	UFUNCTION(Category="PointCloud", BlueprintCallable)
	void SetPointCloudMaterial(int SectionId, UMaterialInterface* Material);

	UPROPERTY(Category="PointCloud", BlueprintReadWrite, EditAnywhere)
	bool bEnablePcl = false;

	UPROPERTY(Category="PointCloud", BlueprintReadWrite, EditAnywhere, meta=(ClampMin="0.0", UIMin="0.0"))
	float PclScale = 1.0f;

	UPROPERTY(Category="PointCloud", BlueprintReadWrite, EditAnywhere, meta=(ClampMin="0.0001", ClampMax="10", UIMin="0.0001", UIMax="10"))
	float PclVoxelSize = 1.0f;

	UPROPERTY(Category="PointCloud", BlueprintReadWrite, EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float PclDensity = 0.2f;

	UPROPERTY(Category="PointCloud", BlueprintReadWrite, EditAnywhere, meta=(ClampMin="0.016", ClampMax="100.0", UIMin="0.016", UIMax="100.0"))
	float PclRenderRate = 5;

	UPROPERTY(Category="PointCloud", BlueprintReadOnly)
	class URuntimeMeshComponent* PclMesh;

	UPROPERTY(Category = "PointCloud", BlueprintReadOnly)
	class UMaterialInterface* PclMaterial;

protected:

	// AActor
	virtual void Tick(float DeltaSeconds) override;

private:

	void ThreadProc();
	void PollFrames();
	void WaitFrames();
	void ProcessFrameset(class rs2::frameset* Frameset);
	void UpdatePointCloud();
	void EnsureProfileSupported(class URealSenseDevice* Device, ERealSenseStreamType StreamType, ERealSenseFormatType Format, FRealSenseStreamMode Mode);

	FCriticalSection StateMx;
	FCriticalSection PointCloudMx;

	TUniquePtr<class FDynamicTexture> DepthRawDtex;
	TUniquePtr<class FDynamicTexture> DepthColorizedDtex;
	TUniquePtr<class FDynamicTexture> ColorDtex;
	TUniquePtr<class FDynamicTexture> InfraredDtex;

	TUniquePtr<class rs2::pipeline> RsPipeline;
	TUniquePtr<class rs2::device> RsDevice;
	TUniquePtr<class rs2::align> RsAlign;

	struct FMeshSection
	{
		TArray<FPointCloudVertex> PclVertices;
		TArray<int32> PclIndices;
	};

	TUniquePtr<class rs2::pointcloud> RsPointCloud;
	TUniquePtr<class rs2::points> RsPoints;
	TMap< int32, TUniquePtr<FMeshSection> > PclMeshData;
	int PclFramesetId = 0;
	float PclRenderAccum = 0;
	volatile int PclCalculateFlag = false;
	volatile int PclReadyFlag = false;

	TUniquePtr<class FRealSenseInspectorWorker> Worker;
	TUniquePtr<class FRunnableThread> Thread;
	volatile int StartedFlag = false;
	volatile int FramesetId = 0;
};

class FRealSenseInspectorWorker : public FRunnable
{
public:
	FRealSenseInspectorWorker(ARealSenseInspector* Context) { this->Context = Context; }
	virtual ~FRealSenseInspectorWorker() {}
	virtual bool Init() { return true; }
	virtual uint32 Run() { Context->ThreadProc(); return 0; }
	virtual void Stop() { Context->StartedFlag = false; }

private:
	ARealSenseInspector* Context = nullptr;
};
