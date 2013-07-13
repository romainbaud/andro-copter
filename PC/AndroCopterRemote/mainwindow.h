/*!
* \file mainwindow.h
* \brief The main part of the program (display and processing).
* \author Romain Baud
* \version 0.1
* \date 2012.11.26
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QFile>
#include <QDateTime>
#include <QKeyEvent>
#include <QDir>

#include "gamepad.h"
#include "constants.h"
#include "pid.h"

namespace Ui
{
    class MainWindow;
}

/// Enum for the messages types.
enum MessageType
{
    TEXT=0, ///< Text to display to the user.
    VIDEO_FRAME, ///< Image to display to the user.
    LOG, ///< Logfile, to save.
    CURRENT_STATE, ///< Current state, to display to the user.
    PHOTO ///< HD photo to save on the disk.
};

/// Main window of the GUI, and main loop.
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    /// Constructor.
    /// \param Parent widget. If not specified, a new window will be created.
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    /// Updates the UI to show that the phone is connected.
    void acceptConnection();

    /// Processes the data received from the phone.
    void onDataReceived();

    /// Updates the UI to show that the phone is disconnected.
    void onClientDisconnected();

    /// Gets the command values and sends them to the phone, at fixed interval.
    void computeAndSendCommands();

    /// Asks the quadcopter to stop completely its motors and regulators.
    void emergencyStop();

    /// Sends the new regulators coefficients to the phone.
    void updateReguCoefs();

    /// Sends the new regulators state to the phone.
    /// \param on Sets if the regulators should be enabled (true) or deactivated
    /// (false).
    void updateReguState(bool on);

    /// Starts or stops the logging on the phone. If it is stopped, the phone
    /// will send the log to this application, and it will be saved to a file.
    void togglePhoneLogging();

    /// Clears the messages frame.
    void clearMessagesLog();

    /// Saves the content of the message frame.
    void saveMessagesLog();

    /// Sets the current yaw as the zero point.
    /// This useful to have the quadcopter looking at the same direction of the
    /// pilot.
    void resetDeviceOrientation();

    /// Set the FPV state to the value indicated by fpvCombo.
    void setFpvState();

    /// Depending on the value of fpvSaveFramesCheckbox, ask the phone to start
    /// or stop recording. Also creates an output folder named with the current
    /// date and time.
    void setFpvRecording();

    /// Take a picture with the phone's camera.
    void takePicture();

    /// Enable or disable the "altitude lock" state.
    void setAltitudeLock();

protected:
    /// Function called when a key is pressed. This is used for the emergency
    /// stop, if the spacebar is pressed.
    void keyPressEvent(QKeyEvent *event);
    
private:
    /// Sends a message to the phone.
    /// \arg text Useful content of the message.
    void sendMessage(QString text);

    /// Get the list of all candidates IP addresses, as strings.
    /// The OS often gives several addresses, this is why this function first
    /// filter out the irrelevant addresses (empty, loopback...).
    /// @return the list of candidate IP addresses. This is up to the user to
    /// choose the good one for his application.
    QStringList getIpAddresses() const;

    /// Display a text message into the messages frame.
    /// Called when a message of type TEXT comes from the phone.
    /// \arg data Byte array representing characters to be displayed.
    void displayTextMessage(QByteArray data);

    /// Display a picture.
    /// Called when a message of type VIDEO_FRAME comes from the phone.
    /// Useful for displaying a first-person-view (FPV) images.
    /// \arg data Byte array representing an image to be displayed.
    void displayImage(QByteArray data);

    /// Saves the given logfile to a text file.
    /// Called when a message of type LOG comes from the phone.
    /// \arg data Byte array representing characters to be saved.
    void savePhoneLog(QByteArray data);

    /// Displays the current state into the charts.
    /// Called when a message of type CURRENT_STATE comes from the phone.
    /// \arg data Byte array representing characters to be interpreted as the
    /// current states.
    void displayCurrentState(QByteArray data);

    /// Save a photograph.
    /// Called when a message of type PHOTO comes from the phone.
    /// Useful to save an aerial photograph on the disk.
    /// \arg data Byte array representing an image to be saved.
    void savePhoto(QByteArray data);

    /// Pointer to the GUI elements, placed using the Qt designer.
    Ui::MainWindow *ui;

    /// Used to save the settings of the application, and to recover them after
    /// the application has been closed.
    QSettings settings;

    /// TCP server.
    /// Manages the incomming connections.
    QTcpServer server;

    /// TCP socket. Sends and receives messages.
    QTcpSocket* clientSocket;

    /// Size (in bytes) of the currently incomming message.
    unsigned int inMessageSize;

    /// Timer which will call the computeAndSendCommands() method regularly.
    QTimer updateTimer;

    /// Gets the data from the selected gamepad.
    Gamepad gamepad;

    /// Current mean thrust. It has to be stored, because as it is only
    /// incremented or decremented by the stick value.
    double currentThrust;

    /// Current yaw target angle. It has to be stored, because as it is only
    /// incremented or decremented by the stick value.
    double currentYaw;

    /// Time (in milliseconds) since the regulator activation request.
    /// TODO: not needed anymore ?
    QTime regulatorStartRequestTime;

    /// Previous connection state of the gamepad. This avoids spamming the
	/// phone with emergency stops messages if the gamepad is disconnected (one
	/// is enough).
    bool gamepadWasConnected;

    /// PID of the X axis.
    Pid xPid;

    /// PID of the Y axis.
    Pid yPid;

    /// PID of the Z axis.
    Pid zPid;

    /// Timer to measure the FPS of the video frames.
    QTime time;

    /// Filename of the folder used for storing the FPV frames.
    QString fpvSaveFolder;

    /// FPV frame number, used to name the frames, if they are stored.
    int fpvFrameNumber;

    /// Reception rate of the FPV frames, in bytes per second.
    double fpvBitrate;

    /// Reception rate of the FPV frames, in frames per second.
    double fpvFramerate;
};

#endif // MAINWINDOW_H
