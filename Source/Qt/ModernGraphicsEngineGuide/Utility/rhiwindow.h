#pragma once

#include <memory>

#include <QFile>
#include <QOffscreenSurface>
#include <QPainter>
#include <QPlatformSurfaceEvent>
#include <QWindow>
#include <QtGui>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>

class RhiWindow : public QWindow {
public:
    explicit RhiWindow(QRhi::Implementation graphicsApi) : m_graphicsApi(graphicsApi)
    {
        switch (graphicsApi)
        {
            case QRhi::OpenGLES2: setSurfaceType(OpenGLSurface); break;
            case QRhi::Vulkan: setSurfaceType(VulkanSurface); break;
            case QRhi::D3D11:
            case QRhi::D3D12: setSurfaceType(Direct3DSurface); break;
            case QRhi::Metal: setSurfaceType(MetalSurface); break;
            case QRhi::Null: break;  // RasterSurface
        }
    }

    QString graphicsApiName() const
    {
        switch (m_graphicsApi)
        {
            case QRhi::Null: return QLatin1String("Null (no output)");
            case QRhi::OpenGLES2: return QLatin1String("OpenGL");
            case QRhi::Vulkan: return QLatin1String("Vulkan");
            case QRhi::D3D11: return QLatin1String("Direct3D 11");
            case QRhi::D3D12: return QLatin1String("Direct3D 12");
            case QRhi::Metal: return QLatin1String("Metal");
        }
        return {};
    }

    void releaseSwapChain()
    {
        if (m_hasSwapChain)
        {
            m_hasSwapChain = false;
            m_sc->destroy();
        }
    }

protected:
    virtual void customInit()   = 0;
    virtual void customRender() = 0;

    // destruction order matters to a certain degree:
    // the fallbackSurface must outlive the rhi,
    // the rhi must outlive all other resources.
    // The resources need no special order when destroying.

#if QT_CONFIG(opengl)
    std::unique_ptr<QOffscreenSurface> m_fallbackSurface;
#endif
    std::unique_ptr<QRhi> m_rhi;

    // swapchain-data
    std::unique_ptr<QRhiSwapChain>            m_sc;
    std::unique_ptr<QRhiRenderBuffer>         m_ds;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rp;

    bool       m_hasSwapChain = false;
    QMatrix4x4 m_viewProjection;

private:
    void init()
    {
        if (m_graphicsApi == QRhi::Null)
        {
            QRhiNullInitParams params;
            m_rhi.reset(QRhi::create(QRhi::Null, &params));
        }

#if QT_CONFIG(opengl)
        if (m_graphicsApi == QRhi::OpenGLES2)
        {
            m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
            QRhiGles2InitParams params;
            params.fallbackSurface = m_fallbackSurface.get();
            params.window          = this;
            m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
        }
#endif

#if QT_CONFIG(vulkan)
        if (m_graphicsApi == QRhi::Vulkan)
        {
            QRhiVulkanInitParams params;
            params.inst   = vulkanInstance();
            params.window = this;
            m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));
        }
#endif

#ifdef Q_OS_WIN
        if (m_graphicsApi == QRhi::D3D11)
        {
            QRhiD3D11InitParams params;
            // Enable the debug layer, if available. This is optional
            // and should be avoided in production builds.
            params.enableDebugLayer = true;
            m_rhi.reset(QRhi::create(QRhi::D3D11, &params));
        }
        else if (m_graphicsApi == QRhi::D3D12)
        {
            QRhiD3D12InitParams params;
            // Enable the debug layer, if available. This is optional
            // and should be avoided in production builds.
            params.enableDebugLayer = true;
            m_rhi.reset(QRhi::create(QRhi::D3D12, &params));
        }
#endif

#if QT_CONFIG(metal)
        if (m_graphicsApi == QRhi::Metal)
        {
            QRhiMetalInitParams params;
            m_rhi.reset(QRhi::create(QRhi::Metal, &params));
        }
