#ifndef QUAZIP_QUAGZIPFILE_H
#define QUAZIP_QUAGZIPFILE_H

#include <QIODevice>
#include "quazip_global.h"

#include <zlib.h>

class QuaGzipFilePrivate;

class QUAZIP_EXPORT QuaGzipFile: public QIODevice {
  Q_OBJECT
public:
  QuaGzipFile();
  QuaGzipFile(QObject *parent);
  QuaGzipFile(const QString &fileName, QObject *parent = NULL);
  virtual ~QuaGzipFile();
  void setFileName(const QString& fileName);
  QString getFileName() const;
  virtual bool isSequential() const;
  virtual bool open(QIODevice::OpenMode mode);
  virtual bool open(int fd, QIODevice::OpenMode mode);
  virtual bool flush();
  virtual void close();
protected:
  virtual qint64 readData(char *data, qint64 maxSize);
  virtual qint64 writeData(const char *data, qint64 maxSize);
private:
    // not implemented by design to disable copy
    QuaGzipFile(const QuaGzipFile &that);
    QuaGzipFile& operator=(const QuaGzipFile &that);
    QuaGzipFilePrivate *d;
};

#endif // QUAZIP_QUAGZIPFILE_H
