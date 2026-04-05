#pragma once

class Shader
{
public:
	Shader() = default;
	~Shader() = default;

	// filepath: Đường dẫn tới file .hlsl
	// entryPoint: Tên hàm chính (ví dụ: "VSMain" hoặc "PSMain")
	// targetProfile: Phiên bản shader (ví dụ: "vs_5_0" cho Vertex, "ps_5_0" cho Pixel)
	bool Init(const std::wstring& filepath, const std::string& entryPoint, const std::string& targetProfile);

	// Các hàm Get để sau này class PSO lấy dữ liệu đưa vào Pipeline
	const void* GetBufferPointer() const { return shaderBlob->GetBufferPointer(); }
	SIZE_T GetBufferSize() const { return shaderBlob->GetBufferSize(); }
	ID3DBlob* GetBlob() const { return shaderBlob.Get(); }
private:
	ComPtr<ID3DBlob> shaderBlob;
};
