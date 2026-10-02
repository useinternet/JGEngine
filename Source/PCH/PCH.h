#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <queue>
#include <stack>
#include <iostream>
#include <sstream>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <cassert>
#include <filesystem>
#include <chrono>
#include <string>
#include <chrono>
#include <cctype>
#include <codecvt>
#include <xhash>

namespace fs = std::filesystem;

#ifdef _DIRECTX12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#endif

#ifdef _PLATFORM_WINDOWS
#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")
#endif 

#pragma comment(lib, "zlibstatic.lib")
#pragma comment(lib, "assimp-mt.lib")

// 서드파티 헤더(바뀌지 않는다). 거의 모든 파일이 Core.h 로 이것을 다시 읽어서 PCH 에 둔다(DevOps BS-2, 2026-10-02).
// String/String.h 와 같이 4996 을 끄고 넣는다 — fmt 가 stdext::checked_array_iterator(STL4043)를 쓴다.
#pragma warning(disable : 4996)
#include "spdlog/spdlog.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"