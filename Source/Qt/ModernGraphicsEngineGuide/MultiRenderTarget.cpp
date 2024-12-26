// c
#include <array>

// 3rdparty
#include "QDateTime"
#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>
#include <QtWidgets>
#include <qdatetime.h>
#include <qerrormessage.h>
#include <qlogging.h>
#include <qmatrix4x4.h>
#include <qobject.h>
#include <rhi/qrhi.h>
#include <vulkan/vulkan_core.h>

// users
#include "Render/RHI/QRhiHelper.h"
#include "Render/RHI/QRhiWindow.h"
#include "Render/RenderGraph/Painter/TexturePainter.h"
#include "Utils/QRhiCamera.h"

#define CHECK(...) check(__VA_ARGS__, __FILE__, __LINE__)
template <typename T>
constexpr auto check(T&& condition, const char* file = "", int line = 0) -> bool
{
    if (!(condition))
    {
        qDebug() << "failed at " << file << ":" << line;
        return false;
    }
    return true;
}

static const std::array<float, 6> VertexData = {
    // position(xy)
    0.0f,  0.5f,   //
    -0.5f, -0.5f,  //
    0.5f,  -0.5f,  //
};

static const QString vertexShader1 = R"(
#version 460
layout(location = 0) in vec2 position;
out gl_PerVertex { vec4 gl_Position; };

void main(){
	gl_Position = vec4(position, 0.0f ,1.0f);
}
)";

static const QString fragmentShader1 = R"(
#version 460
layout(location = 0) out vec4 fragColor0;		//输出到颜色附件0
layout(location = 1) out vec4 fragColor1;		//输出到颜色附件1

void main(){
    fragColor0 = vec4(1.0f,0.0f,0.0f,1.0f);
	fragColor1 = vec4(0.0f,0.0f,1.0f,1.0f);
}
)";

static const QString vertexShader2 = R"(
#version 460
layout (location = 0) out vec2 vUV;
out gl_PerVertex { vec4 gl_Position; };

void main(){
    vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0f - 1.0f, 0.0f, 1.0f);
	%1
}
)";

static const QString fragmentShader2 = R"(
#version 460
layout (binding = 0) uniform sampler2D uSamplerColor;
layout (location = 0) in vec2 vUV;
layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = vec4(texture(uSamplerColor, vUV).rgb,1.0f);
}
)";

class MRTWindow : public QRhiWindow {
public:
    explicit MRTWindow(QRhiHelper::InitParams initParams) : QRhiWindow(initParams)
    {
        mSigInit.request();
    }

private:
    QRhiSignal mSigInit;
    QRhiSignal mSigSubmit;

    QScopedPointer<QRhiBuffer>                 mVertexBuffer;
    QScopedPointer<QRhiShaderResourceBindings> mShaderBindings;
    QScopedPointer<QRhiGraphicsPipeline>       mPipeline;

    QScopedPointer<QRhiTexture>      mColorAttachment0;
    QScopedPointer<QRhiTexture>      mColorAttachment1;
    QScopedPointer<QRhiRenderBuffer> mDepthStencilBuffer;

    QScopedPointer<QRhiTextureRenderTarget>  mRenderTarget;
    QScopedPointer<QRhiRenderPassDescriptor> mRenderPassDesc;

    QScopedPointer<QRhiSampler>                mPaintSampler;
    QScopedPointer<QRhiShaderResourceBindings> mPaintShaderBindings;
    QScopedPointer<QRhiGraphicsPipeline>       mPaintPipeline;

    void keyPressEvent(QKeyEvent* event) override
    {
        QRhiWindow::keyPressEvent(event);
        if (event->key() == Qt::Key_Escape) { close(); }
    }

