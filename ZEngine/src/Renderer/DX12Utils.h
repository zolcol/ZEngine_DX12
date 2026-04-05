#include <system_error> // Thay cho <comdef.h>

#define CHECK(x) \
    do { \
        HRESULT hr__ = (x); \
        if (FAILED(hr__)) { \
            /* Dùng system_category để dịch mã lỗi C++ chuẩn */ \
            std::string errMsg = HRToString(hr__); \
            ENGINE_FATAL("DirectX 12 Error!"); \
            ENGINE_FATAL("Function: {}", #x); \
            ENGINE_FATAL("Code: 0x{:08X} - {}", (unsigned int)hr__, errMsg); \
            __debugbreak(); \
        } \
    } while(0)

// Hàm chuyển đổi wchar_t* sang char* (UTF-8)
inline std::string WStringToString(const std::wstring & wstr)
{
	if (wstr.empty()) return std::string();

	// Tính toán kích thước cần thiết
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);

	// Đổ dữ liệu sang chuỗi std::string thông thường
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

// Hàm chuyển đổi HR sang tiếng người
inline std::string HRToString(const HRESULT& hr)
{
    return std::system_category().message(hr);
}