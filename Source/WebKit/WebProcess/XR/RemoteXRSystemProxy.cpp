/*
 * Copyright (C) 2025 Apple Inc. All rights reserved.
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

#include "config.h"
#include "RemoteXRSystemProxy.h"

#if ENABLE(WEBXR_IN_GPUP)

#include "GPUConnectionToWebProcessMessages.h"
#include "PlatformXRCoordinator.h"
#include "RemoteXRSystemMessages.h"
#include "RemoteXRSystemProxyMessages.h"
#include "WebPage.h"
#include "WebProcess.h"
#include "XRDeviceInfo.h"
#include <WebCore/Page.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/Settings.h>
#include <WebCore/XRCanvasConfiguration.h>
#include <wtf/Vector.h>
#include <wtf/TZoneMallocInlines.h>

using namespace PlatformXR;

namespace WebKit {

using namespace WebCore;

WTF_MAKE_TZONE_ALLOCATED_IMPL(RemoteXRSystemProxy);

Ref<RemoteXRSystemProxy> RemoteXRSystemProxy::create(WebPage& page)
{
    return RemoteXRSystemProxy::create(page, RunLoop::mainSingleton());
}

Ref<RemoteXRSystemProxy> RemoteXRSystemProxy::create(WebPage& page, SerialFunctionDispatcher& dispatcher)
{
    constexpr size_t connectionBufferSizeLog2 = 21;
    auto connectionPair = IPC::StreamClientConnection::create(connectionBufferSizeLog2, WebProcess::singleton().gpuProcessTimeoutDuration());
    if (!connectionPair) {
        // Since we need to return a Ref, we cannot return nullptr.
        // This should be handled by the caller checking if creation is possible beforehand.
        RELEASE_ASSERT_NOT_REACHED();
    }
    auto [clientConnection, serverConnectionHandle] = WTFMove(*connectionPair);
    Ref instance = adoptRef(*new RemoteXRSystemProxy(page, dispatcher));
    instance->initializeIPC(WTFMove(clientConnection), WTFMove(serverConnectionHandle));
    // TODO: We must wait until initialized, because at the moment we cannot receive IPC messages
    // during wait while in synchronous stream send. Should be fixed as part of https://bugs.webkit.org/show_bug.cgi?id=217211.
    instance->waitUntilInitialized();
    return instance;
}

RemoteXRSystemProxy::RemoteXRSystemProxy(WebPage& page, SerialFunctionDispatcher& dispatcher)
    : m_dispatcher(dispatcher)
    , m_identifier(page.identifier())
{
    // FIXME(djg): Is there another route for accessing the settings?
    m_webXREnabled = page.corePage() && page.corePage()->settings().webXREnabled();
}

RemoteXRSystemProxy::~RemoteXRSystemProxy()
{
    disconnectGpuProcessIfNeeded();
}

void RemoteXRSystemProxy::enumerateImmersiveXRDevices(CompletionHandler<void(const PlatformXR::Instance::DeviceList&)>&& completionHandler)
{
    if (m_lost) {
        completionHandler({ });
        return;
    }

    sendWithAsyncReply(Messages::RemoteXRSystem::EnumerateImmersiveXRDevices(), [this, weakThis = WeakPtr { *this }, completionHandler = WTFMove(completionHandler)](Vector<XRDeviceInfo>&& devicesInfos) mutable {
        if (!weakThis)
            return;

        PlatformXR::Instance::DeviceList devices;
        for (auto& deviceInfo : devicesInfos) {
            if (auto device = deviceByIdentifier(deviceInfo.identifier))
                devices.append(*device);
            else
                devices.append(XRDeviceProxy::create(WTFMove(deviceInfo), *this));
        }
        m_devices.swap(devices);
        completionHandler(m_devices);
    });
}

void RemoteXRSystemProxy::requestPermissionOnSessionFeatures(const WebCore::SecurityOriginData& securityOriginData,
     PlatformXR::SessionMode mode,
     const PlatformXR::Device::FeatureList& granted,
     const PlatformXR::Device::FeatureList& consentRequired,
     const PlatformXR::Device::FeatureList& consentOptional,
     const PlatformXR::Device::FeatureList& requiredFeaturesRequested,
     const PlatformXR::Device::FeatureList& optionalFeaturesRequested,
     CompletionHandler<void(std::optional<PlatformXR::Device::FeatureList>&&)>&& completionHandler)
{
    sendWithAsyncReply(Messages::RemoteXRSystem::RequestPermissionOnSessionFeatures(
        securityOriginData,
        mode,
        granted, 
        consentRequired,
        consentOptional,
        requiredFeaturesRequested,
        optionalFeaturesRequested),
        WTFMove(completionHandler));
}

void RemoteXRSystemProxy::initializeTrackingAndRendering(std::optional<WebCore::XRCanvasConfiguration>&& optionalInit)
{
    std::optional<WebCore::WebGPU::TextureFormat> colorFormat;
    std::optional<WebCore::WebGPU::TextureFormat> depthStencilFormat;
    if (optionalInit) {
        colorFormat = optionalInit->colorFormat;
        depthStencilFormat = optionalInit->depthStencilFormat;
    }
    send(Messages::RemoteXRSystem::InitializeTrackingAndRendering(WTFMove(colorFormat), WTFMove(depthStencilFormat)));
}

void RemoteXRSystemProxy::shutDownTrackingAndRendering()
{
    send(Messages::RemoteXRSystem::ShutDownTrackingAndRendering());
}

void RemoteXRSystemProxy::didCompleteShutdownTriggeredBySystem()
{
    // FIXME(djg): This should be a message between UIP and GPUP
    send(Messages::RemoteXRSystem::DidCompleteShutdownTriggeredBySystem());
}

void RemoteXRSystemProxy::requestFrame(std::optional<PlatformXR::RequestData>&& requestData, PlatformXR::Device::RequestFrameCallback&& callback)
{
    sendWithAsyncReply(Messages::RemoteXRSystem::RequestFrame(WTFMove(requestData)), WTFMove(callback));
}

std::optional<PlatformXR::LayerHandle> RemoteXRSystemProxy::createLayerProjection(uint32_t, uint32_t, bool)
{
#if USE(OPENXR)
#else
#endif
    // FIXME(djg):
    // return PlatformXRCoordinator::defaultLayerHandle();
    return { };
}

#if USE(OPENXR)
void RemoteXRSystemProxy::submitFrame(Vector<PlatformXR::Device::Layer>&&)
{}
#else
void RemoteXRSystemProxy::submitFrame()
{
    send(Messages::RemoteXRSystem::SubmitFrame());
}
#endif

void RemoteXRSystemProxy::initializeIPC(Ref<IPC::StreamClientConnection>&& streamConnection, IPC::StreamServerConnection::Handle&& serverHandle)
{
    m_streamConnection = WTFMove(streamConnection);
    protectedStreamConnection()->open(*this, *this);
    callOnMainRunLoopAndWait([&]() {
        Ref gpuProcessConnection = WebProcess::singleton().ensureGPUProcessConnection();
        gpuProcessConnection->createXRSystem(m_identifier, WTFMove(serverHandle));
        m_gpuProcessConnection = gpuProcessConnection.get();
    });
}

// IPC::Connection::Client
void RemoteXRSystemProxy::didClose(IPC::Connection&)
{
    ASSERT(m_streamConnection);
    abandonGPUProcess();
}

// Messages to be received.
void RemoteXRSystemProxy::wasCreated(bool didSucceed, IPC::Semaphore&& wakeUpSemaphore, IPC::Semaphore&& clientWaitSemaphore)
{
    ASSERT(!m_didInitialize);
    m_didInitialize = true;
    if (didSucceed)
        protectedStreamConnection()->setSemaphores(WTFMove(wakeUpSemaphore), WTFMove(clientWaitSemaphore));
    else
        abandonGPUProcess();
}

void RemoteXRSystemProxy::sessionDidEnd(XRDeviceIdentifier identifier)
{
    RELEASE_ASSERT(m_webXREnabled);

    if (auto device = deviceByIdentifier(identifier))
        device->sessionDidEnd();
}

void RemoteXRSystemProxy::sessionDidUpdateVisibilityState(XRDeviceIdentifier identifier, PlatformXR::VisibilityState state)
{
    RELEASE_ASSERT(m_webXREnabled);

    if (auto device = deviceByIdentifier(identifier))
        device->updateSessionVisibilityState(state);
}

void RemoteXRSystemProxy::waitUntilInitialized()
{
    if (m_didInitialize)
        return;
    if (protectedStreamConnection()->waitForAndDispatchImmediately<Messages::RemoteXRSystemProxy::WasCreated>(m_identifier) == IPC::Error::NoError)
        return;
    abandonGPUProcess();
}

void RemoteXRSystemProxy::abandonGPUProcess()
{
    protectedStreamConnection()->invalidate();
    m_lost = true;
}

void RemoteXRSystemProxy::disconnectGpuProcessIfNeeded()
{
    if (m_lost)
        return;
    protectedStreamConnection()->invalidate();
    // FIXME: deallocate m_streamConnection once the children work without the connection.
    ensureOnMainRunLoop([identifier = m_identifier, weakGPUProcessConnection = WTFMove(m_gpuProcessConnection)]() {
        RefPtr gpuProcessConnection = weakGPUProcessConnection.get();
        if (!gpuProcessConnection)
            return;
        gpuProcessConnection->releaseXRSystem(identifier);
    });

}

// SerialFunctionDispatcher
void RemoteXRSystemProxy::dispatch(Function<void()>&& function)
{
    if (RefPtr dispatcher = m_dispatcher.get())
        dispatcher->dispatch(WTFMove(function));
}

bool RemoteXRSystemProxy::isCurrent() const
{
    RefPtr dispatcher = m_dispatcher.get();
    return dispatcher && dispatcher->isCurrent();
}

RefPtr<XRDeviceProxy> RemoteXRSystemProxy::deviceByIdentifier(XRDeviceIdentifier identifier)
{
    for (auto& device : m_devices) {
        auto* deviceProxy = static_cast<XRDeviceProxy*>(device.ptr());
        if (deviceProxy->identifier() == identifier)
            return deviceProxy;
    }

    return nullptr;
}

} // namespace WebKit

#endif // ENABLE(WEBXR_IN_GPUP)
