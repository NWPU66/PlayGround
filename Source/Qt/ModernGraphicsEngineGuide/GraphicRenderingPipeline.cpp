// c

// cpp
#include <array>
#include <rhi/qshader.h>
#include <string>

// 3rdparty
#include <QApplication>
#include <rhi/qrhi.h>
#include <vulkan/vulkan_core.h>

// users
#include "QEngineApplication.h"
#include "Render/RHI/QRhiHelper.h"
#include "Render/RHI/QRhiWidget.h"
#include "Render/RHI/QRhiWindow.h"

// global variable
static const std::array<float, 18> VertexData = {
    // 顶点数据
    // position(xy)      color(rgba)
    0.0f,  -0.5f, 1.0f, 0.0f, 0.0f, 1.0f,  //
    -0.5f, 0.5f,  0.0f, 1.0f, 0.0f, 1.0f,  //
    0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 1.0f,  //
};

static const std::string vertexShader = R"(
#version 460
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

layout (location = 0) out vec4 vColor;

out gl_PerVertex { 
	vec4 gl_Position;
};

void main(){
    gl_Position = vec4(position,0.0f,1.0f);
    vColor = color;
}
)";

static const std::string fragmentShader = R"(
#version 460
layout (location = 0) in vec4 vColor;		
layout (location = 0) out vec4 fragColor;	

void main(){
    fragColor = vColor;
}
)";

/* NOTE - QRhiWindow
class QRhiWindow :public QWindow {
protected:
    virtual void onInit(){}                                 //初式化渲染资源之后会调用
    virtual void onRenderTick() {}                          //每帧都会调用
    virtual void onResize(const QSize& inSize) {}           //当窗口尺寸发生变化时会调用
    virtual void onExit() {}                                //当关闭窗口时调用
protected:
    QSharedPointer<QRhiEx> mRhi;
    QScopedPointer<QRhiSwapChain> mSwapChain;
    QScopedPointer<QRhiRenderBuffer> mDSBuffer  ;
    QScopedPointer<QRhiRenderPassDescriptor> mSwapChainPassDesc;
};
*/

class TriangleWindow : public QRhiWindow {
public:
    explicit TriangleWindow(QRhiHelper::InitParams inInitParams) : QRhiWindow(inInitParams)
    {
        mSigInit.request();    // 请求初始化
        mSigSubmit.request();  // 请求提交资源
    }

protected:
    virtual void onRenderTick() override
    {
        if (mSigInit.ensure()) { initResources(); }

        QRhiRenderTarget*  renderTarget = mSwapChain->currentFrameRenderTarget();
        QRhiCommandBuffer* cmdBuffer    = mSwapChain->currentFrameCommandBuffer();

        if (mSigSubmit.ensure())
        {
            QRhiResourceUpdateBatch* batch =
                mRhi->nextResourceUpdateBatch();  // 申请资源操作合批入口
            batch->uploadStaticBuffer(mVertexBuffer.get(), VertexData.data());
            cmdBuffer->resourceUpdate(batch);
        }
        
        const QColor                     clearColor   = QColor::fromRgbF(0.0f, 0.0f, 0.0f, 1.0f);
        const QRhiDepthStencilClearValue dsClearValue = {1.0f, 0};
        cmdBuffer->beginPass(renderTarget, clearColor, dsClearValue);

        cmdBuffer->setGraphicsPipeline(mPipeline.get());
        cmdBuffer->setViewport(QRhiViewport(0, 0, mSwapChain->currentPixelSize().width(),
                                            mSwapChain->currentPixelSize().height()));
        cmdBuffer->setShaderResources();
        // 设置描述符集布局绑定，如果不填参数（为nullptr），则会使用渲染管线创建时所使用的描述符集布局绑定
        const QRhiCommandBuffer::VertexInput vertexInput(mVertexBuffer.get(), 0);
        // 将 mVertexBuffer 绑定到Buffer0，内存偏移值为0
        cmdBuffer->setVertexInput(0, 1, &vertexInput);
        cmdBuffer->draw(3);  // 执行绘制，其中 3 代表着有 3个顶点数据 输入

        cmdBuffer->endPass();
    }

    void initResources()
    {
        mVertexBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(VertexData)));
        mVertexBuffer->create();

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            QRhiVertexInputBinding(
                6 * sizeof(float))  // 定义每个VertexBuffer，单组顶点数据的跨度，这里是 6 *
                                    // sizeof(float)，可以当作是GPU会从Buffer0（0是Index）读取 6 *
                                    // sizeof(float) 传给 Vertex Shader
        });

        inputLayout.setAttributes({
            QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
            // 从每组顶点数据的位置 0 开始作为 Location0（Float2） 的起始地址
            QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float4, sizeof(float) * 2),
            // 从每组顶点数据的位置 sizeof(float) * 2
            // 开始作为 Location1（Float4） 的起始地址
        });

        mPipeline.reset(mRhi->newGraphicsPipeline());
        mPipeline->setVertexInputLayout(inputLayout);

        mShaderBindings.reset(mRhi->newShaderResourceBindings());  // 创建绑定
        mShaderBindings->create();
        mPipeline->setShaderResourceBindings(mShaderBindings.get());  // 绑定到流水线

        mPipeline->setSampleCount(mSwapChain->sampleCount());
        mPipeline->setRenderPassDescriptor(mSwapChainPassDesc.get());
        mPipeline->setTopology(QRhiGraphicsPipeline::Topology::Triangles);

        QShader vs = QRhiHelper::newShaderFromCode(QShader::VertexStage, vertexShader.c_str());
        QShader fs = QRhiHelper::newShaderFromCode(QShader::FragmentStage, fragmentShader.c_str());
        Q_ASSERT(vs.isValid());
        Q_ASSERT(fs.isValid());
        mPipeline->setShaderStages({
            QRhiShaderStage(QRhiShaderStage::Vertex, vs),
            QRhiShaderStage(QRhiShaderStage::Fragment, fs),
        });

        mPipeline->create();
    }

private:
    QRhiSignal                                 mSigInit;         // 用于初始化的信号
    QRhiSignal                                 mSigSubmit;       // 用于提交资源的信号
    QScopedPointer<QRhiBuffer>                 mVertexBuffer;    // 顶点缓冲区
    QScopedPointer<QRhiShaderResourceBindings> mShaderBindings;  // 描述符集布局绑定
    QScopedPointer<QRhiGraphicsPipeline>       mPipeline;
};

int main(int argc, char** argv)
{
    QEngineApplication     app(argc, argv);
    QRhiHelper::InitParams initParams{.backend = QRhi::Vulkan};
    TriangleWindow         window(initParams);
    window.resize(QSize(800, 600));
    window.show();
    return QEngineApplication::exec();
}
