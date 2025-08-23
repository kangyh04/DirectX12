#include "BaseApp.h"

class CameraAndDynamicIndexingApp : public BaseApp
{
public:
	CameraAndDynamicIndexingApp(HINSTANCE hInstance);
	CameraAndDynamicIndexingApp(const CameraAndDynamicIndexingApp& rhs) = delete;
	CameraAndDynamicIndexingApp& operator=(const CameraAndDynamicIndexingApp& rhs) = delete;
	~CameraAndDynamicIndexingApp();

	virtual bool Initialize() override;

private:
};