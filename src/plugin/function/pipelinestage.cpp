#include "pipelinestage.h"

#include "snapshot.h"

using namespace oi;

/*!
 * \brief PipelineStage::explicitList
 * \param ids
 * \return
 */
PipelineStage PipelineStage::explicitList(const QList<int>& ids) {
  PipelineStage stage(PipelineStageType::eExplicitListStage);
  stage.explicitIds = ids;
  return stage;
}

/*!
 * \brief PipelineStage::exclude
 * D16: an exclusion always carries a reason.
 * \param elementId
 * \param reason
 * \return
 */
PipelineStage PipelineStage::exclude(int elementId, const QString& reason) {
  PipelineStage stage(PipelineStageType::eExcludeStage);
  stage.explicitExclusions.append(PipelineExclusion(elementId, reason));
  return stage;
}

/*!
 * \brief PipelineStage::exclude
 * \param exclusions
 * \return
 */
PipelineStage PipelineStage::exclude(
    const QList<PipelineExclusion>& exclusions) {
  PipelineStage stage(PipelineStageType::eExcludeStage);
  stage.explicitExclusions = exclusions;
  return stage;
}

/*!
 * \brief PipelineStage::residualOutlierRejection
 * \param sigmaMultiplier
 * \return
 */
PipelineStage PipelineStage::residualOutlierRejection(double sigmaMultiplier) {
  PipelineStage stage(PipelineStageType::eResidualOutlierRejectionStage);
  stage.sigmaMultiplier = sigmaMultiplier;
  return stage;
}

/*!
 * \brief PipelineStage::toOpenIndyXML
 * \param xmlDoc
 * \return
 */
QDomElement PipelineStage::toOpenIndyXML(QDomDocument& xmlDoc) const {
  if (xmlDoc.isNull()) {
    return QDomElement();
  }

  QDomElement stage = xmlDoc.createElement("stage");

  switch (this->type) {
    case PipelineStageType::eExplicitListStage:
      stage.setAttribute("type", "explicitList");
      for (int id : oi::snapshot(this->explicitIds)) {
        QDomElement idElem = xmlDoc.createElement("id");
        idElem.setAttribute("ref", id);
        stage.appendChild(idElem);
      }
      break;
    case PipelineStageType::eExcludeStage:
      stage.setAttribute("type", "exclude");
      for (const PipelineExclusion& excl :
           oi::snapshot(this->explicitExclusions)) {
        QDomElement exclElem = xmlDoc.createElement("excluded");
        exclElem.setAttribute("ref", excl.elementId);
        exclElem.setAttribute("reason", excl.reason);
        stage.appendChild(exclElem);
      }
      break;
    case PipelineStageType::eResidualOutlierRejectionStage:
      stage.setAttribute("type", "residualOutlierRejection");
      stage.setAttribute("sigmaMultiplier", this->sigmaMultiplier);
      break;
  }

  return stage;
}

/*!
 * \brief PipelineStage::fromOpenIndyXML
 * \param xmlElem
 * \return
 */
bool PipelineStage::fromOpenIndyXML(const QDomElement& xmlElem) {
  if (xmlElem.isNull() || !xmlElem.hasAttribute("type")) {
    return false;
  }

  const QString typeAttr = xmlElem.attribute("type");

  if (typeAttr.compare("explicitList") == 0) {
    this->type = PipelineStageType::eExplicitListStage;
    this->explicitIds.clear();

    QDomNodeList idList = xmlElem.elementsByTagName("id");
    for (int i = 0; i < idList.size(); i++) {
      QDomElement idElem = idList.at(i).toElement();
      if (idElem.hasAttribute("ref")) {
        this->explicitIds.append(idElem.attribute("ref").toInt());
      }
    }

  } else if (typeAttr.compare("exclude") == 0) {
    this->type = PipelineStageType::eExcludeStage;
    this->explicitExclusions.clear();

    QDomNodeList exclList = xmlElem.elementsByTagName("excluded");
    for (int i = 0; i < exclList.size(); i++) {
      QDomElement exclElem = exclList.at(i).toElement();
      if (exclElem.hasAttribute("ref")) {
        this->explicitExclusions.append(PipelineExclusion(
            exclElem.attribute("ref").toInt(), exclElem.attribute("reason")));
      }
    }

  } else if (typeAttr.compare("residualOutlierRejection") == 0) {
    this->type = PipelineStageType::eResidualOutlierRejectionStage;
    this->sigmaMultiplier =
        xmlElem.hasAttribute("sigmaMultiplier")
            ? xmlElem.attribute("sigmaMultiplier").toDouble()
            : 3.0;

  } else {
    return false;
  }

  return true;
}
