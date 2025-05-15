#include <QApplication>
#include <qcontainerfwd.h>
#include <rhi/qrhi.h>

#include "Render/RHI/QRhiWindow.h"

static const std::string computeShader = R"(
#version 460
layout(std140, binding = 0) buffer StorageBuffer{
	int counter;
}SSBO;
layout (binding = 1, rgba8) uniform image2D Tex;
const int ImageSize = 64 * 64;

void main(){
    int currentCounter = atomicAdd(SSBO.counter,1);
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    imageStore(Tex,pos,vec4(currentCounter/float(ImageSize),0,0,1));
}
)";

static const std::string vertexShader = R"(
#version 460
layout (location = 0) out vec2 vUV;
out gl_PerVertex { vec4 gl_Position; };

void main(){
    vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0f - 1.0f, 0.0f, 1.0f);
	%1
}
)";

static const std::string fragmentShader = R"(
#version 460
layout (binding = 0) uniform sampler2D uSamplerColor;
layout (location = 0) in vec2 vUV;
layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = vec4(texture(uSamplerColor, vUV).rgb,1.0f);
}
)";

class ComputeShaderWindow : public QRhiWindow {
public:
    explicit ComputeShaderWindow(QRhiHelper::InitParams inInitParams) : QRhiWindow(inInitParams)
    {
        mSigInit.request();
    }

private:
    QRhiSignal mSigInit;

    QScopedPointer<QRhiBuffer>  mStorageBuffer;
    QScopedPointer<QRhiTexture> mTexture;

    QScopedPointer<QRhiComputePipeline>        mPipeline;
    QScopedPointer<QRhiShaderResourceBindings> mShaderBindings;

    QScopedPointer<QRhiSampler>                mPaintSampler;
    QScopedPointer<QRhiShaderResourceBindings> mPaintShaderBindings;
    QScopedPointer<QRhiGraphicsPipeline>       mPaintPipeline;

    const int ImageWidth  = 64;
    const int ImageHeight = 64;

    void initResource(QRhiRenderTarget* currentRenderTarget)
    {
        // resource
        mStorageBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, sizeof(float)));
        mStorageBuffer->create();
        mTexture.reset(mRhi->newTexture(QRhiTexture::RGBA8, QSize(ImageWidth, ImageHeight), 1,
                                        QRhiTexture::UsedWithLoadStore));
        mTexture->create();

        // compute pipeline
        mPipeline.reset(mRhi->newComputePipeline());
        mShaderBindings.reset(mRhi->newShaderResourceBindings());
        mShaderBindings->setBindings({
            QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage,
                                                       mStorageBuffer.get()),
            QRhiShaderResourceBinding::imageLoadStore(1, QRhiShaderResourceBinding::ComputeStage,
                                                      mTexture.get(), 0),
        });
        mShaderBindings->create();

        // compute shader
        QShader cs = QRhiHelper::newShaderFromCode(QShader::ComputeStage, computeShader.c_str());
        Q_ASSERT(cs.isValid());
        mPipeline->setShaderStage({QRhiShaderStage(QRhiShaderStage::Compute, cs)});

        mPipeline->setShaderResourceBindings(mShaderBindings.get());
        mPipeline->create();

        // graphics pipeline
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
        QShader vs = QRhiHelper::newShaderFromCode(
            QShader::VertexStage, QString::fromStdString(vertexShader)
                                      .arg(mRhi->isYUpInNDC() ? "	vUV.y = 1 - vUV.y;" : "")
                                      .toLocal8Bit());
        QShader fs = QRhiHelper::newShaderFromCode(QShader::FragmentStage, fragmentShader.c_str());
        Q_ASSERT(fs.isValid());
        Q_ASSERT(vs.isValid());
        mPaintPipeline->setShaderStages({
            {QRhiShaderStage::Vertex, vs},
            {QRhiShaderStage::Fragment, fs},
        });

        // sampler
        mPaintSampler.reset(mRhi->newSampler(
            QRhiSampler::Filter::Linear, QRhiSampler::Filter::Linear, QRhiSampler::Filter::None,
            QRhiSampler::Repeat, QRhiSampler::Repeat, QRhiSampler::Repeat));
        mPaintSampler->create();

        mPaintShaderBindings.reset(mRhi->newShaderResourceBindings());
        mPaintShaderBindings->setBindings({
            QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage,
                                                      mTexture.get(), mPaintSampler.get()),
        });
        mPaintShaderBindings->create();
        mPaintPipeline->setShaderResourceBindings(mPaintShaderBindings.get());
        mPaintPipeline->setRenderPassDescriptor(currentRenderTarget->renderPassDescriptor());
        mPaintPipeline->create();
    }

    void onRenderTick() override
    {
        QRhiRenderTarget*  currentRenderTarget = mSwapChain->currentFrameRenderTarget();
        QRhiCommandBuffer* cmdBuffer           = mSwapChain->currentFrameCommandBuffer();

        if (mSigInit.ensure()) { initResource(currentRenderTarget); }

        QRhiResourceUpdateBatch* resourceUpdates = mRhi->nextResourceUpdateBatch();
        const int                counter         = 0;
        resourceUpdates->uploadStaticBuffer(mStorageBuffer.get(), &counter);

        // compute pass
        cmdBuffer->beginComputePass(resourceUpdates);
        cmdBuffer->setComputePipeline(mPipeline.get());
        cmdBuffer->setShaderResources();
        cmdBuffer->dispatch(ImageWidth, ImageHeight, 1);
        cmdBuffer->endComputePass();

        // graphics pass
        const QColor                     clearColor   = QColor::fromRgbF(0.2f, 0.2f, 0.2f, 1.0f);
        const QRhiDepthStencilClearValue dsClearValue = {1.0f, 0};
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

    QApplication           app(argc, argv);
    QRhiHelper::InitParams initParams{.backend = QRhi::Implementation::Vulkan};
    ComputeShaderWindow    window(initParams);
    window.resize({800, 600});
    window.show();

    return QApplication::exec();
}
