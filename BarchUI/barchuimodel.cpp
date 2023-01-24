#include "barchuimodel.hpp"

#include <barchlib.hpp>

#include <QQmlEngine>

//******************************************************************************
// A bunch of helper functions. They deal with error handling and filename/path
// construction.

static void throwRuntimeError(const QString &description) {
  throw std::runtime_error{description.toUtf8()};
}

static QString pathJoin(const QString &folder, const QString &file) {
  return QDir::cleanPath(folder + QDir::separator() + file);
}

static QString makeBarchFileName(const QFileInfo &fileInfo) {
  return fileInfo.baseName() + "-packed.barch";
}

static QString makeBarchPath(const QFileInfo &fileInfo) {
  return pathJoin(fileInfo.path(), makeBarchFileName(fileInfo));
}

static QString makeBmpFileName(const QFileInfo &fileInfo) {
  return fileInfo.baseName() + "-unpacked.bmp";
}

static QString makeBmpPath(const QFileInfo &fileInfo) {
  return pathJoin(fileInfo.path(), makeBmpFileName(fileInfo));
}

//******************************************************************************
// Implementation of BarchLib::Reader and BarchLib::Writer concepts. They allow
// us to load/save BARCH files.
QT_BEGIN_NAMESPACE

static void read(QFile &file, std::size_t &value) {
  std::uint64_t value64 = 0;
  if (file.read(reinterpret_cast<char *>(&value64), sizeof(value64)) ==
      sizeof(value64)) {
    value = static_cast<std::size_t>(value64);
  } else {
    // NOTE: QFile::fileName() returns actually a relative path to the file.
    QFileInfo fileInfo{file};
    throwRuntimeError(
        u"An error occurred while reading the file '%1'. Corrupt data."_qs.arg(
            fileInfo.fileName()));
  }
}

static void read(QFile &file, const std::span<std::size_t> values) {
  for (auto &value : values) { read(file, value); }
}

static void write(QFile &file, const std::size_t value) {
  std::uint64_t value64 = value;
  if (file.write(reinterpret_cast<const char *>(&value64), sizeof(value64)) !=
      sizeof(value64)) {
    // NOTE: QFile::fileName() returns actually a relative path to the file.
    QFileInfo fileInfo{file};
    throwRuntimeError(
        u"An error occurred while writing the file '%1'. I/O error: %2"_qs
            .arg(fileInfo.fileName())
            .arg(file.errorString()));
  }
}

static void write(QFile &file, const std::span<std::size_t const> values) {
  for (const auto value : values) { write(file, value); }
}

QT_END_NAMESPACE

//******************************************************************************
// These are tasks for bitmap encoding/decoding.
namespace BarchUI::Internal {

struct EncoderTask : public QRunnable {

  EncoderTask(File *file) noexcept : m_file{file} {}

  void run() override {
    try {
      m_file->encode();
    } catch (std::exception &exc) {
      m_file->resetProgress();
      emit m_file->error(QString::fromUtf8(exc.what()));
    }
  }

private:
  File *m_file;
};

struct DecoderTask : public QRunnable {

  DecoderTask(File *file) noexcept : m_file{file} {}

  void run() override {
    try {
      m_file->decode();
    } catch (std::exception &exc) {
      m_file->resetProgress();
      emit m_file->error(QString::fromUtf8(exc.what()));
    }
  }

private:
  File *m_file;
};

} // namespace BarchUI::Internal

//******************************************************************************
// The UI model itself.
namespace BarchUI {

File::File(QFileInfo fileInfo, QQmlEngine &engine)
    : m_fileInfo(std::move(fileInfo)), m_qmlEngine(&engine) {}

void File::transcode() {
  try {
    if (name().endsWith(".barch")) {
      QThreadPool::globalInstance()->start(new Internal::DecoderTask{this});
    } else {
      QThreadPool::globalInstance()->start(new Internal::EncoderTask{this});
    }
  } catch (std::exception &exc) {
    resetProgress();
    m_qmlEngine->throwError(QString::fromUtf8(exc.what()));
  }
}

void File::encode() {
  QImage image(m_fileInfo.filePath());
  if (image.isNull()) {
    throwRuntimeError(
        u"An error occurred while loading '%1'. Unknown image format."_qs.arg(
            name()));
  }
  if (!image.allGray()) {
    throwRuntimeError(
        u"An error occured while loading '%1'. This image is not grayscale."_qs
            .arg(name()));
  }
  const std::size_t width = static_cast<std::size_t>(image.width());
  const std::size_t height = static_cast<std::size_t>(image.height());
  BarchLib::Bitmap sourceBitmap{width, height};
  for (std::size_t y = 0; y < height; ++y) {
    BarchLib::MutablePixels row = sourceBitmap.rowAt(y);
    const uchar *scanLine = image.constScanLine(static_cast<int>(y));
    std::memcpy(row.data(), scanLine, row.size_bytes());
  }
  BarchLib::CompressedBitmap compressedBitmap =
      compress(sourceBitmap, [this](const std::size_t currentStep,
                                    const std::size_t totalSteps) {
        m_progress = (100 * currentStep) / totalSteps;
        emit progressChanged();
      });
  QFile barchFile(makeBarchPath(m_fileInfo));
  if (!barchFile.open(QFile::WriteOnly | QFile::NewOnly)) {
    throwRuntimeError(
        u"An error occurred while saving '%1'. Check if the file already exists."_qs
            .arg(makeBarchFileName(m_fileInfo)));
  }
  save(barchFile, compressedBitmap);
  barchFile.close();
  emit success();
}

void File::decode() {
  QFile barchFile(m_fileInfo.filePath());
  if (!barchFile.open(QFile::ReadOnly | QFile::ExistingOnly)) {
    throwRuntimeError(
        u"An error occurred while decoding '%1'. Cannot open the file."_qs.arg(
            name()));
    return;
  }
  BarchLib::CompressedBitmap compressedBitmap = BarchLib::load(barchFile);
  barchFile.close();
  BarchLib::Bitmap reconstructedBitmap =
      uncompress(compressedBitmap, [this](const std::size_t currentStep,
                                          const std::size_t totalSteps) {
        m_progress = (100 * currentStep) / totalSteps;
        emit progressChanged();
      });
  QImage image(reconstructedBitmap.width(), reconstructedBitmap.height(),
               QImage::Format_Grayscale8);
  for (std::size_t y = 0; y < reconstructedBitmap.height(); ++y) {
    BarchLib::ImmutablePixels row = reconstructedBitmap.rowAt(y);
    std::memcpy(image.scanLine(static_cast<int>(y)), row.data(),
                row.size_bytes());
  }
  QFile bmpFile(makeBmpPath(m_fileInfo));
  if (!image.save(&bmpFile, "BMP")) {
    throwRuntimeError(u"An error occurred while saving '%1'. I/O error: %2"_qs
                          .arg(makeBmpFileName(m_fileInfo))
                          .arg(bmpFile.errorString()));
  }
  bmpFile.close();
  emit success();
}

void File::resetProgress() {
  m_progress = 0;
  emit progressChanged();
}

} // namespace BarchUI
