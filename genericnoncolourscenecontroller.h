#ifndef GENERICNONCOLOURSCENECONTROLLER_H
#define GENERICNONCOLOURSCENECONTROLLER_H

#include <QObject>

class GenericDeviceNonColourSceneController : public QObject
{
    Q_OBJECT
public:
    explicit GenericDeviceNonColourSceneController(QObject *parent = nullptr);

signals:

};

#endif // GENERICNONCOLOURSCENECONTROLLER_H
