#include <array>
#include <cstdlib>
#include <iostream>

#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

#include "Render/RHI/QRhiHelper.h"

static const QString outputPath = "offscreen.png";

static const std::array<float, 6> VertexData = {
    // position(xy)
    0.0f,  0.5f,   //
    -0.5f, -0.5f,  //
    0.5f,  -0.5f,  //
};

static const QString vertexShader = R"(
#version 460
layout(location = 0) in vec2 position;
out gl_PerVertex { vec4 gl_Position; };

void main(){
	gl_Position = vec4(position, 0.0f ,1.0f);
}
)";

static const QString fragmentShader = R"(
#version 460
layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = vec4(0.1f,0.5f,0.9f,1.0f);
}
)";

int main(int argc, char** argv)
{
    qputenv("QSG_INFO", "1");
    QApplication         app(argc, argv);
    QSharedPointer<QRhi> rhi = QRhiHelper::create();

    // set render target
    QScopedPointer<QRhiTexture>              renderTargetTexture;
    QScopedPointer<QRhiTextureRenderTarget>  renderTarget;
    QScopedPointer<QRhiRenderPassDescriptor> renderTargetDesc;
    {
        renderTargetTexture.reset(
            rhi->newTexture(QRhiTexture::RGBA8, QSize(1280, 720), 1,
                            QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        renderTargetTexture->create();
        renderTarget.reset(rhi->newTextureRenderTarget({renderTargetTexture.get()}));
        renderTargetDesc.reset(renderTarget->newCompatibleRenderPassDescriptor());
        renderTarget->setRenderPassDescriptor(renderTargetDesc.get());
        renderTarget->create();
    }

    // pipeline
    QScopedPointer<QRhiBuffer>                 vertexBuffer;
    QScopedPointer<QRhiShaderResourceBindings> shaderBindings;
    QScopedPointer<QRhiGraphicsPipeline>       mPipeline;
    {
        vertexBuffer.reset(
            rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(VertexData)));
        vertexBuffer->create();

        shaderBindings.reset(rhi->newShaderResourceBindings());
        shaderBindings->create();

        // pipeline
        mPipeline.reset(rhi->newGraphicsPipeline());
        mPipeline->setTargetBlends({QRhiGraphicsPipeline::TargetBlend()});
        mPipeline->setSampleCount(renderTarget->sampleCount());
        mPipeline->setDepthTest(false);
        mPipeline->setDepthOp(QRhiGraphicsPipeline::Always);
        mPipeline->setDepthWrite(false);

        // shader
        QShader vs =
            QRhiHelper::newShaderFromCode(QShader::VertexStage, vertexShader.toLocal8Bit());
        QShader fs =
            QRhiHelper::newShaderFromCode(QShader::FragmentStage, fragmentShader.toLocal8Bit());
        Q_ASSERT(fs.isValid());
        Q_ASSERT(vs.isValid());
        mPipeline->setShaderStages({
            QRhiShaderStage{QRhiShaderStage::Vertex, vs},
            QRhiShaderStage{QRhiShaderStage::Fragment, fs},
        });

        // Vertex Input Layout
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({QRhiVertexInputBinding(2 * sizeof(float))});
        inputLayout.setAttributes({
            QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
        });

        mPipeline->setVertexInputLayout(inputLayout);
        mPipeline->setShaderResourceBindings(shaderBindings.get());
        mPipeline->setRenderPassDescriptor(renderTargetDesc.get());
        mPipeline->create();
    }

    QRhiCommandBuffer* cmdBuffer = nullptr;
    if (rhi->beginOffscreenFrame(&cmdBuffer) != QRhi::FrameOpSuccess) { return EXIT_FAILURE; }

    QRhiResourceUpdateBatch* resourceUpdates = rhi->nextResourceUpdateBatch();
    resourceUpdates->uploadStaticBuffer(vertexBuffer.get(), VertexData.data());

    const QColor                     clearColor   = QColor::fromRgbF(0.2f, 0.2f, 0.2f, 1.0f);
    const QRhiDepthStencilClearValue dsClearValue = {1.0f, 0};

    cmdBuffer->beginPass(renderTarget.get(), clearColor, dsClearValue, resourceUpdates);
    {
        cmdBuffer->setGraphicsPipeline(mPipeline.get());
        cmdBuffer->setViewport(QRhiViewport(0, 0, renderTargetTexture->pixelSize().width(),
                                            renderTargetTexture->pixelSize().height()));
        cmdBuffer->setShaderResources();
        const QRhiCommandBuffer::VertexInput vertexBindings(vertexBuffer.get(), 0);
        cmdBuffer->setVertexInput(0, 1, &vertexBindings);
        cmdBuffer->draw(3);

        resourceUpdates = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult rbResult{
            .completed = [&rbResult, &rhi]() -> void {
                if (!rbResult.data.isEmpty())
                {
                    const auto* p = reinterpret_cast<const uchar*>(rbResult.data.constData());
                    QImage      image(p, rbResult.pixelSize.width(), rbResult.pixelSize.height(),
                                      QImage::Format_RGBA8888);
                    if (rhi->isYUpInFramebuffer()) { image.mirrored().save(outputPath); }
                    else { image.save(outputPath); }
                }
            },
        };
        QRhiReadbackDescription rb(renderTargetTexture.get());
        resourceUpdates->readBackTexture(rb, &rbResult);
    }
    cmdBuffer->endPass(resourceUpdates);

    rhi->endOffscreenFrame();
    QDesktopServices::openUrl(QUrl("file:" + outputPath, QUrl::TolerantMode));
    return QApplication::exec();
}
