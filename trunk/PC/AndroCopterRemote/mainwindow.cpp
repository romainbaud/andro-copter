#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <cmath>

const double PI = 3.14159265;

Q_DECLARE_METATYPE(QList<double>)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    xPid(-MAX_PITCH_ROLL_TARGET_ANGLE, MAX_PITCH_ROLL_TARGET_ANGLE, 0.0, true),
    yPid(-MAX_PITCH_ROLL_TARGET_ANGLE, MAX_PITCH_ROLL_TARGET_ANGLE, 0.0, true),
    zPid(0.0, (double)MAX_THRUST, 0.0, true) // Add A_PRIORI_THRUST later.
{
    ui->setupUi(this);

    setWindowTitle(APP_NAME);

    // Setup the TCP server.
    connect(&server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

    if(!server.listen(QHostAddress::Any, IN_PORT))
    {
        QMessageBox::critical(this, "Error", "Can't listen on port " + QString::number(IN_PORT) + ".");
        exit(0);
    }

    onClientDisconnected();

    // Get the gamepad.
    QStringList gamepads = gamepad.getGamepadsList();
    //qDebug() << gamepads;

    if(gamepads.length() == 1)
        gamepad.startMonitoring(0);
    else if(gamepads.length() > 1)
    {
        QString chosenItem = QInputDialog::getItem(this, APP_NAME,
                                                   "Choose a gamepad.",
                                                   gamepads, 0, false);

        gamepad.startMonitoring(gamepads.indexOf(chosenItem));

        gamepadWasConnected = true;

        // Disable the sliders for mouse/keyboard control, to avoid
        // conflicting with the gamepad commands.
        ui->thrustSlider->setEnabled(false);
        ui->yawSlider->setEnabled(false);
        ui->pitchSlider->setEnabled(false);
        ui->rollSlider->setEnabled(false);
    }
    else
    {
        QMessageBox::warning(this, tr("Warning"), tr("No gamepad found!"));
        gamepadWasConnected = false;
    }

    // Setup the timer.
    updateTimer.setSingleShot(false);
    updateTimer.start(UPDATE_PERIOD_MS);
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(computeAndSendCommands()));

    // Retrieve the previously saved regulators coefficients.
    qRegisterMetaType<QList<double> >("charList");
    qRegisterMetaTypeStreamOperators<QList<double> >("charList");

    QList<double> defaultList;
    QList<double> coefs = settings.value("regulators_coefficients", QVariant::fromValue(defaultList)).value< QList<double> >();

    if(coefs.size() == 12) // Previous values exist.
    {
        ui->reguCoefYawP->setValue(coefs[0]);
        ui->reguCoefYawI->setValue(coefs[1]);
        ui->reguCoefYawD->setValue(coefs[2]);
        ui->reguCoefPitchP->setValue(coefs[3]);
        ui->reguCoefPitchI->setValue(coefs[4]);
        ui->reguCoefPitchD->setValue(coefs[5]);
        ui->reguCoefRollP->setValue(coefs[6]);
        ui->reguCoefRollI->setValue(coefs[7]);
        ui->reguCoefRollD->setValue(coefs[8]);
        ui->reguCoefAltitudeP->setValue(coefs[9]);
        ui->reguCoefAltitudeI->setValue(coefs[10]);
        ui->reguCoefAltitudeD->setValue(coefs[11]);
    }
    else // No previous values exist, set to default values.
    {
        ui->reguCoefYawP->setValue(3.0);
        ui->reguCoefYawI->setValue(0.0);
        ui->reguCoefYawD->setValue(0.0);
        ui->reguCoefPitchP->setValue(0.8);
        ui->reguCoefPitchI->setValue(0.0);
        ui->reguCoefPitchD->setValue(0.15);
        ui->reguCoefRollP->setValue(0.8);
        ui->reguCoefRollI->setValue(0.0);
        ui->reguCoefRollD->setValue(0.15);
        ui->reguCoefRollP->setValue(20.0);
        ui->reguCoefRollI->setValue(0.0);
        ui->reguCoefRollD->setValue(0.1);
    }

    // Other initializations.
    currentThrust = 0;
    currentYaw = 0;
    clientSocket = 0;
    fpvFrameNumber = 0;
    clientSocket = 0;

    ui->yawGraphic->setup(50, 180.0, 20.0);
    ui->pitchGraphic->setup(50, 40.0, 20.0);
    ui->rollGraphic->setup(50, 40.0, 20.0);
    ui->altitudeGraphic->setup(50, 3, 255.0);

    // Connect the signals to the slot functions.
    connect(ui->reguCoefYawP, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefYawI, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefYawD, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefPitchP, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefPitchI, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefPitchD, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefRollP, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefRollI, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefRollD, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefAltitudeP, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefAltitudeI, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));
    connect(ui->reguCoefAltitudeD, SIGNAL(editingFinished()), this, SLOT(updateReguCoefs()));

    connect(ui->regulatorsOnCheckBox, SIGNAL(toggled(bool)), SLOT(updateReguState(bool)));
    connect(ui->loggingCheckbox, SIGNAL(clicked()), this, SLOT(togglePhoneLogging()));
    connect(ui->emergencyButton, SIGNAL(clicked()), this, SLOT(emergencyStop()));
    connect(ui->clearLogButton, SIGNAL(clicked()), this, SLOT(clearMessagesLog()));
    connect(ui->saveLogButton, SIGNAL(clicked()), this, SLOT(saveMessagesLog()));
    connect(ui->resetDeviceYawButton, SIGNAL(clicked()), this, SLOT(resetDeviceOrientation()));
    connect(ui->altitudeLockCheckbox, SIGNAL(toggled(bool)), this, SLOT(setAltitudeLock()));

    connect(ui->fpvCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setFpvState()));
    connect(ui->fpvSaveFramesCheckbox, SIGNAL(toggled(bool)), this, SLOT(setFpvRecording()));
    connect(ui->fpvTakePictureButton, SIGNAL(clicked()), this, SLOT(takePicture()));

    // Reload the address the phone should connect to.
    onClientDisconnected();

    // Listen to the keypresses.
    setFocusPolicy(Qt::StrongFocus);
}

