/* Stand-in for vkd3d-proton's include/vkd3d_win32.h when building demos/gears.c standalone with
 * llvm-mingw: plain mingw-w64 Windows/DXGI/D3D12 headers instead of vkd3d-proton's widl output.
 * Like the original it enables the widl C inline wrappers (needed for the aggregate-return
 * ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart). */
#ifndef __VKD3D_WIN32_H
#define __VKD3D_WIN32_H
#ifndef INITGUID
#define INITGUID
#endif
#ifndef COBJMACROS
#define COBJMACROS
#endif
#define WIDL_C_INLINE_WRAPPERS
#include <windows.h>
#ifdef __MINGW32__
#undef FORCEINLINE
#define FORCEINLINE inline
#endif
#include <dxgi1_4.h>
#include <d3d12.h>
#endif
