#include <QtWidgets/QApplication>
#include "QtFfplayer.h"
#undef main


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtFfplayer w;
    w.show();
    return a.exec();
}