MainWindow::~MainWindow()
{
    // Save the regulators coefficients.
    QList<double> coefs;
    coefs << ui->reguCoefYawP->value() << ui->reguCoefYawI->value() << ui->reguCoefYawD->value()
          << ui->reguCoefPitchP->value() << ui->reguCoefPitchI->value() << ui->reguCoefPitchD->value()
          << ui->reguCoefRollP->value() << ui->reguCoefRollI->value() << ui->reguCoefRollD->value()
          << ui->reguCoefAltitudeP->value() << ui->reguCoefAltitudeI->value() << ui->reguCoefAltitudeD->value();
    settings.setValue("regulators_coefficients", QVariant::fromValue(coefs));

    // Delete all the widgets.
    delete ui;
}

void MainWindow::acceptConnection()
{
    clientSocket = server.nextPendingConnection();

    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onDataReceived()));
    connect(clientSocket, SIGNAL(disconnected()),
            this, SLOT(onClientDisconnected()));

    inMessageSize = 0;
    ui->yawGraphic->clearGraph();
    ui->pitchGraphic->clearGraph();
    ui->rollGraphic->clearGraph();

    // Update the UI.
    ui->statusLabel->setText("Connected to: " +
                             clientSocket->peerAddress().toString() + ":" +
                             QString::number(clientSocket->peerPort()));

    ui->statusLabel->setStyleSheet("color: green;");

    ui->regulatorsGroup->setEnabled(true);

    // Send the regulators parameters.
    updateReguCoefs();
}

