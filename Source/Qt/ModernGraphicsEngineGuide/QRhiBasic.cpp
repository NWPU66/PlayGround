#include <QApplication>
#include <private/qrhi_p.h>
#include <private/qrhid3d11_p.h>
#include <private/qrhivulkan_p.h>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QRhiD3D11InitParams  params;
    QRhi::Flags          flags;
    QSharedPointer<QRhi> rhi(QRhi::create(QRhi::D3D11, &params, flags));
    qDebug() << rhi->driverInfo();  // 打印显卡设备信息

    return QApplication::exec();
}
