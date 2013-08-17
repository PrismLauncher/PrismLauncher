#ifndef QUAZIP_QUAZIODEVICE_H
#define QUAZIP_QUAZIODEVICE_H

#include <QIODevice>
#include "quazip_global.h"

#include <zlib.h>

class QuaZIODevicePrivate;

class QUAZIP_EXPORT QuaZIODevice: public QIODevice {
  Q_OBJECT
public:
  QuaZIODevice(QIODevice *io, QObject *parent = NULL);
  ~QuaZIODevice();
  virtual bool flush();
  virtual bool open(QIODevice::OpenMode);
  virtual void close();
  QIODevice *getIoDevice() const;
  virtual bool isSequential() const;
protected:
  virtual qint64 readData(char *data, qint64 maxSize);
  virtual qint64 writeData(const char *data, qint64 maxSize);
private:
  QuaZIODevicePrivate *d;
};
#endif // QUAZIP_QUAZIODEVICE_H
