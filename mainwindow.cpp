#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/maker_lab.png"));

    plotter = new PlotWindow(this);
    readings_timer = new QElapsedTimer();
    serial = new QSerialPort(this);

    readings_timer->start();

    ui->run_readings_button->setStyleSheet("background-color:green");

    ui->calibration_progress_bar->setValue(0);

    ui->actionConnect->setEnabled(false);
    ui->actionDisconnect->setEnabled(false);
    //Disable sending commands
    ui->mpu_comboBox->setEnabled(false);
    ui->validate_MPU_button->setEnabled(false);
    ui->gyro_sens_comboBox->setEnabled(false);
    ui->acc_sens_comboBox->setEnabled(false);
    ui->filters_comboBox1->setEnabled(false);
    ui->filters_comboBox2->setEnabled(false);
    ui->calibrate_button->setEnabled(false);
    ui->run_readings_button->setEnabled(false);
    ui->kalman_filter_checkbox->setEnabled(false);


    connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &MainWindow::handleError);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(openSerialPort()));
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->menuSerial,SIGNAL(aboutToShow()),this,SLOT(update_serial_ports()));
    connect(ui->menuSerial_ports,SIGNAL(triggered(QAction*)),this,SLOT(selected_serial_port(QAction*)));
    connect(ui->menuBaudrate,SIGNAL(triggered(QAction*)),this,SLOT(update_baudrate(QAction*)));

    current_baudrate = 9600; //Default baudrate


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::log(const QString &str)
{
    ui->text_browser->append("["+ QTime::currentTime().toString()+"] - "+str);
}

void MainWindow::on_run_readings_button_clicked()
{
    if(ui->run_readings_button->text()[0] == 'R')
    {
        send_instruction(RUN_READINGS_CODE,0);
        reading = true;
        readings_timer->restart();
        plotter->clearData();
        plotter->showMaximized();
        log("Started reading!");
        ui->run_readings_button->setStyleSheet("background-color:red");
        ui->run_readings_button->setText("Stop readings");
        ui->run_readings_button->setStatusTip("Stop reading from gyro/acc");
    }
    else
    {
        send_instruction(STOP_READINGS_CODE,0);
        reading = false;
        readings_timer->restart();
        log("Stopped reading!");
        ui->run_readings_button->setStyleSheet("background-color:green");
        ui->run_readings_button->setText("Run readings");
        ui->run_readings_button->setStatusTip("Start reading from gyro/acc");
    }
}

void MainWindow::on_validate_MPU_button_clicked()
{
    send_instruction(1,ui->mpu_comboBox->currentIndex()+1);

    log("Validated "+ui->mpu_comboBox->currentText());
    ui->gyro_sens_comboBox->clear();
    ui->gyro_sens_comboBox->insertItems(0,gyro_sens_list);
    ui->gyro_sens_comboBox->setEnabled(true);
    ui->acc_sens_comboBox->clear();
    ui->acc_sens_comboBox->insertItems(0,acc_sens_list);
    ui->acc_sens_comboBox->setEnabled(true);

    ui->calibrate_button->setEnabled(true);
    ui->run_readings_button->setEnabled(true);
    ui->kalman_filter_checkbox->setEnabled(true);


    if(ui->mpu_comboBox->currentIndex() == 0)
    {
        ui->filters_comboBox2->setEnabled(false);
        ui->filters_comboBox1->clear();
        ui->filters_comboBox1->setEnabled(true);
        ui->filters_comboBox1->insertItems(0,mpu6050_filter_list);
    };
    if(ui->mpu_comboBox->currentIndex() == 1)
    {
        ui->filters_comboBox1->setEnabled(true);
        ui->filters_comboBox1->clear();
        ui->filters_comboBox1->insertItems(0,mpu6500_acc_filter_list);
        ui->filters_comboBox2->setEnabled(true);
        ui->filters_comboBox2->clear();
        ui->filters_comboBox2->insertItems(0,mpu6500_gyro_filter_list);
    }
}

void MainWindow::on_gyro_sens_comboBox_activated(int index)
{
    send_instruction(GYRO_SENS_CODE,index+1);
    log("Selected gyro sensibility "+ui->gyro_sens_comboBox->itemText(index));
}

