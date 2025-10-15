#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QProcess>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startCamera();
    void stopCamera();
    void captureFrame();
    void onCaptureFinished(int exitCode);

private:
    void setupUI();

    QProcess *captureProcess;
    QLabel *videoLabel;
    QPushButton *startButton;
    QPushButton *stopButton;
    QLabel *statusLabel;
    QTimer *captureTimer;
    QWidget *centralWidget;
    QString tempImagePath;
    int frameCount;
};

#endif // MAINWINDOW_H