#include <QSerialPort>
#include <QSettings>
#include <QSqlDatabase>
#include <QString>
#include <QTimer>

class Interface: public QObject {
	Q_OBJECT

	public:
		Interface(QObject *parent = 0);
		virtual ~Interface();
		void start();

	private:
		QSerialPort *serialPort;
		QSettings settings;
		double getReducedAtmosphericPressure(double adout) const;
		void updateRrdDatabase(int gamma, double pressure);
		void storeResultInDatabase(int gamma, double pressure);

	private Q_SLOTS:
		void handleSerialError(QSerialPort::SerialPortError error);
		void readData();
		void rrdProcessResult(int);
};
