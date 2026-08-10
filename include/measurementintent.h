#ifndef MEASUREMENTINTENT_H
#define MEASUREMENTINTENT_H

#include <QString>
#include <QMetaType>

namespace oi{

/*!
 * \brief The MeasurementIntent class
 *
 * D12 (domain-model.md S3): what a Point was shot for, stamped at
 * acquisition when the operator knows it and it is free - survey
 * feature-coding. A Point carries a *list* of these (many-to-many: a point
 * shot for wall-01 may also serve as a registration target).
 *
 * Anchored to featureId, not a feature name or a live QPointer<Geometry>:
 * a name collides/orphans silently on rename (the exact class of bug
 * MeasurementConfigKey and the git-derived plugin IID already caused, see
 * domain-model.md S9.1), and a raw pointer has nowhere stable to live
 * across save/load the way an id does - every other cross-feature
 * reference in this codebase already resolves ids through
 * OiJob::getFeatureById rather than holding pointers directly.
 *
 * A fit's point input resolves to a *query* over these tags (matching
 * featureId + roleName against a function's NeededElement::roleName), not
 * a stored membership list - re-measuring re-evaluates it, which is what
 * removes manual backfilling.
 */
class MeasurementIntent{
public:
    MeasurementIntent() : featureId(-1) {}
    MeasurementIntent(int featureId, const QString &roleName) : featureId(featureId), roleName(roleName) {}

    bool operator==(const MeasurementIntent &other) const {
        return this->featureId == other.featureId && this->roleName == other.roleName;
    }

    int featureId;
    QString roleName;
};

}

Q_DECLARE_METATYPE( oi::MeasurementIntent )

#endif // MEASUREMENTINTENT_H
