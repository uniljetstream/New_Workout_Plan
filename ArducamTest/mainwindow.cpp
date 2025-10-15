#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , captureProcess(nullptr)
    , videoLabel(nullptr)
    , captureTimer(nullptr)
    , frameCount(0)
{
    tempImagePath = QDir::tempPath() + "/arducam_frame.jpg";
    setupUI();
}

MainWindow::~MainWindow()
{
    if (captureTimer && captureTimer->isActive()) {
        captureTimer->stop();
    }
    if (captureProcess && captureProcess->state() == QProcess::Running) {
        captureProcess->kill();
        captureProcess->waitForFinished();
    }
    QFile::remove(tempImagePath);
}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    statusLabel = new QLabel("Arducam IMX219 준비", this);
    statusLabel->setStyleSheet("QLabel { background-color: #4CAF50; color: white; padding: 10px; font-size: 14pt; font-weight: bold; }");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);

    videoLabel = new QLabel(this);
    videoLabel->setMinimumSize(960, 540);
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setStyleSheet("QLabel { background-color: black; border: 2px solid #2196F3; color: white; }");
    videoLabel->setText("카메라 화면이 여기에 표시됩니다\n\n'카메라 시작' 버튼을 눌러주세요");
    QFont labelFont = videoLabel->font();
    labelFont.setPointSize(16);
    videoLabel->setFont(labelFont);
    mainLayout->addWidget(videoLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("🎥 카메라 시작", this);
    stopButton = new QPushButton("⏹ 카메라 정지", this);
    
    startButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px; font-size: 12pt; font-weight: bold; border-radius: 5px; } "
                               "QPushButton:hover { background-color: #45a049; }");
    stopButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 10px; font-size: 12pt; font-weight: bold; border-radius: 5px; } "
                              "QPushButton:hover { background-color: #da190b; }");
    stopButton->setEnabled(false);
    
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    mainLayout->addLayout(buttonLayout);

    captureTimer = new QTimer(this);
    captureProcess = new QProcess(this);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::startCamera);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopCamera);
    connect(captureTimer, &QTimer::timeout, this, &MainWindow::captureFrame);
    connect(captureProcess, QOverload<int>::of(&QProcess::finished), this, &MainWindow::onCaptureFinished);

    setWindowTitle("Arducam IMX219 테스트 (Jetson Nano)");
    resize(1000, 700);
}

void MainWindow::startCamera()
{
    statusLabel->setText("✅ 카메라 실행 중 - IMX219 (1920x1080)");
    statusLabel->setStyleSheet("QLabel { background-color: #4CAF50; color: white; padding: 10px; font-size: 14pt; font-weight: bold; }");
    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    frameCount = 0;
    
    captureTimer->start(100);
    captureFrame();
}

void MainWindow::stopCamera()
{
    captureTimer->stop();
    
    if (captureProcess->state() == QProcess::Running) {
        captureProcess->kill();
        captureProcess->waitForFinished();
    }
    
    videoLabel->clear();
    videoLabel->setText("카메라 화면이 여기에 표시됩니다\n\n'카메라 시작' 버튼을 눌러주세요");
    statusLabel->setText("⏹ 카메라 정지됨");
    statusLabel->setStyleSheet("QLabel { background-color: #9E9E9E; color: white; padding: 10px; font-size: 14pt; font-weight: bold; }");
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
}

void MainWindow::captureFrame()
{
    if (captureProcess->state() == QProcess::Running) {
        return;
    }

    QString command = "gst-launch-1.0";
    QStringList args;
    args << "nvarguscamerasrc" << "num-buffers=1"
         << "!" << "video/x-raw(memory:NVMM),width=1920,height=1080,framerate=30/1"
         << "!" << "nvjpegenc"
         << "!" << "filesink" << QString("location=%1").arg(tempImagePath);

    captureProcess->start(command, args);
}

void MainWindow::onCaptureFinished(int exitCode)
{
    if (exitCode == 0 && QFile::exists(tempImagePath)) {
        QPixmap pixmap(tempImagePath);
        if (!pixmap.isNull()) {
            pixmap = pixmap.scaled(videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            videoLabel->setPixmap(pixmap);
            frameCount++;
            statusLabel->setText(QString("✅ 카메라 실행 중 - 프레임: %1").arg(frameCount));
        }
    } else if (exitCode != 0) {
        statusLabel->setText("⚠ 프레임 캡처 실패");
        statusLabel->setStyleSheet("QLabel { background-color: #FF9800; color: white; padding: 10px; font-size: 14pt; font-weight: bold; }");
    }
}