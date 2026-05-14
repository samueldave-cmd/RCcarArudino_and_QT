#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    centralWidget->setStyleSheet("background-color: black; color: white;");

    // Header
    QHBoxLayout *connLayout = new QHBoxLayout();
    ipLineEdit = new QLineEdit("192.168.4.1");
    ipLineEdit->setStyleSheet("color: black; background: white; padding: 4px;");
    btnConnect = new QPushButton("Connect");
    connLayout->addWidget(ipLineEdit);
    connLayout->addWidget(btnConnect);
    mainLayout->addLayout(connLayout);

    statusLabel = new QLabel("Status: Disconnected");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);

    // Modes
    QHBoxLayout *modeLayout = new QHBoxLayout();
    btnAuto = new QPushButton("START AUTO (G)");
    btnManual = new QPushButton("MANUAL MODE (M)");
    btnAuto->setStyleSheet("QPushButton { background-color: #27ae60; height: 35px; font-weight: bold; border-radius: 4px; } QPushButton:pressed { background-color: #1e8449; }");
    btnManual->setStyleSheet("QPushButton { background-color: #c0392b; height: 35px; font-weight: bold; border-radius: 4px; } QPushButton:pressed { background-color: #943126; }");
    modeLayout->addWidget(btnAuto);
    modeLayout->addWidget(btnManual);
    mainLayout->addLayout(modeLayout);

    // --- MISSION CONTROL DASHBOARD ---
    QHBoxLayout *dash = new QHBoxLayout();

    // Col 1: Log
    QVBoxLayout *logCol = new QVBoxLayout();
    logCol->addWidget(new QLabel("SYSTEM EVENT LOG"));
    eventLog = new QPlainTextEdit();
    eventLog->setReadOnly(true);
    eventLog->setStyleSheet("background-color: #0a0a0a; color: #00ff00; font-family: 'Consolas'; font-size: 10px; border: 1px solid #333;");
    logCol->addWidget(eventLog);
    dash->addLayout(logCol, 2);

    // Col 2: WASD
    QVBoxLayout *ctrlCol = new QVBoxLayout();
    btnForward = new QPushButton("W"); btnBackward = new QPushButton("S");
    btnLeft = new QPushButton("A"); btnRight = new QPushButton("D"); btnStop = new QPushButton("X");
    QString s = "QPushButton { width: 45px; height: 45px; background: #222; border: 1px solid #555; font-weight: bold; } QPushButton:pressed { background: #444; }";
    btnForward->setStyleSheet(s); btnBackward->setStyleSheet(s); btnLeft->setStyleSheet(s); btnRight->setStyleSheet(s); btnStop->setStyleSheet(s);
    ctrlCol->addWidget(btnForward, 0, Qt::AlignCenter);
    QHBoxLayout *m = new QHBoxLayout();
    m->addWidget(btnLeft); m->addWidget(btnStop); m->addWidget(btnRight);
    ctrlCol->addLayout(m);
    ctrlCol->addWidget(btnBackward, 0, Qt::AlignCenter);
    dash->addLayout(ctrlCol, 1);

    // Col 3: Visualizer & Telemetry
    QVBoxLayout *teleCol = new QVBoxLayout();

    // Side-by-side Visuals
    QHBoxLayout *viz = new QHBoxLayout();
    speedGauge = new CircularGauge();
    liveMap = new LiveMap();
    QFrame *line = new QFrame(); line->setFrameShape(QFrame::VLine); line->setStyleSheet("background: #333;");
    viz->addWidget(speedGauge);
    viz->addWidget(line);
    viz->addWidget(liveMap);
    teleCol->addLayout(viz);

    sensorLabel = new QLabel("RAW: --");
    barFrontLeft = new QProgressBar(); barFrontLeft->setRange(0, 100);
    barRearLeft = new QProgressBar(); barRearLeft->setRange(0, 100);
    teleCol->addWidget(sensorLabel);
    teleCol->addWidget(new QLabel("FRONT LEFT DIST")); teleCol->addWidget(barFrontLeft);
    teleCol->addWidget(new QLabel("REAR LEFT DIST")); teleCol->addWidget(barRearLeft);
    dash->addLayout(teleCol, 2);

    mainLayout->addLayout(dash);

    // Logic
    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(btnForward, &QPushButton::pressed, [=](){ sendCommand("W"); speedGauge->setValue(100); logEvent("CMD: Forward"); });
    connect(btnBackward, &QPushButton::pressed, [=](){ sendCommand("S"); speedGauge->setValue(100); logEvent("CMD: Backward"); });
    connect(btnLeft, &QPushButton::pressed, [=](){ sendCommand("A"); logEvent("CMD: Left"); });
    connect(btnRight, &QPushButton::pressed, [=](){ sendCommand("D"); logEvent("CMD: Right"); });
    connect(btnStop, &QPushButton::clicked, [=](){ sendCommand("X"); speedGauge->setValue(0); logEvent("CMD: STOP"); });
    connect(btnForward, &QPushButton::released, [=](){ sendCommand("X"); speedGauge->setValue(0); });
    connect(btnBackward, &QPushButton::released, [=](){ sendCommand("X"); speedGauge->setValue(0); });
    connect(btnAuto, &QPushButton::clicked, [=](){ sendCommand("G"); logEvent("MODE: AUTO"); });
    connect(btnManual, &QPushButton::clicked, [=](){ sendCommand("M"); logEvent("MODE: MANUAL"); });
}

