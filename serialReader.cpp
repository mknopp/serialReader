#include <QCoreApplication>

#include "Interface.h"

int main(int argc, char *argv[]) {
	QCoreApplication serialReader(argc, argv);
	
	Interface *iface = new Interface(0);

	iface->start();

	return serialReader.exec();
}