void MainWindow::onDataReceived()
{
    while((clientSocket != 0) && (clientSocket->bytesAvailable() >= MESSAGE_SIZE_SIZE))
    {
        // Get the size of the message, if not acquired yet.
        // The size is given by the first 32 bits (4 bytes) of the message.
        if(inMessageSize == 0)
        {
            if(clientSocket->bytesAvailable() < MESSAGE_SIZE_SIZE)
                return; // The size has not been received entirely.
            else
            {
                const QByteArray ba = clientSocket->read(MESSAGE_SIZE_SIZE);

                // Conversion des octets reçus en int.
                const unsigned char* bytes = reinterpret_cast<const unsigned char*>(ba.data());
                inMessageSize = ((bytes[0]<<24)|(bytes[1]<<16)|(bytes[2]<<8)|(bytes[3]));
            }
        }

        // If the whole message arrived, it is now possible to read it.
        if(clientSocket->bytesAvailable() >= inMessageSize)
        {
            // The first byte is the signification of the message.
            int type = clientSocket->read(1)[0];

            // Read the rest of the message, according to its type.
            QByteArray data = clientSocket->read(inMessageSize-1);

            switch(type)
            {
            case TEXT: // Display the text message to the user.
                displayTextMessage(data);
                break;

            case VIDEO_FRAME: // Display the image to the user.
                displayImage(data);
                break;

            case LOG: // Save the log to a text file.
                savePhoneLog(data);
                break;

            case CURRENT_STATE: // Update the states chart.
                displayCurrentState(data);
                break;

            case PHOTO: // Save the photo.
                savePhoto(data);
                break;

            default: // Error.
                qDebug() << "Unexpected message type:" << type;
                break;
            }

            // Ready to receive a new message.
            inMessageSize = 0;
        }
        else
            break;
    }
}

void MainWindow::displayTextMessage(QByteArray data)
{
    QString textMessage(data);

    QDateTime now = QDateTime::currentDateTime();
    ui->logEdit->appendPlainText(now.toString("(yyyy.MM.dd hh.mm.ss.zzz) ")
                                 + textMessage);
}

void MainWindow::displayImage(QByteArray data)
{
    // Create a QPixmap from the byte array.
    QPixmap pixmap;
    pixmap.loadFromData(data);
    ui->fpvVideoLabel->setPixmap(pixmap);

    // Compute the bitrate and the number of frames per second.
    double elapsedTime = (double)time.restart();

    if(elapsedTime > 0)
    {
        fpvBitrate = FPV_RATE_LPF * fpvBitrate + (1.0-FPV_RATE_LPF) * data.size() / elapsedTime;
        fpvFramerate = FPV_RATE_LPF * fpvFramerate + (1.0-FPV_RATE_LPF) * 1000.0 / elapsedTime;

        // Affichage de l'estimation du débit et du FPS.
        if(elapsedTime > 0)
        {
            ui->fpvStatusLabel->setText("Data reception at "
                                        + QString::number(fpvBitrate, 'f', 0)
                                        + " ko/s ("
                                        + QString::number(fpvFramerate, 'f', 0)
                                        + " fps).");
        }
        else
            ui->fpvStatusLabel->setText("Data reception.");
    }

    // Save the frame if needed.
    if(ui->fpvSaveFramesCheckbox->isChecked() && QFile::exists(fpvSaveFolder))
    {
        pixmap.save(fpvSaveFolder + "\\frame_" +
                    QString::number(fpvFrameNumber).rightJustified(9, '0')
                    + ".jpg", "JPG");
        fpvFrameNumber++;
    }
}

void MainWindow::savePhoneLog(QByteArray data)
{
    QString filename = QString("../logs/log(%1).txt").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss"));
    QFile file(filename);

    if(file.open(QFile::WriteOnly | QFile::Text))
    {
        file.write(data);
        ui->logEdit->appendPlainText(QString("Phone logfile saved: ") + filename);
    }
    else
        ui->logEdit->appendPlainText("Can't write the phone log to file!");
}

