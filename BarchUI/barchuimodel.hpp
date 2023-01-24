#ifndef BARCHUIMODEL_HPP
#define BARCHUIMODEL_HPP

#include <cstddef> // for std::size_t

#include <QtCore/QtCore>
#include <QtQuick/QtQuick>

#include <QQmlEngine>

namespace BarchUI::Internal {

struct EncoderTask;
struct DecoderTask;

} // namespace BarchUI::Internal

namespace BarchUI {

/// File represents a file that can be transcoded.
class File : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString name READ name CONSTANT)
  QString name() const noexcept { return m_fileInfo.fileName(); }

  Q_PROPERTY(qint64 size READ size CONSTANT)
  qint64 size() const noexcept { return m_fileInfo.size(); }

  Q_PROPERTY(std::size_t progress READ progress NOTIFY progressChanged)
  std::size_t progress() const noexcept { return m_progress; }
  Q_SIGNAL void progressChanged();

  Q_PROPERTY(QObject *currentFile READ currentFile CONSTANT)
  QObject *currentFile() noexcept { return this; }

public:
  // File is a light-weight wrapper around a QFileInfo.
  // The QQmlEngine is passed in as a parameter because I feel super duper lazy.
  // Anyways, all that makes the job done.
  File(QFileInfo fileInfo, QQmlEngine &engine);

  Q_INVOKABLE void transcode();

signals:
  // success is emitted every time this file has been transcoded into its
  // alternative representation.
  void success();

  // error is emitted every time this file cannot be transcoeded for one reason
  // or another.
  void error(QString what);

private:
  friend Internal::EncoderTask;
  friend Internal::DecoderTask;

  QFileInfo m_fileInfo;

  QQmlEngine *m_qmlEngine;

  // Holds the current progress as a value from 0 to 100 (percents).
  std::size_t m_progress = 0;

  void encode();
  void decode();

  // Called by the error handlers to reset the progress so that the UI gets
  // properly updated.
  void resetProgress();
};

} // namespace BarchUI

#endif // BARCHUIMODEL_HPP
