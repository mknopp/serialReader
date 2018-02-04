#include <QProcess>
#include <QSqlDatabase>
#include <QString>
#include <QTimer>
#include "qextserialport/src/qextserialport.h"


class Interface: public QObject {
	Q_OBJECT

	public:
		Interface(QObject *parent = 0);
		virtual ~Interface();
		void start();

	private:
		PortSettings pSettings;
		QextSerialPort *serialPort;
		QProcess *rrdProcess;
		QString portName;
		double getReducedAtmosphericPressure(double adout) const;

	private Q_SLOTS:
		void readData();
		void rrdProcessResult(int);
};
