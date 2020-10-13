#pragma once

struct FTextureUpdateData
{
	class FDynamicTexture* Context;
	void* Data;
	uint32 DataSize;
	uint32 Stride;
	uint32 Width;
	uint32 Height;
};

class FDynamicTexture
{
private:

	FString Name;
	UTexture2D* TextureObject = nullptr;

	std::vector<FTextureUpdateData*> TudPool;
	FThreadSafeCounter CommandCounter;

	FCriticalSection StateMx;
	FCriticalSection TudMx;

	int Width = 0;
	int Height = 0;
	int Bpp = 0;
	EPixelFormat Format = PF_Unknown;
	TextureCompressionSettings Compression = TextureCompressionSettings::TC_VectorDisplacementmap;

private:

	void RenderCmd_CreateTexture();
	void RenderCmd_UpdateTexture(FTextureUpdateData* Tud);

public:

	FDynamicTexture(FString Name, int Width, int Height, EPixelFormat Format, TextureCompressionSettings Compression);
	virtual ~FDynamicTexture();

	FTextureUpdateData* AllocBuffer();
	void EnqueUpdateCommand(FTextureUpdateData* Tud);

	void Update(const rs2::video_frame& Frame);
	void CopyData(FTextureUpdateData* Tud, const rs2::video_frame& Frame);

	inline UTexture2D* GetTextureObject() { return TextureObject; }
	int GetWidth() const { return Width; }
	int GetHeight() const { return Height; }
	int GetBpp() const { return Bpp; }
	EPixelFormat GetFormat() const { return Format; }
	TextureCompressionSettings GetCompression() const { return Compression; }
};