void MainWindow::displayCurrentState(QByteArray data)
{
    QString str(data);

    QStringList words = str.split(' ');

    if(words.size() == 16)
    {
        int time = words[0].toInt();
        double currentYaw = words[1].toDouble();
        double targetYaw = words[2].toDouble();
        double yawCommand = words[3].toDouble();
        double currentPitch = words[4].toDouble();
        double targetPitch = words[5].toDouble();
        double pitchCommand = words[6].toDouble();
        double currentRoll = words[7].toDouble();
        double targetRoll = words[8].toDouble();
        double rollCommand = words[9].toDouble();
        double batteryVoltage = words[10].toDouble();
        double temperature = words[11].toDouble();
        bool regulatorEnabled = words[12].toInt() != 0;
        double currentAltitude = words[13].toDouble();
        double targetAltitude = words[14].toDouble();
        double altitudeCommand = words[15].toDouble();

        double batteryPercent = (batteryVoltage-MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE-MIN_BATTERY_VOLTAGE) * 100.0;

        ui->yawGraphic->nextStep(time, currentYaw, targetYaw, yawCommand);
        ui->pitchGraphic->nextStep(time, currentPitch, targetPitch, pitchCommand);
        ui->rollGraphic->nextStep(time, currentRoll, targetRoll, rollCommand);
        ui->altitudeGraphic->nextStep(time, currentAltitude, targetAltitude, altitudeCommand);

        ui->currentYawLabel->setText(QString::number(currentYaw));
        ui->currentPitchLabel->setText(QString::number(currentPitch));
        ui->currentRollLabel->setText(QString::number(currentRoll));
        ui->currentAltitudeLabel->setText(QString::number(currentAltitude));

        ui->currentYawCommandLabel->setText(QString::number(yawCommand));
        ui->currentPitchCommandLabel->setText(QString::number(pitchCommand));
        ui->currentRollCommandLabel->setText(QString::number(rollCommand));
        ui->currentAltitudeCommandLabel->setText(QString::number(altitudeCommand));

        if(batteryVoltage > 1.0)
            ui->currentBatteryLabel->setText(QString::number(batteryVoltage, 'g', 4) + " (" + QString::number(batteryPercent) + "%)");
        else
            ui->currentBatteryLabel->setText("0");

        ui->currentTemperatureLabel->setText(QString::number((int)temperature));
        ui->regulatorStateLabel->setText(regulatorEnabled ? "ON" : "OFF");

        if(!regulatorEnabled &&
           regulatorStartRequestTime.msecsTo(QTime::currentTime()) > REGU_ON_STATE_WAIT_TIME_MS)
        {
            ui->regulatorsOnCheckBox->setChecked(false);
        }

        ui->currentAltitudeLabel->setText(QString::number(currentAltitude));
    }
    else
        qDebug() << "displayCurrentState(): bad number of words:" << words.size();
}

void MainWindow::savePhoto(QByteArray data)
{
    QString filename = QString("../pictures/pic(%1).jpg").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss"));
    QFile file(filename);

    if(file.open(QFile::WriteOnly))
    {
        file.write(data);
        ui->logEdit->appendPlainText(QString("Picture saved: ") + filename);
    }
    else
        ui->logEdit->appendPlainText("Can't write the picture to file!");
}

void MainWindow::onClientDisconnected()
{
    // Get all the possible IP addresses.
    QStringList ipsList = getIpAddresses();
    QString ipsString = "Please connect the phone to one of the following IP addresses:";

    for(int i=0; i<ipsList.size(); i++)
        ipsString += "\n    -" + ipsList[i] + ":" + QString::number(IN_PORT);

    ui->statusLabel->setText(ipsString);

    ui->statusLabel->setStyleSheet("color: red;");

    ui->regulatorsOnCheckBox->setChecked(false);
    ui->regulatorsGroup->setEnabled(false);
}

void MainWindow::sendMessage(QString text)
{
    if(clientSocket != 0 && clientSocket->isOpen())
    {
        text.append("\n");
        clientSocket->write(text.toLatin1());
    }
}

QStringList MainWindow::getIpAddresses() const
{
    QStringList ipStrings;

    foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces())
    {
        if (interface.flags().testFlag(QNetworkInterface::IsRunning))
        {
            foreach (QNetworkAddressEntry entry, interface.addressEntries())
            {
                if (interface.hardwareAddress() != "00:00:00:00:00:00" &&
                    entry.ip().toString().contains(".") &&
                    entry.ip().toString() != "127.0.0.1")
                {
                    ipStrings << entry.ip().toString();
                }
            }
        }
    }

    return ipStrings;
}

