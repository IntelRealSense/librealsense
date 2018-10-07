#pragma once

#include "RealSensePlugin.h"

class FRealSensePlugin : public IRealSensePlugin
{
public:

	FRealSensePlugin();
	virtual ~FRealSensePlugin();
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual int GenId() override;
	virtual class URealSenseContext* GetContext() override;

private:

	void ReleaseContext();

private:

	FCriticalSection UniqIdMx;
	int UniqId = 0;

	FCriticalSection ContextMx;
	struct rs2_context* RsContext = nullptr;
	class URealSenseContext* Context = nullptr;
};
