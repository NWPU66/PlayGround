// c

// c++

// 3rdparty
#include <QApplication>
#include <QMetaProperty>
#include <QMetaType>
#include <QWindow>

// users

class QExmapleClass : public QObject {
    Q_OBJECT                                       // Q_OBJECT是Qt反射数据的入口
    Q_PROPERTY(int Var READ getVar WRITE setVar);  // 通过Getter、Setter生命MetaProperty

public:
    int  getVar() { return mVar; }
    void setVar(int inVar) { mVar = inVar; }

    Q_INVOKABLE void sayHello(const QString& inName)  // 声明MetaMethod
    {
        qDebug() << "Hello " << inName.toLatin1().data();
    }

    enum Number { Zero, One, Two, Three };
    Q_ENUM(Number)  // 声明MetaEnum
protected:
    int mVar = 5;
};

int main(int argc, char* argv[])
{
    // Qt Application
    QApplication a(argc, argv);
    Q_ASSERT(QApplication::instance() == &a);

    // Qt Window
    QWindow window;
    window.setTitle("Hello Qt!");
    window.show();

    // Qt Reflection
    {
        QExmapleClass obj;

        // 获取MetaObject
        const QMetaObject* metaObjectFromStatic  = &QExmapleClass::staticMetaObject;
        const QMetaObject* metaObjectFromVirtual = obj.metaObject();
        if (metaObjectFromStatic == metaObjectFromVirtual) { qDebug() << "MetaObject is same!"; }

        // 读写属性
        obj.setProperty("Var", QVariant::fromValue<int>(10));
        QVariant var = obj.property("Var");
        qDebug() << "Property : " << var.value<int>();

        // 调用函数
        qDebug() << "Invoke Method Begin:";
        QMetaObject::invokeMethod(&obj, "sayHello", Q_ARG(QString, "Boy"));
        qDebug() << "Invoke Method End";

        // MetaType操作
        QMetaType metaType = var.metaType();
        qDebug() << "Meta Type : " << metaType.name() << " ID: " << metaType.id();

        // 在MetaObject中遍历及读写属性，并打印属性类型
        for (int i = metaObjectFromStatic->propertyOffset();
             i < metaObjectFromStatic->propertyCount(); i++)
        {
            const QMetaProperty& metaProperty = metaObjectFromStatic->property(i);
            metaProperty.write(&obj, QVariant::fromValue<int>(15));
            QVariant varFromMetaObject = metaProperty.read(&obj);
            qDebug() << "Meta Property : " << metaProperty.name() << varFromMetaObject;
        }

        // 在MetaObject中遍历函数，并打印参数信息
        for (int i = metaObjectFromStatic->methodOffset(); i < metaObjectFromStatic->methodCount();
             i++)
        {
            const QMetaMethod& metaMethod = metaObjectFromStatic->method(i);
            QByteArrayList     paramNames = metaMethod.parameterNames();
            QStringList        paramsDefineList;
            for (int j = 0; j < metaMethod.parameterCount(); j++)
            {
                paramsDefineList << metaMethod.parameterTypeName(j) + " " + paramNames[j];
            }
            qDebug() << QString("Meta Method : %1 %2(%3)")
                            .arg(metaMethod.typeName())
                            .arg(metaMethod.name())
                            .arg(paramsDefineList.join(','))
                            .toLatin1()
                            .data();
        }

        // 在MetaObject中遍历枚举，并打印枚举信息
        for (int i = metaObjectFromStatic->enumeratorOffset();
             i < metaObjectFromStatic->enumeratorCount(); i++)
        {
            const QMetaEnum& metaEnum = metaObjectFromStatic->enumerator(i);
            qDebug() << "Meta Enum : " << metaEnum.name();
            for (int j = 0; j < metaEnum.keyCount(); j++)
            {
                qDebug() << "-- " << metaEnum.key(j) << " : " << metaEnum.value(j);
            }
        }
    }

    return a.exec();
    /**NOTE - a.exec() 等价于
    while (true) { //QApplication内部有一个退出信号，可惜它是私有的，这里就只能作为死循环了
        QApplication::processEvents();  //注释它,你会发现窗口无法再响应交互事件
    }
     */
}

#include "MetaObjectSystem.moc"  //必须包含main.moc才能触发moc编译