#endif

        if (!m_rhi) { qFatal("Failed to create RHI backend"); }

        // swapchain-init
        m_sc.reset(m_rhi->newSwapChain());
        m_ds.reset(m_rhi->newRenderBuffer(
            QRhiRenderBuffer::DepthStencil,
            QSize(),  // no need to set the size here, due to UsedWithSwapChainOnly
            1, QRhiRenderBuffer::UsedWithSwapChainOnly));
        m_sc->setWindow(this);
        m_sc->setDepthStencil(m_ds.get());
        m_rp.reset(m_sc->newCompatibleRenderPassDescriptor());
        m_sc->setRenderPassDescriptor(m_rp.get());

        customInit();
    }

    void resizeSwapChain()
    {
        m_hasSwapChain = m_sc->createOrResize();  // also handles m_ds

        const QSize outputSize = m_sc->currentPixelSize();
        m_viewProjection       = m_rhi->clipSpaceCorrMatrix();
        m_viewProjection.perspective(45.0f, outputSize.width() / (float)outputSize.height(), 0.01f,
                                     1000.0f);
        m_viewProjection.translate(0, 0, -4);
    }

    void render()
    {
        // render-precheck
        if (!m_hasSwapChain || m_notExposed) { return; }

        // If the window got resized or newly exposed, resize the swapchain. (the
        // newly-exposed case is not actually required by some platforms, but is
        // here for robustness and portability)
        //
        // This (exposeEvent + the logic here) is the only safe way to perform
        // resize handling. Note the usage of the RHI's surfacePixelSize(), and
        // never QWindow::size(). (the two may or may not be the same under the hood,
        // depending on the backend and platform)
        //
        if (m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed)
        {
            resizeSwapChain();
            if (!m_hasSwapChain) { return; }
            m_newlyExposed = false;
        }

        // beginframe
        QRhi::FrameOpResult result = m_rhi->beginFrame(m_sc.get());
        if (result == QRhi::FrameOpSwapChainOutOfDate)
        {
            resizeSwapChain();
            if (!m_hasSwapChain) { return; }
            result = m_rhi->beginFrame(m_sc.get());
        }
        if (result != QRhi::FrameOpSuccess)
        {
            qWarning("beginFrame failed with %d, will retry", result);
            requestUpdate();
            return;
        }

        customRender();

        // request-update
        m_rhi->endFrame(m_sc.get());

        // Always request the next frame via requestUpdate(). On some platforms this is backed
        // by a platform-specific solution, e.g. CVDisplayLink on macOS, which is potentially
        // more efficient than a timer, queued metacalls, etc.
        requestUpdate();
    }

    void exposeEvent(QExposeEvent*) override
    {
        // initialize and start rendering when the window becomes usable for graphics purposes
        if (isExposed() && !m_initialized)
        {
            init();
            resizeSwapChain();
            m_initialized = true;
        }

        const QSize surfaceSize = m_hasSwapChain ? m_sc->surfacePixelSize() : QSize();

        // stop pushing frames when not exposed (or size is 0)
        if ((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_initialized &&
            !m_notExposed)
        {
            m_notExposed = true;
        }

        // Continue when exposed again and the surface has a valid size. Note that
        // surfaceSize can be (0, 0) even though size() reports a valid one, hence
        // trusting surfacePixelSize() and not QWindow.
        if (isExposed() && m_initialized && m_notExposed && !surfaceSize.isEmpty())
        {
            m_notExposed   = false;
            m_newlyExposed = true;
        }

        // always render a frame on exposeEvent() (when exposed) in order to update
        // immediately on window resize.
        if (isExposed() && !surfaceSize.isEmpty()) { render(); }
    }

    bool event(QEvent* event) override
    {
        switch (event->type())
        {
            case QEvent::UpdateRequest: render(); break;

            case QEvent::PlatformSurface:
                // this is the proper time to tear down the swapchain
                // (while the native window and surface are still around)
                if (dynamic_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() ==
                    QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
                {
                    releaseSwapChain();
                }
                break;

            default: break;
        }

        return QWindow::event(event);
    }

    QRhi::Implementation m_graphicsApi;
    bool                 m_initialized  = false;
    bool                 m_notExposed   = false;
    bool                 m_newlyExposed = false;
};