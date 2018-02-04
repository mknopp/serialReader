#include <QSettings>
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
		QSettings settings;
		double getReducedAtmosphericPressure(double adout) const;
		void updateRrdDatabase(int gamma, double pressure);
		void storeResultInDatabase(int gamma, double pressure);

	private Q_SLOTS:
		void readData();
		void rrdProcessResult(int);
};
