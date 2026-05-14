#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QPainter>
#include <QWidget>
#include <QDateTime>

// --- THE PRO CIRCULAR GAUGE ---
class CircularGauge : public QWidget {
    Q_OBJECT
public:
    explicit CircularGauge(QWidget *parent = nullptr) : QWidget(parent), m_value(0) {
        setMinimumSize(160, 160);
    }
    void setValue(int val) { m_value = qBound(0, val, 100); update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        int side = qMin(width(), height());
        p.translate(width() / 2, height() / 2);
        p.scale(side / 200.0, side / 200.0);
        p.setPen(QPen(QColor(60, 60, 60), 12, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(-80, -80, 160, 160, -30 * 16, 240 * 16);
        QConicalGradient grad(0, 0, -150);
        grad.setColorAt(0.0, Qt::green);
        grad.setColorAt(0.2, Qt::yellow);
        grad.setColorAt(0.4, Qt::red);
        p.setPen(QPen(QBrush(grad), 12, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(-80, -80, 160, 160, 210 * 16, - (m_value * 2.4) * 16);
        p.save();
        p.rotate(210 + (m_value * 2.4));
        p.setPen(Qt::NoPen); p.setBrush(QColor(255, 50, 50));
        static const QPoint needle[3] = { QPoint(0, -3), QPoint(0, 3), QPoint(75, 0) };
        p.drawConvexPolygon(needle, 3);
        p.restore();
        p.setBrush(Qt::white); p.drawEllipse(-6, -6, 12, 12);
        p.setPen(Qt::white); p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(-20, 45, 40, 20, Qt::AlignCenter, QString::number(m_value) + "%");
    }
private:
    int m_value;
};

// --- THE LIVE RADAR MAP ---
class LiveMap : public QWidget {
    Q_OBJECT
public:
    explicit LiveMap(QWidget *parent = nullptr) : QWidget(parent), distA(50), distB(50) {
        setMinimumSize(160, 160);
    }
    void setDistances(int a, int b) { distA = qBound(5, a, 100); distB = qBound(5, b, 100); update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(width() / 2, height() / 2);
        p.scale(qMin(width(), height()) / 200.0, qMin(width(), height()) / 200.0);

        // Draw Car
        p.setBrush(QColor(52, 152, 219));
        p.setPen(QPen(Qt::white, 1));
        p.drawRect(-15, -30, 30, 60);

        // Draw Wall (Red Line)
        int xA = -20 - distA; int xB = -20 - distB;
        p.setPen(QPen(Qt::red, 4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(xA, -30, xB, 30);

        // Wall Glow
        p.setPen(QPen(QColor(255, 0, 0, 60), 10));
        p.drawLine(xA, -30, xB, 30);

        p.setPen(QColor(100, 100, 100));
        p.setFont(QFont("Arial", 8));
        p.drawText(-80, 85, 160, 20, Qt::AlignCenter, "LEFT WALL RADAR");
    }
private:
    int distA, distB;
};

// --- MAIN WINDOW ---
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
private slots:
    void toggleConnection();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void sendCommand(QString cmd);
private:
    void setupUI();
    void logEvent(QString message);
    QTcpSocket  *tcpSocket;
    QLineEdit   *ipLineEdit;
    QPushButton *btnConnect, *btnForward, *btnBackward, *btnLeft, *btnRight, *btnStop, *btnAuto, *btnManual;
    QLabel      *statusLabel, *sensorLabel;
    QPlainTextEdit *eventLog;
    QProgressBar *barFrontLeft, *barRearLeft;
    CircularGauge *speedGauge;
    LiveMap *liveMap;
};
#endif
