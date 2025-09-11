/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(WEBXR_IN_GPUP)

#include "GPUProcessConnection.h"
#include "StreamClientConnection.h"
#include "XRDeviceIdentifier.h"
#include "XRDeviceProxy.h"
#include <WebCore/PlatformXR.h>
#include <wtf/TZoneMalloc.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace WebCore {
class SecurityOriginData;
}

namespace WebKit {

class WebPage;

class RemoteXRSystemProxy final : public IPC::Connection::Client, public ThreadSafeRefCounted<RemoteXRSystemProxy>, SerialFunctionDispatcher {
    WTF_MAKE_TZONE_ALLOCATED(RemoteXRSystemProxy);
    WTF_OVERRIDE_DELETE_FOR_CHECKED_PTR(RemoteXRSystemProxy);
public:
    static Ref<RemoteXRSystemProxy> create(WebPage&);
    static Ref<RemoteXRSystemProxy> create(WebPage&, SerialFunctionDispatcher&);

    virtual ~RemoteXRSystemProxy();

    IPC::StreamClientConnection& streamClientConnection() { return *m_streamConnection; }
    Ref<IPC::StreamClientConnection> protectedStreamClientConnection() { return *m_streamConnection; }

    void ref() const final { return ThreadSafeRefCounted<RemoteXRSystemProxy>::ref(); }
    void deref() const final { return ThreadSafeRefCounted<RemoteXRSystemProxy>::deref(); }

    void enumerateImmersiveXRDevices(CompletionHandler<void(const PlatformXR::Instance::DeviceList&)>&&);
    void requestPermissionOnSessionFeatures(const WebCore::SecurityOriginData&, PlatformXR::SessionMode, const PlatformXR::Device::FeatureList& /* granted */, const PlatformXR::Device::FeatureList& /* consentRequired */, const PlatformXR::Device::FeatureList& /* consentOptional */, const PlatformXR::Device::FeatureList& /* requiredFeaturesRequested */, const PlatformXR::Device::FeatureList& /* optionalFeaturesRequested */,  CompletionHandler<void(std::optional<PlatformXR::Device::FeatureList>&&)>&&);
    void initializeTrackingAndRendering(std::optional<WebCore::XRCanvasConfiguration>&&);
    void shutDownTrackingAndRendering();
    void didCompleteShutdownTriggeredBySystem();
    void requestFrame(std::optional<PlatformXR::RequestData>&&, PlatformXR::Device::RequestFrameCallback&&);
    std::optional<PlatformXR::LayerHandle> createLayerProjection(uint32_t, uint32_t, bool);
#if USE(OPENXR)
    void submitFrame(Vector<PlatformXR::Device::Layer>&&);
#else
    void submitFrame();
#endif

    WebCore::PageIdentifier identifier() const { return m_identifier; }

private:
    RemoteXRSystemProxy(WebPage&, SerialFunctionDispatcher&);
    void initializeIPC(Ref<IPC::StreamClientConnection>&&, IPC::StreamServerConnection::Handle&&);

    RemoteXRSystemProxy(const RemoteXRSystemProxy&) = delete;
    RemoteXRSystemProxy(RemoteXRSystemProxy&&) = delete;
    RemoteXRSystemProxy& operator=(const RemoteXRSystemProxy&) = delete;
    RemoteXRSystemProxy& operator=(RemoteXRSystemProxy&&) = delete;

    // IPC::Connection::Client
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) final;
    void didClose(IPC::Connection&) final;
    void didReceiveInvalidMessage(IPC::Connection&, IPC::MessageName, const Vector<uint32_t>&) final { }

    // Messages to be received.
    void wasCreated(bool, IPC::Semaphore&&, IPC::Semaphore&&);
    void sessionDidEnd(XRDeviceIdentifier);
    void sessionDidUpdateVisibilityState(XRDeviceIdentifier, PlatformXR::VisibilityState);

    void waitUntilInitialized();

    template<typename T>
    IPC::Error send(T&& message)
    {
        return protectedStreamClientConnection()->send(std::forward<T>(message), identifier());
    }
    template<typename T, typename C>
    std::optional<IPC::AsyncReplyID> sendWithAsyncReply(T&& message, C&& completionHandler)
    {
        return protectedStreamClientConnection()->sendWithAsyncReply(std::forward<T>(message), std::forward<C>(completionHandler), identifier());
    }

    void abandonGPUProcess();
    void disconnectGpuProcessIfNeeded();

    // SerialFunctionDispatcher
    void dispatch(Function<void()>&&) final;
    bool isCurrent() const final;

    RefPtr<IPC::StreamClientConnection> protectedStreamConnection() const { return m_streamConnection; }

    RefPtr<XRDeviceProxy> deviceByIdentifier(XRDeviceIdentifier);
    bool webXREnabled() const;

private:
    ThreadSafeWeakPtr<SerialFunctionDispatcher> m_dispatcher;
    WeakPtr<GPUProcessConnection> m_gpuProcessConnection;
    RefPtr<IPC::StreamClientConnection> m_streamConnection;
    WebCore::PageIdentifier m_identifier;

    PlatformXR::Instance::DeviceList m_devices;

    bool m_didInitialize { false };
    bool m_lost { false };
    bool m_webXREnabled { false };
};

} // namespace WebKit

#endif // ENABLE(WEBXR_IN_GPUP)
