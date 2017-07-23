#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <QDate>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QElapsedTimer>

#include <QSerialPort>
#include <QSerialPortInfo>

#include "plotwindow.h"

namespace Ui {
class MainWindow;
}

class PlotWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_run_readings_button_clicked();

    void on_validate_MPU_button_clicked();

    void on_gyro_sens_comboBox_activated(int index);

    void on_acc_sens_comboBox_activated(int index);

    void on_filters_comboBox1_activated(int index);

    void on_filters_comboBox2_activated(int index);

    void on_save_log_button_clicked();

    void on_clear_screen_button_clicked();

    void readData();

    void handleError(QSerialPort::SerialPortError error);

    void openSerialPort();

    void closeSerialPort();

    void update_serial_ports();

    void selected_serial_port(QAction *action);

    void update_baudrate(QAction *action);

    void on_calibration_progress_bar_valueChanged(int value);

    void on_calibrate_button_clicked();

    void on_kalman_filter_checkbox_clicked(bool checked);

    bool isCalibrating() const { return calibrating;}

    bool isReading() const { return reading; }

private:
    Ui::MainWindow *ui;
    PlotWindow *plotter;

    void log(const QString &str);

    void send_instruction(const char &inst,const char &para);

    const char SET_MPU_CODE = 1;
    const char GYRO_SENS_CODE = 2;
    const char ACC_SENS_CODE = 3;
    const char SET_FILTER_CODE = 4;
    const char SET_FILTER2_CODE = 5;
    const char CALIB_CODE = 6;
    const char KALMAN_CODE = 7;
    const char RUN_READINGS_CODE = 15;
    const char STOP_READINGS_CODE = 16;

    QByteArray WAKE_UP;

    QStringList gyro_sens_list = {
        "sensitivity : 131; scale : 250 °/s",
        "sensitivity : 65.5; scale : 500 °/s",
        "sensitivity : 32.8; scale : 1000 °/s",
        "sensitivity : 16.4; scale : 2000 °/s"
    };

    QStringList acc_sens_list = {
        "sensitivity : 16384; scale : 2g",
        "sensitivity : 8192; scale : 4g",
        "sensitivity : 4096; scale : 8g",
        "sensitivity : 2048; scale : 16g"
    };

    QStringList mpu6050_filter_list = {
        "accel bandwidth : 260Hz; gyro bandwidth : 256Hz",
        "accel bandwidth : 184Hz; gyro bandwidth : 188Hz",
        "accel bandwidth : 94Hz; gyro bandwidth : 98Hz",
        "accel bandwidth : 44Hz; gyro bandwidth : 42Hz",
        "accel bandwidth : 21Hz; gyro bandwidth : 20Hz",
        "accel bandwidth : 10Hz; gyro bandwidth : 10Hz",
        "accel bandwidth : 5Hz; gyro bandwidth : 5Hz"
    };

    QStringList mpu6500_acc_filter_list = {
        "bandwidth : 1130Hz",
        "bandwidth : 460Hz",
        "bandwidth : 184Hz",
        "bandwidth : 92Hz",
        "bandwidth : 41Hz",
        "bandwidth : 20Hz",
        "bandwidth : 10Hz",
        "bandwidth : 5Hz"
    };

    QStringList mpu6500_gyro_filter_list = {
        "bandwidth : 8800Hz; Fs:32kHz",
        "bandwidth : 3600Hz; Fs:32kHz",
        "bandwidth : 250Hz; Fs : 8kHz",
        "bandwidth : 184Hz; Fs : 1kHz",
        "bandwidth : 92Hz; Fs : 1kHz",
        "bandwidth : 41Hz; Fs : 1kHz",
        "bandwidth : 20; Fs : 1kHz",
        "bandwidth : 10Hz; Fs : 1kHz",
        "bandwidth : 5Hz; Fs : 1kHz",
        "bandwidth : 3600Hz; Fs : 8kHz"
    };

    QSerialPort *serial;

    qint32 current_baudrate;

    bool calibrating,reading;

    QVector <double> qv_x,qv_y;

    QElapsedTimer *readings_timer;
};

#endif // MAINWINDOW_H
