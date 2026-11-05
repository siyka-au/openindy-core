#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QtTest>

#include "constructfunction.h"
#include "exchangeinterface.h"
#include "fitfunction.h"
#include "generatefeaturefunction.h"
#include "geodeticfunction.h"
#include "lasertracker.h"
#include "objecttransformation.h"
#include "plugin.h"
#include "pluginabi.h"
#include "specialfunction.h"
#include "systemtransformation.h"
#include "totalstation.h"

/*!
 * Guards the plugin ABI version declared in include/plugin/pluginabi.h.
 *
 * A hand-declared version number is only as good as the discipline that
 * maintains it, and the dangerous failure is not a spurious bump - it is a
 * MISSED one. A missed bump lets a plugin binary built against the old
 * contract load successfully (the IID still matches) and then call through a
 * vtable that has moved underneath it. That is memory corruption, at runtime,
 * on a user's machine, with no diagnostic.
 *
 * So this test fingerprints the declared interface surface - every header in
 * include/plugin/ - and fails when it changes without the version moving.
 * The digest is stored alongside the version it belongs to, so refreshing it
 * without bumping is a visible, deliberate act in the diff rather than
 * something that happens silently.
 *
 * Comments and blank lines are excluded from the fingerprint, so documenting
 * an interface does not trip the guard. Everything else does, deliberately -
 * this errs toward false positives, because a false positive costs one
 * command and a false negative costs a corrupted measurement session.
 *
 * WHAT IT DOES NOT COVER: layout changes to domain types that cross the
 * boundary but live outside include/plugin/ (Geometry, Position, Reading,
 * Observation, ...). Those are just as ABI-breaking. Judgement is still
 * required; see the bump criteria in pluginabi.h.
 */
class PluginAbiTest : public QObject {
  Q_OBJECT

 private slots:
  void interfaceHeadersMatchTheDeclaredAbiVersion();
  void everyInterfaceIidCarriesTheAbiVersion();

 private:
  static QString normalize(const QString& source);
  static QString digestOfInterfaceHeaders(QStringList& filesOut);
};

/*!
 * \brief PluginAbiTest::normalize
 * Strips comments and insignificant whitespace so that documentation changes
 * do not read as interface changes.
 */