void MainWindow::on_acc_sens_comboBox_activated(int index)
{
    send_instruction(ACC_SENS_CODE,index+1);
    log("Selected acc sensibility "+ui->acc_sens_comboBox->itemText(index));
}

void MainWindow::on_filters_comboBox1_activated(int index)
{
    send_instruction(SET_FILTER_CODE,index+1);
    if(ui->mpu_comboBox->currentIndex() == 0)
        log("Selected filter configuration "+ui->filters_comboBox1->itemText(index));
    else if(ui->mpu_comboBox->currentIndex() == 1)
        log("Selected accelerometer filter bandwidth "+ui->filters_comboBox1->itemText(index));
}

void MainWindow::on_filters_comboBox2_activated(int index)
{
    send_instruction(SET_FILTER2_CODE,index+1);
    log("Selected gyro filter bandwidth " + ui->filters_comboBox2->itemText(index));
}

void MainWindow::on_save_log_button_clicked()
{
    ///Save log with current time and date as name in the current working directory
    QString filename = QTime::currentTime().toString()+ " --- " + QDate::currentDate().toString() + " log";
    if(!QFile::exists(filename))
    {
        QFile logfile(filename+".txt");
        if(!logfile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            log("Couldn't open file to save into");
        }
        else
        {
            QTextStream out(&logfile);
            out << ui->text_browser->toPlainText();
            out.flush();
            logfile.close();
            log("Saved log");
        }
    }
}

void MainWindow::on_clear_screen_button_clicked()
{
    if(QMessageBox::question(this,
                             "Confirmation",
                             "Are you sure you want to clear log?",
                             QMessageBox::Yes,QMessageBox::Cancel)
            == QMessageBox::Yes)
        ui->text_browser->setText("");
}

void MainWindow::readData()
{
    if(isCalibrating())
    {
        QByteArray res = serial->readAll();
        QString test = "";

        foreach(auto c, res)
        {
            //test += (char)c; //DEBUG
            if(c == '.')
            {
                if(ui->calibration_progress_bar->value()!=100)
                {
                    ui->calibration_progress_bar->setValue(ui->calibration_progress_bar->value()+1);
                }
                else
                {
                    ui->calibration_progress_bar->setValue(1);
                }
            }else{
                log("Error");
            }
        }
        //log(test); //DEBUG
    }
    else if(isReading())
    {
        QString read_text = "";
        QByteArray res = serial->readLine();
        foreach(auto c , res)
        {
            read_text += (char)c;
        }
        double value = read_text.toDouble();
        //log(read_text); //DEBUG
        plotter->addPoint(readings_timer->elapsed(),value);
        plotter->plot();

    }

}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    QString error_text;
    switch(error)
    {
    case QSerialPort::NoError:
        return;
    case QSerialPort::DeviceNotFoundError:
        error_text = "Device "+ serial->portName()+" not found";
        break;
    case QSerialPort::PermissionError:
        error_text = "Permission error";
        break;
    case QSerialPort::OpenError:
        error_text = "Device "+ serial->portName()+" already open by another process";
        break;
    case QSerialPort::NotOpenError:
        error_text = "Couldn't open device "+ serial->portName();
        break;
    case QSerialPort::WriteError:
        error_text = "Error occured while writing to device "+ serial->portName();
        break;
    case QSerialPort::ReadError:
        error_text = "Error occured while reading from device "+ serial->portName();
        break;
    case QSerialPort::TimeoutError:
        error_text = "Communication timed out";
        break;
    case QSerialPort::ResourceError:
        error_text = "Resource error (Eg. device unexpectedly removed)";
        closeSerialPort();
        break;
    default:
        error_text = "Unknown error";
    }

    QMessageBox::critical(this, tr("Error"),error_text);
}