void MainWindow::computeAndSendCommands()
{
    // Do not compute a command if the regulators are supposed to be OFF.
    if(!ui->regulatorsOnCheckBox->isChecked())
        return;

    // Get the data from the gamepad, or from the spinners if there is no
    // gamepad connected.
    QVector<double> axes = gamepad.getAxes();

    double pitchAngle, rollAngle;

    if(axes.isEmpty() || !gamepad.isGamepadStillConnected())
    {
        if(gamepadWasConnected)
        {
            // The gamepad has just disconnected, this is very dangerous, so
            // we stop the quadcopter.
            ui->logEdit->appendPlainText("### The gamepad has been disconnected! ###");
            emergencyStop();
            gamepadWasConnected = false;
            currentThrust = 0;

            return;
        }
        else
        {
            // There is no gamepad, so the sliders are used instead.
            currentThrust = (double) ui->thrustSlider->value();
            currentYaw = ((double)ui->yawSlider->value()) / ((double)ui->yawSlider->maximum()) * YAW_AMPLITUDE;
            pitchAngle = ((double)ui->pitchSlider->value()) / ((double)ui->pitchSlider->maximum()) * PITCH_AMPLITUDE;
            rollAngle = ((double)ui->rollSlider->value()) / ((double)ui->rollSlider->maximum()) * ROLL_AMPLITUDE;
        }
    }
    else // Everything is OK with the gamepad, get the commands from it.
    {
        gamepadWasConnected = true;

        // Update the thrust.
        if(fabs(axes[THRUST_AXIS]) > DEAD_ZONE_GAMEPAD)
        {
            currentThrust -= (int)(axes[THRUST_AXIS] / GP_AXIS_AMPLITUDE * THRUST_VARSPEED);

            if(currentThrust < 0) // The thrust has saturation.
                currentThrust = 0;
            else if(currentThrust > MAX_THRUST)
                currentThrust = MAX_THRUST;
        }

        // Update the yaw angle.
        if((!ui->lockYawTargetCheckBox->isChecked()) && abs(axes[YAW_AXIS]) > DEAD_ZONE_GAMEPAD)
        {
            currentYaw += axes[YAW_AXIS] / GP_AXIS_AMPLITUDE * YAW_VARSPEED;

            if(currentYaw >= 180) // The yaw angle is circular.
                currentYaw -= 360;
            else if(currentYaw < -180)
                currentYaw += 360;
        }

        // Set the other angles.
        pitchAngle = axes[PITCH_AXIS] / GP_AXIS_AMPLITUDE * PITCH_AMPLITUDE;
        rollAngle = axes[ROLL_AXIS] / GP_AXIS_AMPLITUDE * ROLL_AMPLITUDE;

        QVector<bool> buttons = gamepad.getButtons();

        // Emergency stop ("safe" state). The propellers should not move, until the
        // regulators are explicitely restarted.
        if(buttons[B_BUTTON])
            emergencyStop();

        // Set the mean thrust to zero. The regulators are still active, so the
        // propeller may continue rotating !
        if(buttons[X_BUTTON] || buttons[LEFT_TRIGGER] || buttons[RIGHT_TRIGGER])
            currentThrust = 0;

        // Enable/Disable the "altitude lock" mode.
        if(buttons[Y_BUTTON])
            ui->altitudeLockCheckbox->setChecked(true);

        if(buttons[A_BUTTON])
            ui->altitudeLockCheckbox->setChecked(false);

        if(buttons[RIGHT_STICK_BUTTON])
            takePicture();
    }

    // Update the bars and the labels.
    ui->thrustSlider->setValue(currentThrust);
    ui->yawSlider->setValue((int)(currentYaw/YAW_AMPLITUDE*100.0));
    ui->pitchSlider->setValue((int)(pitchAngle/PITCH_AMPLITUDE*100.0));
    ui->rollSlider->setValue((int)(rollAngle/ROLL_AMPLITUDE*100.0));

    ui->thrustLabel->setText(QString::number((int)currentThrust));
    ui->yawLabel->setText(QString::number(currentYaw, 'f', 1));
    ui->pitchLabel->setText(QString::number(pitchAngle, 'f', 1));
    ui->rollLabel->setText(QString::number(rollAngle, 'f', 1));

    // Send the command to the phone.
    sendMessage(QString("command ") + QString::number(currentThrust) + " "
                + QString::number(currentYaw) + " " + QString::number(pitchAngle)
                + " " + QString::number(rollAngle));
}

