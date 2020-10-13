#pragma once

#include "RealSenseTypes.h"
#include "RealSenseOption.generated.h"

UCLASS(ClassGroup="RealSense", BlueprintType)
class REALSENSE_API URealSenseOption : public UObject
{
	GENERATED_UCLASS_BODY()
	friend class URealSenseSensor;

public:

	virtual ~URealSenseOption();
	struct rs2_options* GetHandle();

	float GetCachedValue() const { return Value; }

	UFUNCTION(Category="RealSense", BlueprintCallable)
	float QueryValue();

	UFUNCTION(Category="RealSense", BlueprintCallable)
	void SetValue(float Value);

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	ERealSenseOptionType Type;

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	FString Name;

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	FString Description;

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	FRealSenseOptionRange Range;

private:

	void SetHandle(struct rs2_options* Handle);
	void SetType(ERealSenseOptionType Type);
	void QueryData();

	struct rs2_options* RsOptions = nullptr;
	float Value = 0;
};
