/*
 * Copyright (C) 2021-2025 Apple Inc. All rights reserved.
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

#include "MessageReceiver.h"
#include "PlatformXRCoordinator.h"
#include "ProcessThrottler.h"
#include "StreamConnectionWorkQueue.h"
#include <WebCore/PageIdentifier.h>
#include <WebCore/PlatformXR.h>
#include <WebCore/SecurityOriginData.h>
#include <wtf/HashMap.h>
#include <wtf/TZoneMalloc.h>
#include <wtf/WeakPtr.h>

namespace WebCore {
class SecurityOriginData;
struct XRCanvasConfiguration;

namespace WebGPU {
enum class TextureFormat : uint8_t;
}
}

namespace WebKit {

class GPUConnectionToWebProcess;
struct SharedPreferencesFroWebProcess;

class RemoteXRSystem : public CanMakeWeakPtr<RemoteXRSystem>, public IPC::StreamServerConnection::Client {
    WTF_MAKE_TZONE_ALLOCATED(RemoteXRSystem);
    WTF_OVERRIDE_DELETE_FOR_CHECKED_PTR(RemoteXRSystem);
public:
    static Ref<RemoteXRSystem> create(GPUConnectionToWebProcess&, WebCore::PageIdentifier, Ref<IPC::StreamServerConnection>&&);
    virtual ~RemoteXRSystem();
    void stopListeningForIPC();

    std::optional<SharedPreferencesForWebProcess> sharedPreferencesForWebProcess() const;

    // IPC::StreamMessageReceiver overrides.
    void didReceiveStreamMessage(IPC::StreamServerConnection&, IPC::Decoder&) final;

    // Runs Function in RemoteXRSystem task queue
    void dispatch(Function<void()>&&);

    IPC::StreamServerConnection& streamConnection() const { return m_streamConnection.get(); }
    GPUConnectionToWebProcess& gpuConnectionToWebProcess() { return m_gpuConnectionToWebProcess.get(); }
    IPC::StreamConnectionWorkQueue& workQueue() const { return m_workQueue; }

    WebCore::PageIdentifier identifier() { return m_pageIdentifier; }

private:
    RemoteXRSystem(GPUConnectionToWebProcess&, WebCore::PageIdentifier, Ref<IPC::StreamServerConnection>&&);
    void startListeningForIPC();
    void workQueueInitialize();
    void workQueueUninitialize();
    template<typename T>
    IPC::Error send(T&& message) const
    {
        return m_streamConnection->send(std::forward<T>(message), m_pageIdentifier);
    }

    // IPC::StreamServerConnection::Client overrides.
    void didReceiveInvalidMessage(IPC::StreamServerConnection&, IPC::MessageName, const Vector<uint32_t>&) final;

    // Messages to be received.
    void enumerateImmersiveXRDevices(CompletionHandler<void(Vector<XRDeviceInfo>&&)>&&); // -> (Vector<WebKit::XRDeviceInfo> devicesInfos)
    void requestPermissionOnSessionFeatures(const WebCore::SecurityOriginData&, PlatformXR::SessionMode, const PlatformXR::Device::FeatureList&, const PlatformXR::Device::FeatureList&, const PlatformXR::Device::FeatureList&, const PlatformXR::Device::FeatureList&, const PlatformXR::Device::FeatureList&, CompletionHandler<void(std::optional<PlatformXR::Device::FeatureList>&&)>&&);
    void initializeTrackingAndRendering(std::optional<WebCore::WebGPU::TextureFormat>, std::optional<WebCore::WebGPU::TextureFormat>);
    void shutDownTrackingAndRendering();
    void requestFrame(std::optional<PlatformXR::RequestData>&&, CompletionHandler<void(PlatformXR::FrameData&&)>&&); // -> (struct PlatformXR::FrameData frameData)
    void submitFrame();
    void didCompleteShutdownTriggeredBySystem();

private:
    const Ref<IPC::StreamConnectionWorkQueue> m_workQueue;
    const Ref<IPC::StreamServerConnection> m_streamConnection;
    const Ref<GPUConnectionToWebProcess> m_gpuConnectionToWebProcess;
    WebCore::PageIdentifier m_pageIdentifier;
};

} // namespace WebKit

#endif // ENABLE(WEBXR_IN_GPUP)
