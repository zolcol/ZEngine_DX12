#include "pch.h"
#include "Shader.h"

bool Shader::Init(const std::wstring& filepath, const std::string& entryPoint, const std::string& targetProfile)
{
	uint32_t compileFlags = 0;
#ifdef _DEBUG
	compileFlags = D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#endif // _DEBUG

	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3DCompileFromFile(
		filepath.c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint.c_str(),
		targetProfile.c_str(),
		compileFlags,
		0,
		&shaderBlob,
		&errorBlob
	);

	// Xử lý in lỗi rõ ràng nếu biên dịch HLSL thất bại
	if (errorBlob)
	{
		std::string errorMsg = (char*)errorBlob->GetBufferPointer();
		if (FAILED(hr))
		{
			ENGINE_FATAL("Shader Compile Error in file: {}", WStringToString(filepath));
			ENGINE_FATAL("{}", errorMsg);
			return false;
		}
		else
		{
			ENGINE_WARN("Shader Compile Warning in file: {}", WStringToString(filepath));
			ENGINE_WARN("{}", errorMsg);
		}
	}

	if (FAILED(hr))
	{
		std::string errorStr = HRToString(hr);
		ENGINE_FATAL("Shader Compile Error in file: {} <= Reason: {}", WStringToString(filepath), errorStr);
		return false;
	}
	return true;
}