QString PluginAbiTest::normalize(const QString& source) {
  QString text = source;

  // block comments first, so a "//" inside one is not mistaken for a line
  // comment
  text.remove(QRegularExpression(
      "/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption));
  text.remove(QRegularExpression("//[^\n]*"));

  QStringList significantLines;
  foreach (const QString& rawLine, text.split('\n')) {
    const QString line = rawLine.simplified();
    if (!line.isEmpty()) {
      significantLines.append(line);
    }
  }

  return significantLines.join('\n');
}

/*!
 * \brief PluginAbiTest::digestOfInterfaceHeaders
 * \param filesOut receives the hashed file names, in the order hashed
 * \return hex sha256 over every header in include/plugin/ except pluginabi.h
 */
QString PluginAbiTest::digestOfInterfaceHeaders(QStringList& filesOut) {
  const QDir pluginIncludeDir(QString("%1/include/plugin").arg(SOURCEDIR));

  QStringList headers;
  QDirIterator it(pluginIncludeDir.absolutePath(), QStringList() << "*.h",
                  QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString absolute = it.next();
    const QString relative = pluginIncludeDir.relativeFilePath(absolute);
    // pluginabi.h itself is excluded: bumping the version edits it, which
    // would change the digest on every bump and make the pairing circular
    if (relative == QString("pluginabi.h")) {
      continue;
    }
    headers.append(relative);
  }

  // sorted, so the digest does not depend on filesystem iteration order
  headers.sort();

  QCryptographicHash hash(QCryptographicHash::Sha256);
  foreach (const QString& relative, headers) {
    QFile file(pluginIncludeDir.absoluteFilePath(relative));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return QString();
    }
    // the file name participates, so renaming or moving an interface header
    // counts as a change even when its contents are untouched
    hash.addData(relative.toUtf8());
    hash.addData(
        PluginAbiTest::normalize(QString::fromUtf8(file.readAll())).toUtf8());
  }

  filesOut = headers;
  return QString::fromLatin1(hash.result().toHex());
}

void PluginAbiTest::interfaceHeadersMatchTheDeclaredAbiVersion() {
  QStringList hashedFiles;
  const QString actualDigest =
      PluginAbiTest::digestOfInterfaceHeaders(hashedFiles);
  QVERIFY2(!actualDigest.isEmpty(),
           "could not read the plugin interface headers");
  QVERIFY2(!hashedFiles.isEmpty(),
           "found no plugin interface headers to fingerprint");

  const QString digestPath = QString("%1/plugin-abi.digest").arg(SOURCEDIR);

  QFile digestFile(digestPath);
  QString recordedVersion;
  QString recordedDigest;
  if (digestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    const QStringList parts =
        QString::fromUtf8(digestFile.readAll()).simplified().split(' ');
    if (parts.size() == 2) {
      recordedVersion = parts.at(0);
      recordedDigest = parts.at(1);
    }
  }

  const QString currentVersion = QString::number(OI_PLUGIN_ABI_VERSION);
  const QString expectedFileContents =
      QString("%1 %2\n").arg(currentVersion).arg(actualDigest);

  if (recordedVersion == currentVersion && recordedDigest == actualDigest) {
    return;
  }

  // write what the file should say, so the fix is a copy rather than a
  // transcription of a 64-character hex string from a test log
  const QString suggestionPath =
      QString("%1/plugin-abi.digest.actual").arg(QDir::currentPath());
  QFile suggestion(suggestionPath);
  if (suggestion.open(QIODevice::WriteOnly | QIODevice::Text)) {
    suggestion.write(expectedFileContents.toUtf8());
    suggestion.close();
  }

  const QString message =
      QString(
          "\n"
          "The plugin interface headers no longer match the recorded "
          "fingerprint for\n"
          "plugin ABI version %1.\n"
          "\n"
          "  recorded: version %2, digest %3\n"
          "  actual:   version %1, digest %4\n"
          "\n"
          "If you changed the plugin contract (see the bump criteria in\n"
          "include/plugin/pluginabi.h), bump OI_PLUGIN_ABI_VERSION and then "
          "run:\n"
          "\n"
          "    cp %5 %6\n"
          "\n"
          "If the change genuinely cannot break an existing plugin binary, "
          "refresh\n"
          "the digest with the same command without bumping - but do that "
          "knowingly,\n"
          "because the diff is the only record that you decided it was safe.\n")
          .arg(currentVersion)
          .arg(recordedVersion.isEmpty() ? QString("<none recorded>")
                                         : recordedVersion)
          .arg(recordedDigest.isEmpty() ? QString("<none recorded>")
                                        : recordedDigest)
          .arg(actualDigest)
          .arg(suggestionPath)
          .arg(digestPath);

  QFAIL(qPrintable(message));
}

/*!
 * Every plugin interface must key its IID off the ABI version. One that does
 * not would keep a constant IID across a genuine ABI break - it would keep
 * loading, and keep being wrong.
 */
void PluginAbiTest::everyInterfaceIidCarriesTheAbiVersion() {
  const QDir pluginIncludeDir(QString("%1/include/plugin").arg(SOURCEDIR));
  const QString suffix = QString::number(OI_PLUGIN_ABI_VERSION);

  // a representative IID from each interface family
  const QMap<QString, QString> iids = {
      {"Plugin", Plugin_iidd},
      {"Sensor", Sensor_iidd},
      {"LaserTracker", LaserTracker_iidd},
      {"TotalStation", TotalStation_iidd},
      {"Function", Function_iid},
      {"FitFunction", FitFunction_iidd},
      {"ConstructFunction", ConstructFunction_iidd},
      {"SystemTransformation", SystemTransformation_iidd},
      {"ObjectTransformation", ObjectTransformation_iidd},
      {"GeodeticFunction", GeodeticFunction_iidd},
      {"SpecialFunction", SpecialFunction_iidd},
      {"GenerateFeatureFunction", GenerateFeatureFunction_iidd},
      {"Simulation", Simulation_iidd},
      {"BundleAdjustment", BundleAdjustment_iidd},
      {"Tool", Tool_iidd},
      {"Exchange", Exchange_iidd},
      {"ExchangeSimpleAscii", ExchangeSimpleAscii_iidd},
      {"ExchangeDefinedFormat", ExchangeDefinedFormat_iidd}};

  for (auto it = iids.constBegin(); it != iids.constEnd(); ++it) {
    QVERIFY2(
        it.value().endsWith(suffix),
        qPrintable(QString("%1's IID (%2) does not end with the ABI version %3")
                       .arg(it.key())
                       .arg(it.value())
                       .arg(suffix)));
    QVERIFY2(
        it.value().startsWith("de.openIndy.plugin"),
        qPrintable(
            QString("%1's IID (%2) is not in the de.openIndy.plugin namespace")
                .arg(it.key())
                .arg(it.value())));
  }

  // and they must be distinct, or two interface categories would be
  // mutually castable
  QSet<QString> distinct;
  foreach (const QString& iid, iids.values()) {
    distinct.insert(iid);
  }
  QCOMPARE(distinct.size(), iids.size());
}

QTEST_APPLESS_MAIN(PluginAbiTest)
#include "tst_pluginabi.moc"