void MainWindow::logEvent(QString message) {
    eventLog->appendPlainText(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), message));
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    int k = event->key();
    if (k == Qt::Key_W) { sendCommand("W"); btnForward->setDown(true); speedGauge->setValue(100); logEvent("KEY: W"); }
    else if (k == Qt::Key_S) { sendCommand("S"); btnBackward->setDown(true); speedGauge->setValue(100); logEvent("KEY: S"); }
    else if (k == Qt::Key_A) { sendCommand("A"); btnLeft->setDown(true); logEvent("KEY: A"); }
    else if (k == Qt::Key_D) { sendCommand("D"); btnRight->setDown(true); logEvent("KEY: D"); }
    else if (k == Qt::Key_X) { sendCommand("X"); btnStop->setDown(true); speedGauge->setValue(0); logEvent("KEY: STOP"); }
    else if (k == Qt::Key_G) { sendCommand("G"); btnAuto->setDown(true); logEvent("KEY: AUTO"); }
    else if (k == Qt::Key_M) { sendCommand("M"); btnManual->setDown(true); logEvent("KEY: MANUAL"); }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    int k = event->key();
    if (k == Qt::Key_W || k == Qt::Key_S) { sendCommand("X"); btnForward->setDown(false); btnBackward->setDown(false); speedGauge->setValue(0); }
    else if (k == Qt::Key_A || k == Qt::Key_D) { sendCommand("C"); btnLeft->setDown(false); btnRight->setDown(false); }
    else if (k == Qt::Key_X || k == Qt::Key_G || k == Qt::Key_M) { btnStop->setDown(false); btnAuto->setDown(false); btnManual->setDown(false); }
}

void MainWindow::onReadyRead() {
    QString msg = QString::fromUtf8(tcpSocket->readAll()).trimmed();
    sensorLabel->setText("RAW: " + msg);
    int fa = 50, fb = 50;
    QStringList parts = msg.split('|');
    for (const QString &p : parts) {
        if (p.startsWith("FA:")) { fa = p.mid(3).toInt(); barFrontLeft->setValue(fa); }
        if (p.startsWith("FB:")) { fb = p.mid(3).toInt(); barRearLeft->setValue(fb); }
    }
    liveMap->setDistances(fa, fb);
}

void MainWindow::toggleConnection() {
    if (tcpSocket->state() == QAbstractSocket::UnconnectedState) tcpSocket->connectToHost(ipLineEdit->text(), 8888);
    else tcpSocket->disconnectFromHost();
}

void MainWindow::onConnected() { logEvent("CONNECTED"); statusLabel->setText("Connected"); btnConnect->setText("Disconnect"); }
void MainWindow::onDisconnected() { logEvent("DISCONNECTED"); statusLabel->setText("Disconnected"); btnConnect->setText("Connect"); }

void MainWindow::sendCommand(QString cmd) {
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->write(cmd.toUtf8()); tcpSocket->flush();
    }
}
