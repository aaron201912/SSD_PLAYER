#include "mainwindow.h"

#include <QApplication>

#include <QGraphicsView>
#include <QGraphicsProxyWidget>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    //w.show();
    
    // rotate
    QGraphicsScene *scene = new QGraphicsScene;
    QGraphicsProxyWidget *widget = scene->addWidget(&w);
    widget->setRotation(30);

    QGraphicsView *view = new QGraphicsView(scene);
    //view->resize(800, 480);
    view->show();

    return a.exec();
}
