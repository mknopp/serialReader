#include <QCoreApplication>

#include "Interface.h"

int main(int argc, char *argv[]) {
	QCoreApplication serialReader(argc, argv);

	QCoreApplication::setOrganizationName("Martin Knopp");
	QCoreApplication::setOrganizationDomain("vala.home.arpa");
	QCoreApplication::setApplicationName("Serial Reader");
	
	Interface *iface = new Interface(0);

	iface->start();

	return serialReader.exec();
}
