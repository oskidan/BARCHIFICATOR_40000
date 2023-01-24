#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <BarchUI/barchuimodel.hpp>

// clang-format off
/*
    ___
   //_\\_
 ."\\    ".
/          \        _  _  ____  ____  ____    ____  ____    ___  ____  __  __  ____  __    ____
|           \_     ( )/ )( ___)( ___)(  _ \  (_  _)(_  _)  / __)(_  _)(  \/  )(  _ \(  )  ( ___)
|       ,--.-.)     )  (  )__)  )__)  )___/   _)(_   )(    \__ \ _)(_  )    (  )___/ )(__  )__)
 \     /  o \o\	   (_)\_)(____)(____)(__)    (____) (__)   (___/(____)(_/\/\_)(__)  (____)(____)
 /\/\  \    /_/
  (_.   `--'__)
   |     .-'  \
   |  .-'.     )
   | (  _/--.-'
   |  `.___.'
         (
 */
// clang-format on

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  /*
        Command line arguments:

        -t | --target-directory <directory>
                Specifies the target directory to use. Current directory is used
                by default.
   */
  QCommandLineParser parser;
  parser.setApplicationDescription("Test helper");
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption targetDirectoryOption(
      QStringList() << "t"
                    << "target-directory",
      QCoreApplication::translate("main", "Read files from <directory>."),
      QCoreApplication::translate("main", "directory"), QDir::currentPath());
  parser.addOption(targetDirectoryOption);
  parser.process(app);

  QQmlApplicationEngine engine;
  const QUrl url(u"qrc:/oleksii.skidan/imports/BarchViewer/main.qml"_qs);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  engine.addImportPath(":/oleksii.skidan/imports");
  engine.load(url);

  // The model is implemented by a QList of File(s).
  QDir targetDirectory(parser.value(targetDirectoryOption));
  {
    QList<QObject *> files;
    for (const QFileInfo &fileInfo : targetDirectory.entryInfoList()) {
      if (fileInfo.isFile()) {
        files.append(new BarchUI::File(fileInfo, engine));
      }
    }
    engine.rootContext()->setContextProperty("files",
                                             QVariant::fromValue(files));
  }

  // File watcher is responsible for updating the model.
  QFileSystemWatcher fsWatcher;
  QObject::connect(
      &fsWatcher, &QFileSystemWatcher::directoryChanged, &fsWatcher,
      [&engine, &targetDirectory] {
        // Make sure it can pick up the changes.
        targetDirectory.refresh();
        QList<QObject *> files;
        for (const QFileInfo &fileInfo : targetDirectory.entryInfoList()) {
          if (fileInfo.isFile()) {
            files.append(new BarchUI::File(fileInfo, engine));
          }
        }
        engine.rootContext()->setContextProperty("files",
                                                 QVariant::fromValue(files));
      });
  fsWatcher.addPath(targetDirectory.path());

  return app.exec();
}
