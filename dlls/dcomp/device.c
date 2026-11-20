/*
 * Copyright 2020 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objidl.h"
#include "dxgi.h"
#include "initguid.h"
#include "dcomp.h"
/* Only IWineDXGISwapChain is used here; these are referenced by winedxgi.h's other interfaces. */
struct wined3d_resource;
struct wined3d_texture;
#include "wine/winedxgi.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(dcomp);

/* This is enough of DirectComposition to show a single composition swapchain in a window:
 * a device, targets for windows, and visuals whose content is a swapchain. Committing makes
 * each target's root visual swapchain present into the target's window. Transforms, clips,
 * effects, surfaces and visual trees aren't composited. */

struct composition_device
{
    IDCompositionDevice IDCompositionDevice_iface;
    LONG refcount;
    CRITICAL_SECTION cs;
    struct list targets;
};

struct composition_target
{
    IDCompositionTarget IDCompositionTarget_iface;
    LONG refcount;
    struct composition_device *device;
    struct list entry;
    HWND window;
    IDCompositionVisual *root;
};

struct composition_visual
{
    IDCompositionVisual IDCompositionVisual_iface;
    LONG refcount;
    IUnknown *content;
};

static inline struct composition_visual *impl_from_IDCompositionVisual(IDCompositionVisual *iface)
{
    return CONTAINING_RECORD(iface, struct composition_visual, IDCompositionVisual_iface);
}

