#pragma once

// Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

// COM
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// DirectX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "directxtex.h"

// IMGUI
#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

// Math
#include <DirectXMath.h>
using namespace DirectX;

// STL
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <cstdint>
#include <iostream>
#include "entt/entt.hpp"
#include <entt/meta/factory.hpp>
#include <entt/meta/resolve.hpp>

using namespace entt::literals;
// Log
#include "Core/Log.h"
#include "Renderer/DX12Utils.h"

// Render Types
#include "Renderer/RenderTypes.h"