    void initRhiResource(QRhiRenderTarget* currentRenderTarget)
    {
        // render target
        mColorAttachment0.reset(mRhi->newTexture(
            QRhiTexture::RGBA8, QSize(100, 100), 1,
            QRhiTexture::Flag::RenderTarget | QRhiTexture::UsedAsTransferSource));  // 创建颜色附件0
        CHECK(mColorAttachment0->create());
        mColorAttachment1.reset(mRhi->newTexture(
            QRhiTexture::RGBA8, QSize(100, 100), 1,
            QRhiTexture::Flag::RenderTarget | QRhiTexture::UsedAsTransferSource));  // 创建颜色附件1
        CHECK(mColorAttachment1->create());
        mDepthStencilBuffer.reset(mRhi->newRenderBuffer(
            QRhiRenderBuffer::Type::DepthStencil, QSize(100, 100), 1, QRhiRenderBuffer::Flag(),
            QRhiTexture::Format::D24S8));  // 创建深度（24位）模版（8位）附件
        CHECK(mDepthStencilBuffer->create());

        QRhiTextureRenderTargetDescription rtDesc;
        rtDesc.setColorAttachments({mColorAttachment0.get(), mColorAttachment1.get()});
        rtDesc.setDepthStencilBuffer(mDepthStencilBuffer.get());
        mRenderTarget.reset(mRhi->newTextureRenderTarget(rtDesc));

        // 根据RenderTarget的结构来创建RenderPass描述，在使用GraphicsPipeline时，必须指定RenderPassDesc
        // 这是因为流水线在创建时就需要明确它被用于何种结构的RenderPass，这里的结构指的是：RenderTarget的附件数量和格式
        mRenderPassDesc.reset(mRenderTarget->newCompatibleRenderPassDescriptor());
        mRenderTarget->setRenderPassDescriptor(mRenderPassDesc.get());

        CHECK(mRenderTarget->create());

        // vertex buffer
        mVertexBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(VertexData)));
        CHECK(mVertexBuffer->create());

        // descriptor set
        mShaderBindings.reset(mRhi->newShaderResourceBindings());
        CHECK(mShaderBindings->create());

        // 1st pipeline
        mPipeline.reset(mRhi->newGraphicsPipeline());

        QRhiGraphicsPipeline::TargetBlend targetBlend;
        targetBlend.enable = false;
        mPipeline->setTargetBlends({targetBlend, targetBlend});  // 有两个Attachment
        mPipeline->setSampleCount(mSwapChain->sampleCount());
        mPipeline->setDepthTest(false);
        mPipeline->setDepthOp(QRhiGraphicsPipeline::Always);
        mPipeline->setDepthWrite(false);

        // shader
        QShader vs1 =
            QRhiHelper::newShaderFromCode(QShader::VertexStage, vertexShader1.toLocal8Bit());
        QShader fs1 =
            QRhiHelper::newShaderFromCode(QShader::FragmentStage, fragmentShader1.toLocal8Bit());
        Q_ASSERT(fs1.isValid());
        Q_ASSERT(vs1.isValid());
        mPipeline->setShaderStages({
            QRhiShaderStage{QRhiShaderStage::Vertex, vs1},
            QRhiShaderStage{QRhiShaderStage::Fragment, fs1},
        });

        // vertex input
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            QRhiVertexInputBinding{2 * sizeof(float)},
        });
        inputLayout.setAttributes({
            QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float2, 0},
        });

        mPipeline->setVertexInputLayout(inputLayout);
        mPipeline->setShaderResourceBindings(mShaderBindings.get());
        mPipeline->setRenderPassDescriptor(mRenderPassDesc.get());
        CHECK(mPipeline->create());

        // ---------------------------------------------------------------------------
        // 2nd pipeline
        mPaintPipeline.reset(mRhi->newGraphicsPipeline());
        QRhiGraphicsPipeline::TargetBlend blendState{
            .enable   = true,
            .srcColor = QRhiGraphicsPipeline::One,
            .dstColor = QRhiGraphicsPipeline::One,
            .srcAlpha = QRhiGraphicsPipeline::One,
            .dstAlpha = QRhiGraphicsPipeline::One,
        };
        mPaintPipeline->setTargetBlends({blendState});
        mPaintPipeline->setSampleCount(currentRenderTarget->sampleCount());
        mPaintPipeline->setDepthTest(false);

        // shader
        QShader vs2 = QRhiHelper::newShaderFromCode(
            QShader::VertexStage,
            vertexShader2.arg(mRhi->isYUpInNDC() ? "	vUV.y = 1 - vUV.y;" : "").toLocal8Bit());
        QShader fs2 =
            QRhiHelper::newShaderFromCode(QShader::FragmentStage, fragmentShader2.toLocal8Bit());
        Q_ASSERT(fs2.isValid());
        Q_ASSERT(vs2.isValid());
        mPaintPipeline->setShaderStages({
            {QRhiShaderStage::Vertex, vs2},
            {QRhiShaderStage::Fragment, fs2},
        });

        // sampler
        mPaintSampler.reset(mRhi->newSampler(
            QRhiSampler::Filter::Linear, QRhiSampler::Filter::Linear, QRhiSampler::Filter::None,
            QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat));
        CHECK(mPaintSampler->create());

        // 描述符集
        mPaintShaderBindings.reset(mRhi->newShaderResourceBindings());
        mPaintShaderBindings->setBindings({
            QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage,
                                                      mColorAttachment0.get(), mPaintSampler.get()),
        });
        CHECK(mPaintShaderBindings->create());
        mPaintPipeline->setShaderResourceBindings(mPaintShaderBindings.get());
        mPaintPipeline->setRenderPassDescriptor(currentRenderTarget->renderPassDescriptor());
        CHECK(mPaintPipeline->create());

        mSigSubmit.request();
    }

    void onRenderTick() override
    {
        QRhiRenderTarget*  currentRenderTarget = mSwapChain->currentFrameRenderTarget();
        QRhiCommandBuffer* cmdBuffer           = mSwapChain->currentFrameCommandBuffer();

        if (mSigInit.ensure()) { initRhiResource(currentRenderTarget); }

        QRhiResourceUpdateBatch* resourceUpdates = nullptr;
        if (mSigSubmit.ensure())
        {
            resourceUpdates = mRhi->nextResourceUpdateBatch();
            resourceUpdates->uploadStaticBuffer(mVertexBuffer.get(), VertexData.data());
        }

        const QColor                     clearColor   = QColor::fromRgbF(0.2f, 0.2f, 0.2f, 1.0f);
        const QRhiDepthStencilClearValue dsClearValue = {1.0f, 0};

        // 1st pass
        cmdBuffer->beginPass(mRenderTarget.get(), clearColor, dsClearValue, resourceUpdates);
        cmdBuffer->setGraphicsPipeline(mPipeline.get());
        cmdBuffer->setViewport(QRhiViewport(0, 0, mRenderTarget->pixelSize().width(),
                                            mRenderTarget->pixelSize().height()));
        cmdBuffer->setShaderResources();
        const QRhiCommandBuffer::VertexInput vertexBindings(mVertexBuffer.get(), 0);
        cmdBuffer->setVertexInput(0, 1, &vertexBindings);
        cmdBuffer->draw(3);
        cmdBuffer->endPass();

        static int          counter        = 0;
        static QRhiTexture* CurrentTexture = nullptr;
        if ((counter != 0) && counter % 60 == 0)
        {
            if ((CurrentTexture == nullptr) || CurrentTexture == mColorAttachment1.get())
            {
                CurrentTexture = mColorAttachment0.get();
            }
            else { CurrentTexture = mColorAttachment1.get(); }
            mPaintShaderBindings->setBindings({
                QRhiShaderResourceBinding::sampledTexture(0,
                                                          QRhiShaderResourceBinding::FragmentStage,
                                                          CurrentTexture, mPaintSampler.get()),
            });
            mPaintShaderBindings->create();
            counter = 0;
        }
        counter++;

        // 2nd pass
        cmdBuffer->beginPass(currentRenderTarget, clearColor, dsClearValue);
        cmdBuffer->setGraphicsPipeline(mPaintPipeline.get());
        cmdBuffer->setViewport(QRhiViewport(0, 0, currentRenderTarget->pixelSize().width(),
                                            currentRenderTarget->pixelSize().height()));
        cmdBuffer->setShaderResources(mPaintShaderBindings.get());
        cmdBuffer->draw(4);
        cmdBuffer->endPass();
    }
};

int main(int argc, char** argv)
{
    qputenv("QSG_INFO", "1");
    QApplication app(argc, argv);

    QRhiHelper::InitParams initParams{.backend = QRhi::Vulkan};
    MRTWindow              window(initParams);
    window.resize({800, 600});
    window.show();

    return QApplication::exec();
}