static HRESULT STDMETHODCALLTYPE visual_QueryInterface(IDCompositionVisual *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IDCompositionVisual) || IsEqualGUID(iid, &IID_IUnknown))
    {
        IDCompositionVisual_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE visual_AddRef(IDCompositionVisual *iface)
{
    struct composition_visual *visual = impl_from_IDCompositionVisual(iface);
    ULONG refcount = InterlockedIncrement(&visual->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);
    return refcount;
}

static ULONG STDMETHODCALLTYPE visual_Release(IDCompositionVisual *iface)
{
    struct composition_visual *visual = impl_from_IDCompositionVisual(iface);
    ULONG refcount = InterlockedDecrement(&visual->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (visual->content)
            IUnknown_Release(visual->content);
        free(visual);
    }
    return refcount;
}

static HRESULT STDMETHODCALLTYPE visual_SetOffsetXAnimation(IDCompositionVisual *iface, IDCompositionAnimation *animation)
{
    FIXME("iface %p, animation %p stub!\n", iface, animation);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE visual_SetOffsetX(IDCompositionVisual *iface, float offset_x)
{
    if (offset_x)
        FIXME("iface %p, offset_x %.8e stub!\n", iface, offset_x);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetOffsetYAnimation(IDCompositionVisual *iface, IDCompositionAnimation *animation)
{
    FIXME("iface %p, animation %p stub!\n", iface, animation);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE visual_SetOffsetY(IDCompositionVisual *iface, float offset_y)
{
    if (offset_y)
        FIXME("iface %p, offset_y %.8e stub!\n", iface, offset_y);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetTransformObject(IDCompositionVisual *iface, IDCompositionTransform *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetTransform(IDCompositionVisual *iface, const D2D_MATRIX_3X2_F *matrix)
{
    FIXME("iface %p, matrix %p stub!\n", iface, matrix);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetTransformParent(IDCompositionVisual *iface, IDCompositionVisual *parent)
{
    FIXME("iface %p, parent %p stub!\n", iface, parent);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetEffect(IDCompositionVisual *iface, IDCompositionEffect *effect)
{
    FIXME("iface %p, effect %p stub!\n", iface, effect);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetBitmapInterpolationMode(IDCompositionVisual *iface,
        enum DCOMPOSITION_BITMAP_INTERPOLATION_MODE mode)
{
    TRACE("iface %p, mode %#x.\n", iface, mode);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetBorderMode(IDCompositionVisual *iface, enum DCOMPOSITION_BORDER_MODE mode)
{
    TRACE("iface %p, mode %#x.\n", iface, mode);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetClipObject(IDCompositionVisual *iface, IDCompositionClip *clip)
{
    FIXME("iface %p, clip %p stub!\n", iface, clip);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetClip(IDCompositionVisual *iface, const D2D_RECT_F *rect)
{
    FIXME("iface %p, rect %p stub!\n", iface, rect);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetContent(IDCompositionVisual *iface, IUnknown *content)
{
    struct composition_visual *visual = impl_from_IDCompositionVisual(iface);

    TRACE("iface %p, content %p.\n", iface, content);

    if (content)
        IUnknown_AddRef(content);
    if (visual->content)
        IUnknown_Release(visual->content);
    visual->content = content;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_AddVisual(IDCompositionVisual *iface, IDCompositionVisual *visual,
        BOOL insert_above, IDCompositionVisual *reference_visual)
{
    FIXME("iface %p, visual %p, insert_above %d, reference_visual %p stub!\n",
            iface, visual, insert_above, reference_visual);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_RemoveVisual(IDCompositionVisual *iface, IDCompositionVisual *visual)
{
    FIXME("iface %p, visual %p stub!\n", iface, visual);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_RemoveAllVisuals(IDCompositionVisual *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE visual_SetCompositeMode(IDCompositionVisual *iface,
        enum DCOMPOSITION_COMPOSITE_MODE mode)
{
    TRACE("iface %p, mode %#x.\n", iface, mode);
    return S_OK;
}

static const IDCompositionVisualVtbl visual_vtbl =
{
    visual_QueryInterface,
    visual_AddRef,
    visual_Release,
    visual_SetOffsetXAnimation,
    visual_SetOffsetX,
    visual_SetOffsetYAnimation,
    visual_SetOffsetY,
    visual_SetTransformObject,
    visual_SetTransform,
    visual_SetTransformParent,
    visual_SetEffect,
    visual_SetBitmapInterpolationMode,
    visual_SetBorderMode,
    visual_SetClipObject,
    visual_SetClip,
    visual_SetContent,
    visual_AddVisual,
    visual_RemoveVisual,
    visual_RemoveAllVisuals,
    visual_SetCompositeMode,
};

static struct composition_visual *unsafe_impl_from_IDCompositionVisual(IDCompositionVisual *iface)
{
    if (!iface || iface->lpVtbl != &visual_vtbl)
        return NULL;
    return impl_from_IDCompositionVisual(iface);
}

static inline struct composition_target *impl_from_IDCompositionTarget(IDCompositionTarget *iface)
{
    return CONTAINING_RECORD(iface, struct composition_target, IDCompositionTarget_iface);
}

static HRESULT STDMETHODCALLTYPE target_QueryInterface(IDCompositionTarget *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IDCompositionTarget) || IsEqualGUID(iid, &IID_IUnknown))
    {
        IDCompositionTarget_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE target_AddRef(IDCompositionTarget *iface)
{
    struct composition_target *target = impl_from_IDCompositionTarget(iface);
    ULONG refcount = InterlockedIncrement(&target->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);
    return refcount;
}

static ULONG STDMETHODCALLTYPE target_Release(IDCompositionTarget *iface)
{
    struct composition_target *target = impl_from_IDCompositionTarget(iface);
    ULONG refcount = InterlockedDecrement(&target->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        struct composition_device *device = target->device;

        EnterCriticalSection(&device->cs);
        list_remove(&target->entry);
        LeaveCriticalSection(&device->cs);
        if (target->root)
            IDCompositionVisual_Release(target->root);
        IDCompositionDevice_Release(&device->IDCompositionDevice_iface);
        free(target);
    }
    return refcount;
}

static HRESULT STDMETHODCALLTYPE target_SetRoot(IDCompositionTarget *iface, IDCompositionVisual *visual)
{
    struct composition_target *target = impl_from_IDCompositionTarget(iface);

    TRACE("iface %p, visual %p.\n", iface, visual);

    if (visual && !unsafe_impl_from_IDCompositionVisual(visual))
        return E_INVALIDARG;

    EnterCriticalSection(&target->device->cs);
    if (visual)
        IDCompositionVisual_AddRef(visual);
    if (target->root)
        IDCompositionVisual_Release(target->root);
    target->root = visual;
    LeaveCriticalSection(&target->device->cs);
    return S_OK;
}

static const IDCompositionTargetVtbl target_vtbl =
{
    target_QueryInterface,
    target_AddRef,
    target_Release,
    target_SetRoot,
};

static inline struct composition_device *impl_from_IDCompositionDevice(IDCompositionDevice *iface)
{
    return CONTAINING_RECORD(iface, struct composition_device, IDCompositionDevice_iface);
}

static HRESULT STDMETHODCALLTYPE device_QueryInterface(IDCompositionDevice *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IDCompositionDevice) || IsEqualGUID(iid, &IID_IUnknown))
    {
        IDCompositionDevice_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE device_AddRef(IDCompositionDevice *iface)
{
    struct composition_device *device = impl_from_IDCompositionDevice(iface);
    ULONG refcount = InterlockedIncrement(&device->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);
    return refcount;
}

static ULONG STDMETHODCALLTYPE device_Release(IDCompositionDevice *iface)
{
    struct composition_device *device = impl_from_IDCompositionDevice(iface);
    ULONG refcount = InterlockedDecrement(&device->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        device->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&device->cs);
        free(device);
    }
    return refcount;
}

static HRESULT STDMETHODCALLTYPE device_Commit(IDCompositionDevice *iface)
{
    struct composition_device *device = impl_from_IDCompositionDevice(iface);
    struct composition_target *target;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&device->cs);
    LIST_FOR_EACH_ENTRY(target, &device->targets, struct composition_target, entry)
    {
        struct composition_visual *root = unsafe_impl_from_IDCompositionVisual(target->root);
        IWineDXGISwapChain *swapchain;

        if (!root || !root->content)
            continue;
        if (FAILED(IUnknown_QueryInterface(root->content, &IID_IWineDXGISwapChain, (void **)&swapchain)))
        {
            FIXME("Unsupported visual content %p.\n", root->content);
            continue;
        }
        IWineDXGISwapChain_set_target_window(swapchain, target->window);
        IWineDXGISwapChain_Release(swapchain);
    }
    LeaveCriticalSection(&device->cs);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE device_WaitForCommitCompletion(IDCompositionDevice *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE device_GetFrameStatistics(IDCompositionDevice *iface,
        DCOMPOSITION_FRAME_STATISTICS *statistics)
{
    FIXME("iface %p, statistics %p stub!\n", iface, statistics);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE device_CreateTargetForHwnd(IDCompositionDevice *iface, HWND window,
        BOOL topmost, IDCompositionTarget **out)
{
    struct composition_device *device = impl_from_IDCompositionDevice(iface);
    struct composition_target *target;

    TRACE("iface %p, window %p, topmost %d, out %p.\n", iface, window, topmost, out);

    if (!out)
        return E_POINTER;
    *out = NULL;
    if (!IsWindow(window))
        return E_INVALIDARG;
    if (!(target = calloc(1, sizeof(*target))))
        return E_OUTOFMEMORY;

    target->IDCompositionTarget_iface.lpVtbl = &target_vtbl;
    target->refcount = 1;
    target->window = window;
    IDCompositionDevice_AddRef(&(target->device = device)->IDCompositionDevice_iface);
    EnterCriticalSection(&device->cs);
    list_add_tail(&device->targets, &target->entry);
    LeaveCriticalSection(&device->cs);

    *out = &target->IDCompositionTarget_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE device_CreateVisual(IDCompositionDevice *iface, IDCompositionVisual **out)
{
    struct composition_visual *visual;

    TRACE("iface %p, out %p.\n", iface, out);

    if (!out)
        return E_POINTER;
    if (!(visual = calloc(1, sizeof(*visual))))
        return E_OUTOFMEMORY;

    visual->IDCompositionVisual_iface.lpVtbl = &visual_vtbl;
    visual->refcount = 1;
    *out = &visual->IDCompositionVisual_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE device_CreateSurface(IDCompositionDevice *iface, UINT width, UINT height,
        DXGI_FORMAT format, DXGI_ALPHA_MODE alpha_mode, IDCompositionSurface **surface)
{
    FIXME("iface %p, width %u, height %u, format %#x, alpha_mode %#x, surface %p stub!\n",
            iface, width, height, format, alpha_mode, surface);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE device_CreateVirtualSurface(IDCompositionDevice *iface, UINT width, UINT height,
        DXGI_FORMAT format, DXGI_ALPHA_MODE alpha_mode, IDCompositionVirtualSurface **surface)
{
    FIXME("iface %p, width %u, height %u, format %#x, alpha_mode %#x, surface %p stub!\n",
            iface, width, height, format, alpha_mode, surface);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE device_CreateSurfaceFromHandle(IDCompositionDevice *iface, HANDLE handle,
        IUnknown **surface)
{
    FIXME("iface %p, handle %p, surface %p stub!\n", iface, handle, surface);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE device_CreateSurfaceFromHwnd(IDCompositionDevice *iface, HWND window,
        IUnknown **surface)
{
    FIXME("iface %p, window %p, surface %p stub!\n", iface, window, surface);
    return E_NOTIMPL;
}

#define DEVICE_CREATE_STUB(name, type) \
static HRESULT STDMETHODCALLTYPE device_##name(IDCompositionDevice *iface, type **out) \
{ \
    FIXME("iface %p, out %p stub!\n", iface, out); \
    return E_NOTIMPL; \
}

DEVICE_CREATE_STUB(CreateTranslateTransform, IDCompositionTranslateTransform)
DEVICE_CREATE_STUB(CreateScaleTransform, IDCompositionScaleTransform)
DEVICE_CREATE_STUB(CreateRotateTransform, IDCompositionRotateTransform)
DEVICE_CREATE_STUB(CreateSkewTransform, IDCompositionSkewTransform)
DEVICE_CREATE_STUB(CreateMatrixTransform, IDCompositionMatrixTransform)
DEVICE_CREATE_STUB(CreateTranslateTransform3D, IDCompositionTranslateTransform3D)
DEVICE_CREATE_STUB(CreateScaleTransform3D, IDCompositionScaleTransform3D)
DEVICE_CREATE_STUB(CreateRotateTransform3D, IDCompositionRotateTransform3D)
DEVICE_CREATE_STUB(CreateMatrixTransform3D, IDCompositionMatrixTransform3D)
DEVICE_CREATE_STUB(CreateEffectGroup, IDCompositionEffectGroup)
DEVICE_CREATE_STUB(CreateRectangleClip, IDCompositionRectangleClip)
DEVICE_CREATE_STUB(CreateAnimation, IDCompositionAnimation)

static HRESULT STDMETHODCALLTYPE device_CreateTransformGroup(IDCompositionDevice *iface,
        IDCompositionTransform **transforms, UINT count, IDCompositionTransform **group)
{
    FIXME("iface %p, transforms %p, count %u, group %p stub!\n", iface, transforms, count, group);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE device_CreateTransform3DGroup(IDCompositionDevice *iface,
        IDCompositionTransform3D **transforms, UINT count, IDCompositionTransform3D **group)
{
    FIXME("iface %p, transforms %p, count %u, group %p stub!\n", iface, transforms, count, group);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE device_CheckDeviceState(IDCompositionDevice *iface, BOOL *valid)
{
    TRACE("iface %p, valid %p.\n", iface, valid);

    if (!valid)
        return E_POINTER;
    *valid = TRUE;
    return S_OK;
}

static const IDCompositionDeviceVtbl device_vtbl =
{
    device_QueryInterface,
    device_AddRef,
    device_Release,
    device_Commit,
    device_WaitForCommitCompletion,
    device_GetFrameStatistics,
    device_CreateTargetForHwnd,
    device_CreateVisual,
    device_CreateSurface,
    device_CreateVirtualSurface,
    device_CreateSurfaceFromHandle,
    device_CreateSurfaceFromHwnd,
    device_CreateTranslateTransform,
    device_CreateScaleTransform,
    device_CreateRotateTransform,
    device_CreateSkewTransform,
    device_CreateMatrixTransform,
    device_CreateTransformGroup,
    device_CreateTranslateTransform3D,
    device_CreateScaleTransform3D,
    device_CreateRotateTransform3D,
    device_CreateMatrixTransform3D,
    device_CreateTransform3DGroup,
    device_CreateEffectGroup,
    device_CreateRectangleClip,
    device_CreateAnimation,
    device_CheckDeviceState,
};

HRESULT WINAPI DCompositionCreateDevice(IDXGIDevice *dxgi_device, REFIID iid, void **out)
{
    struct composition_device *device;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", dxgi_device, debugstr_guid(iid), out);

    if (!out)
        return E_POINTER;
    *out = NULL;
    if (!(device = calloc(1, sizeof(*device))))
        return E_OUTOFMEMORY;

    device->IDCompositionDevice_iface.lpVtbl = &device_vtbl;
    device->refcount = 1;
    InitializeCriticalSectionEx(&device->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    device->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": composition_device.cs");
    list_init(&device->targets);

    hr = IDCompositionDevice_QueryInterface(&device->IDCompositionDevice_iface, iid, out);
    IDCompositionDevice_Release(&device->IDCompositionDevice_iface);
    return hr;
}

HRESULT WINAPI DCompositionCreateDevice2(IUnknown *rendering_device, REFIID iid, void **device)
{
    FIXME("%p, %s, %p.\n", rendering_device, debugstr_guid(iid), device);

    return E_NOTIMPL;
}

HRESULT WINAPI DCompositionCreateDevice3(IUnknown *rendering_device, REFIID iid, void **device)
{
    FIXME("%p, %s, %p.\n", rendering_device, debugstr_guid(iid), device);

    return E_NOTIMPL;
}
