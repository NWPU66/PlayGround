// c
#include <array>

// 3rdparty
#include "QDateTime"
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>
#include <QtWidgets>

// users
#include "Render/RHI/QRhiHelper.h"
#include "Render/RHI/QRhiWindow.h"

static const std::array<float, 16> VertexData = {
    // position(xy)		texture coord(uv)
    1.0f,  1.0f,  1.0f, 1.0f,  //
    1.0f,  -1.0f, 1.0f, 0.0f,  //
    -1.0f, -1.0f, 0.0f, 0.0f,  //
    -1.0f, 1.0f,  0.0f, 1.0f,  //
};

static const std::array<int, 6> IndexData = {
    0, 1, 2,  //
    2, 3, 0,  //
};

static const std::string imagePath = "E:/Study/CodeProj/HelloQt/Asset/4x.jpg";

static const std::string vertexShader = R"(
#version 460
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 vUV;

out gl_PerVertex { vec4 gl_Position; };

void main(){
    vUV = inUV;
	gl_Position = vec4(inPosition,0.0f,1.0f);
}
)";

static const std::string fragmentShader = R"(
#version 460
layout(location = 0) in vec2 vUV;
layout (location = 0) out vec4 outFragColor;	

layout(binding = 0) uniform UniformBlock{
	float time;
	vec2 mousePos;
}UBO;

layout(binding = 1) uniform sampler2D inTexture;

void main(){
    vec4 textureColor = texture(inTexture,vUV);

    const float speed = 5.0f;
    vec4 mixColor = vec4(1.0f) * sin(UBO.time * speed);

    outFragColor = mix(textureColor, mixColor, distance(gl_FragCoord.xy,UBO.mousePos)/500);
}
)";

struct UniformBlock
{
    float time = 0;
    alignas(8) QVector2D mousePos;  // 设置成8Byte对齐

    /**NOTE - 内存对齐
    float time；
    QVector2D mousePos;
    结构：4B|4B|4B

    float time；
    alignas(8) QVector2D mousePos;
    结构：4B|空(4B)|4B|4B
     */
};

class MyWindow : public QRhiWindow {
public:
    explicit MyWindow(QRhiHelper::InitParams initParams) : QRhiWindow(initParams)
    {
        mSigInit.request();
    }

private:
    QRhiSignal mSigInit;
    QRhiSignal mSigSubmit;

    QImage                                     mImage;
    QScopedPointer<QRhiBuffer>                 mVertexBuffer;
    QScopedPointer<QRhiBuffer>                 mIndexBuffer;
    QScopedPointer<QRhiBuffer>                 mUniformBuffer;
    QScopedPointer<QRhiTexture>                mTexture;
    QScopedPointer<QRhiSampler>                mSampler;
    QScopedPointer<QRhiShaderResourceBindings> mShaderBindings;
    QScopedPointer<QRhiGraphicsPipeline>       mPipeline;

    void keyPressEvent(QKeyEvent* event) override
    {
        QRhiWindow::keyPressEvent(event);
        if (event->key() == Qt::Key_Escape) { close(); }
    }

