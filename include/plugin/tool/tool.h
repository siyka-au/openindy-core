#ifndef TOOL_H
#define TOOL_H

#include <QCloseEvent>
#include <QObject>
#include <QWidget>

#include "oijob.h"
#include "oirequestresponse.h"
#include "pluginmetadata.h"
#include "types.h"

namespace oi {

class OI_CORE_EXPORT Tool : public QWidget {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Tool)

 public:
  explicit Tool(QWidget* parent = nullptr);

  ~Tool() override;

  // ##########################
  // tool initialization method
  // ##########################

  virtual void init();

  virtual bool saveProjectEnabled();

  // #########################################
  // methods to get or set further information
  // #########################################

  [[nodiscard]] const PluginMetaData& getMetaData() const;

  [[nodiscard]] const ToolTypes& getToolType() const;

  [[nodiscard]] const QPointer<OiJob>& getJob() const;
  void setJob(const QPointer<OiJob>& job);

  // #########################
  // override QWidget methods
  // #########################

  void closeEvent(QCloseEvent* event) override;

 public slots:

  // #####################################
  // special events a tool could listen on
  // #####################################

  virtual void customXmlRequest(OiRequestResponse* request);
  virtual void watchWindowKeyPressed(const Qt::Key& key);

  // receive emitted job
  virtual void setCurrentJob(const QPointer<OiJob>& job);

 signals:

  // ############################################
  // signals to inform OpenIndy about tool issues
  // ############################################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);
  void customXmlResponse(OiRequestResponse* response);

  // ###############################
  // signals to start sensor actions
  // ###############################

  void startConnect();
  void startDisconnect();
  void startMeasurement();
  void startMove(const Reading& reading);
  void startAim();
  void startAimAndMeasure();
  void startToggleSight();
  void startInitialize();
  void startHome();
  void startCompensation();
  void startChangeMotorState();
  void startCustomAction(const QString& task);

 protected:
  // ###########################
  // input and output parameters
  // ###########################

  PluginMetaData metaData;

  ToolTypes toolType;

  QPointer<OiJob> job;  // the OpenIndy job that this tool works on
};

}  // namespace oi

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "pluginabi.h"

#define TOOL_PREFIX de.openIndy.plugin.tool.v
#define Tool_iidd STR2(CONCAT2(TOOL_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif  // TOOL_H
