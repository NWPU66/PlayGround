#include <QApplication>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>
#include <QtWidgets>

class QExmapleWidget : public QWidget {
    Q_OBJECT;
    Q_DISABLE_COPY_MOVE(QExmapleWidget)

public:
    explicit QExmapleWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        qDebug() << "QExmapleWidget::QExmapleWidget()";
        // setWindowFlags(Qt::FramelessWindowHint);     // 无边框
        // setAttribute(Qt::WA_TranslucentBackground);  // 背景透明
        // setAttribute(Qt::WA_AlwaysStackOnTop);       // 置顶
        resize(800, 600);

        createUI();
    }

    ~QExmapleWidget()
    {
        QWidget::~QWidget();
        qDebug() << "QExmapleWidget::~QExmapleWidget()";
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        QWidget::mousePressEvent(event);
        qDebug() << "QExmapleWidget::mousePressEvent()";

        if (event->button() == Qt::LeftButton)
        {
            mMousePressPos = event->pos();
            bIsPressed     = true;
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        QWidget::mouseReleaseEvent(event);
        qDebug() << "QExmapleWidget::mouseReleaseEvent()";

        if (event->button() == Qt::LeftButton)
        {
            bIsPressed = false;
            update();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        QWidget::mouseMoveEvent(event);
        // qDebug() << "QExmapleWidget::mouseMoveEvent()";

        if ((event->buttons() & Qt::LeftButton) != 0)
        {
            move(event->pos() + this->pos() - mMousePressPos);
            // qDebug() << "Pos: " << this->pos();
        }
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        QWidget::keyPressEvent(event);
        qDebug() << "QExmapleWidget::keyPressEvent()";

        if (event->key() == Qt::Key_Escape) { close(); }
    }

    void enterEvent(QEnterEvent* event) override
    {
        QWidget::enterEvent(event);
        qDebug() << "QExmapleWidget::enterEvent()";

        bIsHovered = true;                // 鼠标进入Widget中
        setCursor(Qt::ClosedHandCursor);  // 设置鼠标光标的形状
        update();                         // 请求刷新界面
    }

    void leaveEvent(QEvent* event) override
    {
        QWidget::leaveEvent(event);
        qDebug() << "QExmapleWidget::leaveEvent()";

        bIsHovered = false;          // 鼠标离开Widget
        setCursor(Qt::ArrowCursor);  // 设置鼠标光标的形状
        update();                    // 请求刷新界面
    }

    /**
     * @brief paintEvent
     *
     * @note 请求刷新界面 update() 时才会刷新
     */
    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);
        qDebug() << "QExmapleWidget::paintEvent()";

        if (bIsPressed) { qDebug() << "bIsPressed"; }

        QPainter painter(this);
        QColor   backgroundColor;
        if (!bIsHovered) { backgroundColor = Qt::blue; }
        else if (bIsPressed) { backgroundColor = Qt::red; }
        else { backgroundColor = Qt::green; }
        painter.fillRect(this->rect(), backgroundColor);

        painter.setPen(QPen(Qt::white));
        painter.setFont(QFont("JetBrains Mono", 20, 90));
        painter.drawText(this->rect(), Qt::AlignCenter, "Hello, World!");
    }

private:
    QPoint mMousePressPos;
    bool   bIsHovered = false;
    bool   bIsPressed = false;

    void createUI()
    {
        auto* hLayout = new QHBoxLayout(this);
        /* NOTE - 直接在Heap上创建，它的销毁由Qt的GC系统负责。
        QHBoxLayout* hLayout = new QHBoxLayout();   //等价
        setLayout(hLayout);
        */
        
        hLayout->setContentsMargins(5, 5, 5, 5);                  // 设置外边距
        hLayout->setSpacing(10);                                  // 设置内部元素间隔
        hLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);  // 设置左对齐且垂直居中
        hLayout->addWidget(new QPushButton("A"));
        hLayout->addSpacing(20);  // 加入空白填充
        hLayout->addWidget(new QPushButton("B"));
        hLayout->addWidget(new QPushButton("C"));
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QExmapleWidget widget;
    widget.show();

    return QApplication::exec();
}

#include "QtWidget.moc"
