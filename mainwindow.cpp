
// Test cursor movement
void MainWindow::on_testCursorButton_clicked()
{
    if (!m_testTimer) {
        m_testTimer = new QTimer(this);
        connect(m_testTimer, &QTimer::timeout, this, &MainWindow::onTestTimerTimeout);
    }

    if (m_testTimer->isActive()) {
        m_testTimer->stop();
        ui->testCursorButton->setText("Start Test");
        appendLog("Test cursor movement stopped", "blue");
    } else {
        m_testTimer->start(50); // 20Hz
        ui->testCursorButton->setText("Stop Test");
        appendLog("Test cursor movement started - circular pattern", "green");
    }
}

void MainWindow::onTestTimerTimeout()
{
    static double angle = 0.0;
    
    // Generate circular movement
    double radius = 15.0;
    double mouseX = radius * qCos(angle);
    double mouseY = radius * qSin(angle);
    
    angle += 0.1; // Increment angle
    if (angle > 2 * M_PI) {
        angle = 0.0;
    }
    
    // Move cursor
    if (m_cursorCanvas) {
        m_cursorCanvas->moveCursor(static_cast<int>(mouseX), static_cast<int>(mouseY));
    }
    
    // Update data label
    QString dataText = QString("Mouse X: %1\nMouse Y: %2\nScroll: 0\n(TEST MODE)")
                           .arg(mouseX, 0, 'f', 2)
                           .arg(mouseY, 0, 'f', 2);
    ui->airMouseDataLabel->setText(dataText);
}
