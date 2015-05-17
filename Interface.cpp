#include <cmath>
#include <iostream>
#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <unistd.h>

#include "Interface.h"

#define CURR_TIME_STRING QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss: ").toStdString()

// TODO implement system daemon process so that we do not require screen

Interface::Interface(QObject *parent) :
		QObject(parent),
		portName("/dev/ttyUSB0") {
	pSettings.BaudRate = BAUD115200;
	pSettings.DataBits = DATA_8;
	pSettings.Parity = PAR_NONE;
	pSettings.StopBits = STOP_1;
	pSettings.FlowControl = FLOW_OFF;
	pSettings.Timeout_Millisec = 0;

	serialPort = new QextSerialPort(portName, pSettings, QextSerialPort::EventDriven);
	if (!serialPort->open(QextSerialPort::ReadOnly)) {
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
}

void Interface::readData() {
	QByteArray bytes;
	QList<QByteArray> results;

	qint64 avail = serialPort->bytesAvailable();

	// -1 bytes are returned when an error occurred, this usually means the USB devices was disconnected
	if (avail < 0) {
		std::cerr << CURR_TIME_STRING << "serial read failed; reinitializing ...\n";
		serialPort->close();
		sleep(10);
		while (!serialPort->open(QextSerialPort::ReadOnly)) {
			std::cerr << "Waiting 10 seconds for " << portName.toStdString() << " to become ready.\n";
			sleep(10);
		}
		return;
	}

	bytes.resize(avail);

	// Check if there is a complete line in the input buffer and wait for new data otherwise
	serialPort->peek(bytes.data(), bytes.size());
	bool lineFinished = false;
	for (int i=0; i<bytes.size(); ++i) {
		if (bytes[i] == '\n') {
			lineFinished = true;
			break;
		}
	}
	if (!lineFinished) {
		std::cerr << CURR_TIME_STRING << "Couldn't read a complete line, waiting for next read\n";
		return;
	}

	serialPort->read(bytes.data(), bytes.size());

	results = bytes.split(';');

	bool gammaOk = false, pressureOk = false;
	double adout, pressure;
	int measuredValue;

	if (results.length() >= 2) {
		measuredValue = results.at(0).simplified().toInt(&gammaOk);
		adout = results.at(1).simplified().toDouble(&pressureOk);
		pressure = getReducedAtmosphericPressure(adout);
	}

	if (gammaOk && pressureOk) {
		if (measuredValue > 80 && pressure > 900) {
			std::cout << CURR_TIME_STRING << "Storing measured values "
					<< measuredValue << " nGy/h and " << pressure << " hPa into database.\n";
			QSqlDatabase database = QSqlDatabase::addDatabase("QMYSQL", "log");
			database.setHostName("database.home.h2o");
			database.setDatabaseName("gamma_measurement");
			database.setUserName("router");
			database.setPassword("CH3COO-");

			if (!database.open())
				qCritical() << "Failed to establish database connection:" << database.lastError().text();

			QSqlQuery query(database);
			query.prepare("INSERT INTO log (date, value, pressure) VALUES ( NOW(), :value, :pressure)");
			query.bindValue(":value", QVariant(measuredValue));
			query.bindValue(":pressure", QVariant(pressure));
			query.exec();

			database.close();
		} else
			std::cout << CURR_TIME_STRING << "Ignoring measured values "
				<< measuredValue << " nGy/h and " << pressure << " hPa.\n";
	} else
		std::cerr << CURR_TIME_STRING << bytes.constData();

	QSqlDatabase::removeDatabase("log");
}

double Interface::getReducedAtmosphericPressure(double adout) const {
	// first convert the A/D readout to absolute pressure
	// see MPX4115A datasheet, p. 6
	double absolutePressure = 10 * (adout / 0x3ff + 0.095) / 0.009;

	// see http://de.wikipedia.org/wiki/Barometrische_H%C3%B6henformel#Reduktion_auf_Meeresh.C3.B6he
	const double g0 = 9.80665;
	const double R = 287.05;
	const double altitude = 556;
	const double tempC = 20;
	const double a = 0.0065;
	const double Ch = 0.12;

	double Ehat = 18.2194 * (1.0463 - exp(-0.0666 * tempC));

	return absolutePressure	* exp(g0 * altitude	/ (R * (tempC + 273.15 + Ch * Ehat + a * altitude / 2)));
}
