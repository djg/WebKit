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

#include "config.h"
#include "RemoteXRSystem.h"

#if ENABLE(WEBXR_IN_GPUP)

#include "GPUConnectionToWebProcess.h"
#include "GPUProcessProxy.h"
#include "MessageSenderInlines.h"
#include "RemoteXRSystemMessages.h"
#include "RemoteXRSystemProxyMessages.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <WebCore/GPUTextureFormat.h>
#include <WebCore/SecurityOriginData.h>
#include <WebCore/XRCanvasConfiguration.h>
#include <wtf/CheckedArithmetic.h>
#include <wtf/RunLoop.h>
#include <wtf/StdLibExtras.h>
#include <wtf/SystemTracing.h>
#include <wtf/TZoneMallocInlines.h>

#define MESSAGE_CHECK(assertion, connection) MESSAGE_CHECK_WITH_MESSAGE_BASE(assertion, m_streamConnection, connection);

namespace WebKit {
using namespace WebCore;

WTF_MAKE_TZONE_ALLOCATED_IMPL(RemoteXRSystem);

Ref<RemoteXRSystem> RemoteXRSystem::create(GPUConnectionToWebProcess& gpuConnectionToWebProcess, WebCore::PageIdentifier pageIdentifier, Ref<IPC::StreamServerConnection>&& streamConnection)
{
    auto instance = adoptRef(*new RemoteXRSystem(gpuConnectionToWebProcess, pageIdentifier, WTFMove(streamConnection)));
    instance->startListeningForIPC();
    return instance;
}

// TODO: Refactor constructor to work without WebPageProxy in GPU process context
RemoteXRSystem::RemoteXRSystem(GPUConnectionToWebProcess& gpuConnectionToWebProcess, WebCore::PageIdentifier pageIdentifier, Ref<IPC::StreamServerConnection>&& streamConnection)
    : m_workQueue(IPC::StreamConnectionWorkQueue::create("RemoteXRSystem work queue"_s))
    , m_streamConnection(WTFMove(streamConnection))
    , m_gpuConnectionToWebProcess(gpuConnectionToWebProcess)
    , m_pageIdentifier(pageIdentifier)
{
    ASSERT(RunLoop::isMain());
    // Stub constructor - needs refactoring for GPU process context
}

RemoteXRSystem::~RemoteXRSystem() = default;

void RemoteXRSystem::startListeningForIPC()
{
    dispatch([protectedThis = Ref { *this }] {
        protectedThis->workQueueInitialize();
    });
}

void RemoteXRSystem::stopListeningForIPC()
{
    m_workQueue->stopAndWaitForCompletion([protectedThis = Ref { *this }] {
        protectedThis->workQueueUninitialize();
    });
}

std::optional<SharedPreferencesForWebProcess> RemoteXRSystem::sharedPreferencesForWebProcess() const
{
    return m_gpuConnectionToWebProcess->sharedPreferencesForWebProcess();
}

void RemoteXRSystem::workQueueInitialize()
{
    assertIsCurrent(workQueue());
    m_streamConnection->open(*this, m_workQueue.get());
    m_streamConnection->startReceivingMessages(*this, Messages::RemoteXRSystem::messageReceiverName(), m_pageIdentifier.toUInt64());
    send(Messages::RemoteXRSystemProxy::WasCreated(true, workQueue().wakeUpSemaphore(), m_streamConnection->clientWaitSemaphore()));
}

void RemoteXRSystem::workQueueUninitialize()
{
    assertIsCurrent(workQueue());

    Ref streamConnection = m_streamConnection;
    streamConnection->stopReceivingMessages(Messages::RemoteXRSystem::messageReceiverName(), m_pageIdentifier.toUInt64());
    streamConnection->invalidate();
}

void RemoteXRSystem::didReceiveInvalidMessage(IPC::StreamServerConnection&, IPC::MessageName messageName, const Vector<uint32_t>&)
{
    RELEASE_LOG_FAULT(IPC, "Received an invalid message '%" PUBLIC_LOG_STRING "' from WebContent process %" PRIu64 ", requesting for it to be terminated.", description(messageName).characters(), m_gpuConnectionToWebProcess->webProcessIdentifier().toUInt64());
    callOnMainRunLoop([gpuConnectionToWebProcess = m_gpuConnectionToWebProcess] {
        gpuConnectionToWebProcess->terminateWebProcess();
    });
}

void RemoteXRSystem::dispatch(Function<void()>&& task)
{
    m_workQueue->dispatch(WTFMove(task));
}

void RemoteXRSystem::enumerateImmersiveXRDevices(CompletionHandler<void(Vector<XRDeviceInfo>&&)>&& completionHandler)
{
    // TODO: Implement device enumeration
    completionHandler({ });
}

void RemoteXRSystem::requestPermissionOnSessionFeatures(const WebCore::SecurityOriginData&, PlatformXR::SessionMode, const PlatformXR::Device::FeatureList&, const PlatformXR::Device::FeatureList&, const PlatformXR::Device::FeatureList&, const PlatformXR::Device::FeatureList&, const PlatformXR::Device::FeatureList&, CompletionHandler<void(std::optional<PlatformXR::Device::FeatureList>&&)>&& completionHandler)
{
    // TODO: Implement permission request handling
    completionHandler(std::nullopt);
}

void RemoteXRSystem::initializeTrackingAndRendering(std::optional<WebCore::WebGPU::TextureFormat>, std::optional<WebCore::WebGPU::TextureFormat>)
{
    // TODO: Implement tracking and rendering initialization
}

void RemoteXRSystem::shutDownTrackingAndRendering()
{
    // TODO: Implement tracking and rendering shutdown
}

void RemoteXRSystem::requestFrame(std::optional<PlatformXR::RequestData>&&, CompletionHandler<void(PlatformXR::FrameData&&)>&& completionHandler)
{
    // TODO: Implement frame request handling
    completionHandler({ });
}

void RemoteXRSystem::submitFrame()
{
    // TODO: Implement frame submission
}

void RemoteXRSystem::didCompleteShutdownTriggeredBySystem()
{}

} // namespace WebKit

#undef MESSAGE_CHECK

#endif // ENABLE(WEBXR_IN_GPUP)