void MainWindow::emergencyStop()
{
    sendMessage("emergency_stop");
    currentThrust = 0.0;
    ui->regulatorsOnCheckBox->setChecked(false);
    ui->logEdit->appendPlainText("### Emergency stop! ###");
}

void MainWindow::updateReguCoefs()
{
    sendMessage("regulator_coefs " +
                QString::number(ui->reguCoefYawP->value()) + " " +
                QString::number(ui->reguCoefYawI->value()) + " " +
                QString::number(ui->reguCoefYawD->value()) + " " +
                QString::number(ui->reguCoefPitchP->value()) + " " +
                QString::number(ui->reguCoefPitchI->value()) + " " +
                QString::number(ui->reguCoefPitchD->value()) + " " +
                QString::number(ui->reguCoefRollP->value()) + " " +
                QString::number(ui->reguCoefRollI->value()) + " " +
                QString::number(ui->reguCoefRollD->value()) + " " +
                QString::number(ui->reguCoefAltitudeP->value()) + " " +
                QString::number(ui->reguCoefAltitudeI->value()) + " " +
                QString::number(ui->reguCoefAltitudeD->value()));
}

void MainWindow::updateReguState(bool on)
{
    ui->quadControlGroupBox->setEnabled(on);

    regulatorStartRequestTime = QTime::currentTime();

    if(gamepad.isGamepadStillConnected())
    {
        // Disable the spinBoxes for mouse/keyboard control, to avoid
        // conflicting with the gamepad commands.
        ui->thrustSlider->setEnabled(false);
        ui->yawSlider->setEnabled(false);
        ui->pitchSlider->setEnabled(false);
        ui->rollSlider->setEnabled(false);
    }

    if(on)
    {
        sendMessage("regulator_state on");
        ui->yawGraphic->clearGraph();
        ui->pitchGraphic->clearGraph();
        ui->rollGraphic->clearGraph();
    }
    else
    {
        currentThrust = 0.0;
        ui->thrustSlider->setValue(0);
        ui->yawSlider->setValue(0);
        ui->rollSlider->setValue(0);
        ui->pitchSlider->setValue(0);
        ui->thrustLabel->setText("0");
        ui->yawLabel->setText("0");
        ui->pitchLabel->setText("0");
        ui->rollLabel->setText("0");
        sendMessage("regulator_state off");
    }
}

void MainWindow::togglePhoneLogging()
{
    if(ui->loggingCheckbox->isChecked())
        sendMessage("log on");
    else
        sendMessage("log off");
}

void MainWindow::clearMessagesLog()
{
    ui->logEdit->clear();
}

void MainWindow::saveMessagesLog()
{
    QFile file(QString("logs/messagesLog(%1).txt").arg(QDateTime::currentDateTime()
                                                  .toString("yyyy-MM-dd-hh-mm-ss")));

    if(file.open(QFile::WriteOnly | QFile::Text))
        file.write(ui->logEdit->toPlainText().toLatin1());
    else
        QMessageBox::warning(this, "Warning", "Can't write messages log to file!");
}

void MainWindow::resetDeviceOrientation()
{
    sendMessage("orientation_reset");
}

void MainWindow::setFpvState()
{
    if(ui->fpvCombo->currentIndex() == 0)
        sendMessage("fpv stop");
    else if(ui->fpvCombo->currentIndex() == 1)
        sendMessage("fpv sd");
    else if(ui->fpvCombo->currentIndex() == 2)
        sendMessage("fpv hd");
}

void MainWindow::setFpvRecording()
{
    if(ui->fpvSaveFramesCheckbox->isChecked())
    {
        fpvSaveFolder = QString("../pictures/record_") + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
        QDir dir;
        dir.mkpath(fpvSaveFolder);
    }
}

void MainWindow::takePicture()
{
    sendMessage("take_picture");
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Go into emergency stop mode if the space bar is pressed.
    // Note that it does not work if the focus is on a widget that can handle
    // the keypresses (e.g. a QLineEdit).
    if(event->key() == Qt::Key_Space)
        emergencyStop();
}

void MainWindow::setAltitudeLock()
{
    if(ui->altitudeLockCheckbox->isChecked())
    {
        sendMessage("altitude_lock on");
    }
    else
    {
        sendMessage("altitude_lock off");
    }
}
