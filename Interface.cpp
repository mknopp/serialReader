#include <cmath>
#include <iostream>
#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QProcess>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>
#include <unistd.h>

#include "Interface.h"

#define CURR_TIME_STRING QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss: ").toStdString()

Interface::Interface(QObject *parent) : QObject(parent) {
	QString portName = settings.value("port/device", "/dev/ttyUSB0").toString();

	serialPort = new QSerialPort(portName, this);
	serialPort->setBaudRate(QSerialPort::Baud115200);
	if (!serialPort->open(QIODevice::ReadOnly)) {
		qCritical() << "Failed to open serial port";
		exit(-1);
	}
}

Interface::~Interface() {
	serialPort->close();
	delete serialPort;
}

void Interface::start() {
	connect(serialPort, SIGNAL(readyRead()), this, SLOT(readData()));
#if QT_VERSION >= 0x050800
	connect(serialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
			this, SLOT(handleSerialError(QSerialPort::SerialPortError)));
#else
	connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)),
			this, SLOT(handleSerialError(QSerialPort::SerialPortError)));
#endif
}

void Interface::readData() {
	QByteArray bytes;
	QList<QByteArray> results;

	qint64 avail = serialPort->bytesAvailable();

	bytes.resize(avail);

	// Check if there is a complete line in the input buffer;
	// wait for new data otherwise
	serialPort->peek(bytes.data(), bytes.size());
	bool lineFinished = false;
	for (int i=0; i<bytes.size(); ++i) {
		if (bytes[i] == '\n') {
			lineFinished = true;
			// When does the first line break happen?
			avail = i+1;
			break;
		}
	}
	if (!lineFinished) {
		//std::cerr << CURR_TIME_STRING
		//	<< "Couldn't read a complete line, waiting for next read\n";
		return;
	}

	// Receiving more than one line is also an issue, so stop at the first '\n'
	bytes.resize(avail);
	serialPort->read(bytes.data(), bytes.size());

	results = bytes.split(';');

	bool gammaOk = false, pressureOk = false, alarmOk = false;
	double adout, pressure;
	int measuredValue, alarm;

	if (results.length() >= 3) {
		measuredValue = results.at(0).simplified().toInt(&gammaOk);
		adout = results.at(1).simplified().toDouble(&pressureOk);
		pressure = getReducedAtmosphericPressure(adout);
		alarm = results.at(2).simplified().toInt(&alarmOk);
	}

	if (gammaOk && pressureOk && alarmOk) {
		// The unit is broken and returns a second (empty) result imidiately
		// after the first one, so we do a simple sanity check here
		if (measuredValue > 0 && pressure > 800) {
			std::cout << CURR_TIME_STRING << "Storing measured values "
				<< measuredValue << " nGy/h, " << pressure
				<< " hPa and alarm " << alarm << " into database." << std::endl;

			storeResultInDatabase(measuredValue, pressure, alarm);
			updateRrdDatabase(measuredValue, pressure);

			// Clean up the database connection, has to be out of scope
			QSqlDatabase::removeDatabase("log");
		}
		else
			std::cout << CURR_TIME_STRING << "Ignoring measured values "
				<< measuredValue << " nGy/h, " << pressure << " hPa and alarm "
				<< alarm << "." << std::endl;
	}
	else
		std::cerr << CURR_TIME_STRING << bytes.constData();
}

double Interface::getReducedAtmosphericPressure(double adout) const {
	// first convert the A/D readout to absolute pressure
	// see MPX4115A datasheet, p. 6
	double voltageFromSensor = adout/4096.0 * 2.72 + 3.37;
	double absolutePressure = 10 * (voltageFromSensor / 5.08 + 0.095) / 0.009;

	// see http://de.wikipedia.org/wiki/Barometrische_H%C3%B6henformel#Reduktion_auf_Meeresh.C3.B6he
	const double g0 = 9.80665;
	const double R = 287.05;
	const double altitude = 556;
	const double tempC = 20;
	const double a = 0.0065;
	const double Ch = 0.12;

	double Ehat = 18.2194 * (1.0463 - exp(-0.0666 * tempC));

	return absolutePressure * exp(g0 * altitude /
				(R * (tempC + 273.15 + Ch * Ehat + a * altitude / 2)));
}

void Interface::updateRrdDatabase(int gamma, double pressure) {
	static QProcess *rrdProcess = Q_NULLPTR;

	// We want to remove stale processes, so check that there is only one
	if (rrdProcess == Q_NULLPTR)
		rrdProcess = new QProcess(this);
	else {
		rrdProcess->kill();
		delete rrdProcess;
		rrdProcess = new QProcess(this);
	}

	rrdProcess->setProcessChannelMode(QProcess::ForwardedChannels);
	connect(rrdProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
			this, SLOT(rrdProcessResult(int)));

	QStringList arguments;
	arguments << "update" << "/home/gamma/gamma.rrd" << QString("N:%1:%2").arg(gamma).arg(pressure);
	qDebug() << "About to execute rrdtool" << arguments;
	rrdProcess->start("rrdtool", arguments);
}

void Interface::storeResultInDatabase(int gamma, double pressure, int alarm) {
	QSettings settings;
	QSqlDatabase database = QSqlDatabase::addDatabase("QMYSQL", "log");

	database.setHostName(settings.value("database/host").toString());
	database.setDatabaseName(settings.value("database/database").toString());
	database.setUserName(settings.value("database/username").toString());
	database.setPassword(settings.value("database/password").toString());

	if (!database.open())
		qCritical() << "Failed to establish database connection:" << database.lastError().text();

	QSqlQuery query(database);
	query.prepare("INSERT INTO log (date, value, pressure, alarm) VALUES ( NOW(), :value, :pressure, :alarm)");
	query.bindValue(":value", QVariant(gamma));
	query.bindValue(":pressure", QVariant(pressure));
	query.bindValue(":alarm", QVariant(alarm));
	query.exec();

	database.close();
}

void Interface::handleSerialError(QSerialPort::SerialPortError error) {
	if (error == QSerialPort::ReadError) {
		std::cerr << CURR_TIME_STRING
			<< "serial read failed; reinitializing ...\n";
		serialPort->close();
		sleep(3);
		while (!serialPort->open(QSerialPort::ReadOnly)) {
			std::cerr << "Waiting 3 seconds for "
				<< serialPort->portName().toStdString()
				<< " to become ready.\n";
			sleep(3);
		}
	}
}

void Interface::rrdProcessResult(int exitCode) {
	if(exitCode != 0) {
		qWarning() << "rrdTool exited with a non-zero exit code!";
	}
}