void MainWindow::openSerialPort()
{
    log("Serial port selected: "+serial->portName());
    log("with baudrate : "+QString::number(current_baudrate));
    serial->setBaudRate(current_baudrate);
    if (serial->open(QIODevice::ReadWrite)) {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);

        //Enable sending commands
        ui->mpu_comboBox->setEnabled(true);
        ui->validate_MPU_button->setEnabled(true);
        ui->gyro_sens_comboBox->setEnabled(true);
        ui->acc_sens_comboBox->setEnabled(true);
        ui->filters_comboBox1->setEnabled(true);
        ui->filters_comboBox2->setEnabled(true);
        ui->calibrate_button->setEnabled(true);
        ui->run_readings_button->setEnabled(true);
        ui->kalman_filter_checkbox->setEnabled(true);

        ui->statusBar->showMessage("Connected to "+serial->portName());
        log("Connected to "+serial->portName());
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
    }
    log("Opened serial port");
    if(serial->clear())
    {
        log("Cleared buffer");
    }
    else
    {
        log("Couldn't clear buffer");
    }

}

void MainWindow::closeSerialPort()
{
    if(serial->isOpen())
        serial->close();
    ui->actionDisconnect->setEnabled(false);
    ui->actionConnect->setEnabled(true);

    //Disable sending commands
    ui->mpu_comboBox->setEnabled(false);
    ui->validate_MPU_button->setEnabled(false);
    ui->gyro_sens_comboBox->setEnabled(false);
    ui->acc_sens_comboBox->setEnabled(false);
    ui->filters_comboBox1->setEnabled(false);
    ui->filters_comboBox2->setEnabled(false);
    ui->calibrate_button->setEnabled(false);
    ui->run_readings_button->setEnabled(false);
    ui->kalman_filter_checkbox->setEnabled(false);

    log("Closed serial port");
}

void MainWindow::update_serial_ports()
{
    ui->menuSerial_ports->clear();
    auto available = QSerialPortInfo::availablePorts();
    for(int i = 0;i<available.size();i++)
    {
        ui->menuSerial_ports->addAction(available[i].portName());
    }
    foreach(QAction *action,ui->menuSerial_ports->actions())
    {
        action->setCheckable(true);
    }
}

void MainWindow::selected_serial_port(QAction *action)
{
    log("Selected "+action->text()+" with baudrate "+QString::number(current_baudrate));
    serial->setPortName(action->text());
    ui->actionConnect->setEnabled(true);
}

void MainWindow::update_baudrate(QAction *action)
{
    switch(action->text().toInt())
    {
        case 9600:
            log("Changed baudrate to 9600");
            current_baudrate = QSerialPort::Baud9600;
        break;
        case 19200:
            log("Changed baudrate to 19200");
            current_baudrate = QSerialPort::Baud19200;
        break;
        case 38400:
            log("Changed baudrate to 38400");
            current_baudrate = QSerialPort::Baud38400;
        break;
        case 57600:
            log("Changed baudrate to 57600");
            current_baudrate = QSerialPort::Baud57600;
        break;
        case 115200:
            log("Changed baudrate to 115200");
            current_baudrate = QSerialPort::Baud115200;
        break;
        default:
            log("Error: Baudrate not supported : "+action->text());
    }
}

void MainWindow::send_instruction(const char &inst, const char &para)
{
    serial->write(&inst,1);
    serial->write(&para,1);
}

void MainWindow::on_calibration_progress_bar_valueChanged(int value)
{
    if(value == 100)
    {
        ui->run_readings_button->setEnabled(true);
        ui->calibrate_button->setEnabled(true);
        calibrating = false;
        log("Calibration complete");
    }
}

void MainWindow::on_calibrate_button_clicked()
{
    ui->run_readings_button->setEnabled(false);
    ui->calibrate_button->setEnabled(false);
    ui->calibration_progress_bar->setValue(0);
    calibrating = true;
    send_instruction(CALIB_CODE,1); //aazejgalzkejgalzkejgalkzejglakzezkeagojzkzegjalzekglazke PARAMETRE
}

void MainWindow::on_kalman_filter_checkbox_clicked(bool checked)
{
    if(checked)
    {
        send_instruction(KALMAN_CODE,ui->kalman_spinBox->value());
        log("Applying kalman filter!");
    }
    else
    {
        send_instruction(KALMAN_CODE,101);
        log("Disabling kalman filter");
    }
}
