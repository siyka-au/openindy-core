#include "tool.h"

using namespace oi;

/*!
 * \brief Tool::Tool
 * \param parent
 */
Tool::Tool(QWidget* parent) : QWidget(parent) {}

/*!
 * \brief Tool::~Tool
 */
Tool::~Tool() {}

/*!
 * \brief Tool::getMetaData
 * \return
 */
const PluginMetaData& Tool::getMetaData() const { return this->metaData; }

/*!
 * \brief Tool::getToolType
 * \return
 */
const ToolTypes& Tool::getToolType() const { return this->toolType; }

/*!
 * \brief Tool::getJob
 * \return
 */
const QPointer<OiJob>& Tool::getJob() const { return this->job; }

/*!
 * \brief Tool::setJob
 * \param job
 */
void Tool::setJob(const QPointer<OiJob>& job) { this->job = job; }

/*!
 * \brief Tool::closeEvent
 * \param event
 */
void Tool::closeEvent(QCloseEvent* event) { event->accept(); }

/*!
 * \brief Tool::customRequest
 * \param request
 */
void Tool::customRequest(OiRequestResponse* /*request*/) {
  emit this->sendMessage(QString("No custom request implementation"),
                         MessageTypes::eCriticalMessage,
                         MessageDestinations::eMessageBoxMessage);
}

/*!
 * \brief Tool::watchWindowKeyPressed
 * \param key
 */
void Tool::watchWindowKeyPressed(const Qt::Key& /*key*/) {
  emit this->sendMessage(QString("No watch window key pressed implementation"),
                         MessageTypes::eCriticalMessage,
                         MessageDestinations::eMessageBoxMessage);
}

void Tool::setCurrentJob(const QPointer<OiJob>& /*job*/) {}

/*!
 * \brief Tool::init
 */
void Tool::init() { this->toolType = ToolTypes::eUnknownTool; }

bool Tool::saveProjectEnabled() { return true; }
