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
#include <qmatrix4x4.h>
#include <rhi/qrhi.h>

// users
#include "Render/RHI/QRhiHelper.h"
#include "Render/RHI/QRhiWindow.h"
#include "Utils/QRhiCamera.h"

struct UniformBlock
{
    float time = 0.0f;
    alignas(16) QGenericMatrix<4, 4, float> MVP;
};

static const std::array<float, 30> GridData = {
    // xyz			    uv
    -1.0f, 0.0f, -1.0f, 0.0f,   0.0f,    //
    1.0f,  0.0f, -1.0f, 100.0f, 0.0f,    //
    1.0f,  0.0f, 1.0f,  100.0f, 100.0f,  // 使用重复填充扩展UV

    1.0f,  0.0f, 1.0f,  100.0f, 100.0f,  //
    -1.0f, 0.0f, 1.0f,  0.0f,   100.0f,  //
    -1.0f, 0.0f, -1.0f, 0.0f,   0.0f,    //
};

static const std::string imagePath = "E:/Study/CodeProj/HelloQt/Asset/Grid.png";

static const std::string vertexShader = R"(
#version 460
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec2 vUV;

layout(binding = 0) uniform UniformBlock{
    float time;
	mat4 MVP;
}UBO;

out gl_PerVertex { vec4 gl_Position; };

void main(){
    vUV = inUV;
	gl_Position = UBO.MVP * vec4(inPosition ,1.0f);
}
)";

static const std::string fragmentShader = R"(
#version 460
layout(location = 0) in vec2 vUV;
layout (location = 0) out vec4 outFragColor;	

layout(binding = 1) uniform sampler2D uTexture;

layout(binding = 0) uniform UniformBlock{
    float time;
	mat4 MVP;
}UBO;

void main(){
    vec2 newUV = vec2(vUV.x + UBO.time / 10.0f, vUV.y);
    outFragColor = texture(uTexture,newUV);
}
)";

class MyWindow : public QRhiWindow {
public:
    explicit MyWindow(QRhiHelper::InitParams initParams) : QRhiWindow(initParams)
    {
        mSigInit.request();
    }

private:
    QRhiSignal mSigInit;
    QRhiSignal mSigSubmit;

    QScopedPointer<QRhiCamera>                 mCamera;
    QImage                                     mImage;
    QScopedPointer<QRhiTexture>                mTexture;
    QScopedPointer<QRhiSampler>                mSampler;
    QScopedPointer<QRhiBuffer>                 mVertexBuffer;
    QScopedPointer<QRhiBuffer>                 mUniformBuffer;
    QScopedPointer<QRhiShaderResourceBindings> mShaderBindings;
    QScopedPointer<QRhiGraphicsPipeline>       mPipeline;

    void keyPressEvent(QKeyEvent* event) override
    {
        QRhiWindow::keyPressEvent(event);
        if (event->key() == Qt::Key_Escape) { close(); }
    }

    void initRhiResource()
    {
        // camera
        mCamera.reset(new QRhiCamera());
        mCamera->setupRhi(mRhi.get());
        mCamera->setupWindow(this);
        mCamera->setPosition(QVector3D(0, 10, 0));

        // image
        mImage = QImage(imagePath.c_str()).convertedTo(QImage::Format_RGBA8888);
        // texture
        mTexture.reset(
            mRhi->newTexture(QRhiTexture::RGBA8, mImage.size(), 1,
                             QRhiTexture::Flag::MipMapped | QRhiTexture::UsedWithGenerateMips));
        mTexture->create();  // 创建的是空的

        // sampler
        mSampler.reset(
            mRhi->newSampler(QRhiSampler::Filter::Linear, QRhiSampler::Filter::Nearest,
                             QRhiSampler::Filter::Linear, QRhiSampler::AddressMode::Repeat,
                             QRhiSampler::AddressMode::Repeat, QRhiSampler::AddressMode::Repeat));
        mSampler->create();

        // vertex buffer
        mVertexBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(GridData)));
        mVertexBuffer->create();

        // uniform buffer
        mUniformBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(UniformBlock)));
        mUniformBuffer->create();

        // descriptor set
        mShaderBindings.reset(mRhi->newShaderResourceBindings());
        mShaderBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(
                0,
                QRhiShaderResourceBinding::StageFlag::VertexStage |
                    QRhiShaderResourceBinding::StageFlag::FragmentStage,
                mUniformBuffer.get()),
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
            QRhiVertexInputBinding{5 * sizeof(float)},
        });
        inputLayout.setAttributes({
            QRhiVertexInputAttribute{0, 0, QRhiVertexInputAttribute::Float3, 0},
            QRhiVertexInputAttribute{0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float)},
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
            batch->uploadStaticBuffer(mVertexBuffer.get(), GridData.data());
            batch->uploadTexture(mTexture.get(), mImage);
            batch->generateMips(mTexture.get());
        }

        QMatrix4x4 model;
        model.scale(10000);
        UniformBlock ubo{
            .time = QTime::currentTime().msecsSinceStartOfDay() / 1000.0f,
            .MVP  = (mCamera->getProjectionMatrixWithCorr() * mCamera->getViewMatrix() * model)
                       .toGenericMatrix<4, 4>(),
        };
        batch->updateDynamicBuffer(mUniformBuffer.get(), 0, sizeof(UniformBlock), &ubo);

        const QColor                     clearColor   = QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f);
        const QRhiDepthStencilClearValue dsClearValue = {1.0f, 0};

        cmdBuffer->beginPass(renderTarget, clearColor, dsClearValue, batch);

        cmdBuffer->setGraphicsPipeline(mPipeline.get());
        cmdBuffer->setViewport(QRhiViewport(0, 0, mSwapChain->currentPixelSize().width(),
                                            mSwapChain->currentPixelSize().height()));
        cmdBuffer->setShaderResources();
        const QRhiCommandBuffer::VertexInput vertexBindings(mVertexBuffer.get(), 0);
        cmdBuffer->setVertexInput(0, 1, &vertexBindings);
        cmdBuffer->draw(6);

        cmdBuffer->endPass();
    }
};

int main(int argc, char** argv)
{
    qputenv("QSG_INFO", "1");
    QApplication app(argc, argv);

    QRhiHelper::InitParams initParams{.backend = QRhi::Vulkan};
    MyWindow               window(initParams);
    window.resize({800, 600});
    window.show();

    return QApplication::exec();
}
