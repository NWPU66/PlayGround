#include <QApplication>
#include <rhi/qrhi.h>

#include "Render/RHI/QRhiWindow.h"
#include "private/qrhivulkan_p.h"
#include "qvulkanfunctions.h"

static const std::string computeShader = R"(
#version 460
layout(std140, binding = 0) buffer StorageBuffer{
	int counter;
}SSBO;
layout (binding = 1, rgba8) uniform image2D Tex;
const int ImageSize = 64 * 64;

void main(){
    int currentCounter = atomicAdd(SSBO.counter,1);
}
)";

struct DispatchStruct
{
    int x;
    int y;
    int z;
};

class IndirectDrawWindow : public QRhiWindow {
public:
    explicit IndirectDrawWindow(QRhiHelper::InitParams inInitParams) : QRhiWindow(inInitParams)
    {
        mSigInit.request();
        mSigSubmit.request();
    }

private:
    QRhiSignal mSigInit;
    QRhiSignal mSigSubmit;

    QScopedPointer<QRhiBuffer> mStorageBuffer;
    QScopedPointer<QRhiBuffer> mIndirectDrawBuffer;

    QScopedPointer<QRhiComputePipeline>        mPipeline;
    QScopedPointer<QRhiShaderResourceBindings> mShaderBindings;

    void initResource()
    {
        mStorageBuffer.reset(
            mRhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, sizeof(float)));
        mStorageBuffer->create();

        mIndirectDrawBuffer.reset(QRhiHelper::newVkBuffer(mRhi.get(), QRhiBuffer::Static,
                                                          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                                          sizeof(DispatchStruct)));
        mIndirectDrawBuffer->create();

        mShaderBindings.reset(mRhi->newShaderResourceBindings());
        mShaderBindings->setBindings({
            QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage,
                                                       mStorageBuffer.get()),
        });
        mShaderBindings->create();

        mPipeline.reset(mRhi->newComputePipeline());

        // compute shader
        QShader cs = QRhiHelper::newShaderFromCode(QShader::ComputeStage, computeShader.c_str());
        Q_ASSERT(cs.isValid());
        mPipeline->setShaderStage({QRhiShaderStage(QRhiShaderStage::Compute, cs)});

        mPipeline->setShaderResourceBindings(mShaderBindings.get());
        mPipeline->create();
    }

    void onRenderTick() override
    {
        QRhiRenderTarget*  currentRenderTarget = mSwapChain->currentFrameRenderTarget();
        QRhiCommandBuffer* cmdBuffer           = mSwapChain->currentFrameCommandBuffer();

        if (mSigInit.ensure()) { initResource(); }

        if (mSigSubmit.ensure())
        {
            QRhiResourceUpdateBatch* resourceUpdates = mRhi->nextResourceUpdateBatch();
            DispatchStruct           dispatch        = {3, 3, 3};
            resourceUpdates->uploadStaticBuffer(mIndirectDrawBuffer.get(), 0,
                                                sizeof(DispatchStruct), &dispatch);
            cmdBuffer->resourceUpdate(resourceUpdates);
            mRhi->finish();
        }

        QVulkanInstance*        vkInstance = vulkanInstance();
        auto*                   vkHandle   = (QRhiVulkanNativeHandles*)mRhi->nativeHandles();
        QVulkanDeviceFunctions* vkDevFunc  = vkInstance->deviceFunctions(vkHandle->dev);

        auto* cmdBufferHandles = (QRhiVulkanCommandBufferNativeHandles*)cmdBuffer->nativeHandles();
        auto* cbD              = QRHI_RES(QVkCommandBuffer, cmdBuffer);
        VkCommandBuffer vkCmdBuffer = cmdBufferHandles->commandBuffer;

        auto*      pipelineHandle = (QVkComputePipeline*)mPipeline.get();
        VkPipeline vkPipeline     = pipelineHandle->pipeline;

        QRhiBuffer::NativeBuffer indirectBufferHandle = mIndirectDrawBuffer->nativeBuffer();
        VkBuffer                 vkIndirectBuffer     = *(VkBuffer*)indirectBufferHandle.objects[0];

        cmdBuffer->beginExternal();  // 开始扩展，之后可输入原生API的指令
        vkDevFunc->vkCmdBindPipeline(
            vkCmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline);
        QRhiHelper::setShaderResources(
            mPipeline.get(), cmdBuffer,
            mShaderBindings.get());  // 辅助函数，用于更新VK流水线的描述符集绑定
        vkDevFunc->vkCmdDispatchIndirect(vkCmdBuffer, vkIndirectBuffer, 0);
        cmdBuffer->endExternal();

        static QRhiReadbackResult mCtxReader;
        mCtxReader.completed = [this]() {
            int counter = 0;
            memcpy(&counter, mCtxReader.data.constData(), mCtxReader.data.size());
            qDebug() << counter;
        };
        QRhiResourceUpdateBatch* resourceUpdates = mRhi->nextResourceUpdateBatch();
        resourceUpdates->readBackBuffer(mStorageBuffer.get(), 0, sizeof(float), &mCtxReader);
        cmdBuffer->resourceUpdate(resourceUpdates);
        mRhi->finish();
    }
};

int main(int argc, char** argv)
{
    qputenv("QSG_INFO", "1");
    QApplication           app(argc, argv);
    QRhiHelper::InitParams initParams{.backend = QRhi::Implementation::Vulkan};
    IndirectDrawWindow     window(initParams);
    window.resize({800, 600});
    window.show();

    return QApplication::exec();
}