    void initRhiResource()
    {
        // image
        mImage = QImage(imagePath.c_str()).convertedTo(QImage::Format_RGBA8888);

        // texture
        mTexture.reset(mRhi->newTexture(QRhiTexture::RGBA8, mImage.size()));
        mTexture->create();  // 创建的是空的

        // sampler
        mSampler.reset(
            mRhi->newSampler(QRhiSampler::Filter::Linear, QRhiSampler::Filter::Nearest,
                             QRhiSampler::Filter::Linear, QRhiSampler::AddressMode::Repeat,
                             QRhiSampler::AddressMode::Repeat, QRhiSampler::AddressMode::Repeat));
        mSampler->create();

        // vertex buffer
        mVertexBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(VertexData)));
        mVertexBuffer->create();

        // imdex buffer
        mIndexBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(IndexData)));
        mIndexBuffer->create();

        // uniform buffer
        mUniformBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UniformBlock)));
        mUniformBuffer->create();

        // descriptor set
        mShaderBindings.reset(mRhi->newShaderResourceBindings());
        mShaderBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(
                0, QRhiShaderResourceBinding::StageFlag::FragmentStage, mUniformBuffer.get()),
            QRhiShaderResourceBinding::sampledTexture(
                1, QRhiShaderResourceBinding::StageFlag::FragmentStage, mTexture.get(),
                mSampler.get()),
        });
        mShaderBindings->create();

        // pipeline
        mPipeline.reset(mRhi->newGraphicsPipeline());

        QRhiGraphicsPipeline::TargetBlend targetBlend;
        targetBlend.enable = false;
        mPipeline->setTargetBlends({targetBlend});

        mPipeline->setSampleCount(mSwapChain->sampleCount());
        mPipeline->setTopology(QRhiGraphicsPipeline::Triangles);

        // shader
        QShader vs = QRhiHelper::newShaderFromCode(QShader::VertexStage, vertexShader.c_str());
        QShader fs = QRhiHelper::newShaderFromCode(QShader::FragmentStage, fragmentShader.c_str());
        Q_ASSERT(fs.isValid());
        Q_ASSERT(vs.isValid());
        mPipeline->setShaderStages({
            {QRhiShaderStage::Vertex, vs},
            {QRhiShaderStage::Fragment, fs},
        });

        // vertex input
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            QRhiVertexInputBinding{4 * sizeof(float)},
        });
        inputLayout.setAttributes({
            QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float2, 0},
            QRhiVertexInputAttribute{0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)},
        });

        mPipeline->setVertexInputLayout(inputLayout);
        mPipeline->setShaderResourceBindings(mShaderBindings.get());
        mPipeline->setRenderPassDescriptor(mSwapChainPassDesc.get());
        mPipeline->create();

        mSigSubmit.request();
    }

    void onRenderTick() override
    {
        if (mSigInit.ensure()) { initRhiResource(); }

        QRhiRenderTarget*  renderTarget = mSwapChain->currentFrameRenderTarget();
        QRhiCommandBuffer* cmdBuffer    = mSwapChain->currentFrameCommandBuffer();

        QRhiResourceUpdateBatch* batch = mRhi->nextResourceUpdateBatch();
        if (mSigSubmit.ensure())
        {
            batch->uploadStaticBuffer(mIndexBuffer.get(), IndexData.data());
            batch->uploadStaticBuffer(mVertexBuffer.get(), VertexData.data());
            batch->uploadTexture(mTexture.get(), mImage);
        }
        UniformBlock ubo{
            .time     = QTime::currentTime().msecsSinceStartOfDay() / 1000.0f,
            .mousePos = QVector2D(mapFromGlobal(QCursor::pos())) * qApp->devicePixelRatio(),
        };
        batch->updateDynamicBuffer(mUniformBuffer.get(), 0, sizeof(UniformBlock), &ubo);

        const QColor                     clearColor   = QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f);
        const QRhiDepthStencilClearValue dsClearValue = {1.0f, 0};

        cmdBuffer->beginPass(renderTarget, clearColor, dsClearValue, batch);

        cmdBuffer->setGraphicsPipeline(mPipeline.get());
        cmdBuffer->setViewport(QRhiViewport(0, 0, mSwapChain->currentPixelSize().width(),
                                            mSwapChain->currentPixelSize().height()));
        cmdBuffer->setShaderResources();  // REVIEW -
        const QRhiCommandBuffer::VertexInput vertexBindings(mVertexBuffer.get(), 0);
        cmdBuffer->setVertexInput(0, 1, &vertexBindings, mIndexBuffer.get(), 0,
                                  QRhiCommandBuffer::IndexUInt32);
        cmdBuffer->drawIndexed(6);

        cmdBuffer->endPass();
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QRhiHelper::InitParams initParams{.backend = QRhi::Vulkan};
    MyWindow               window(initParams);
    window.resize({800, 600});
    window.show();

    return QApplication::exec();
}
