#ifndef POINTGROUP_H
#define POINTGROUP_H

#include "group.h"
#include "pointcollection.h"

namespace oi{

class Point;

/*!
 * \brief The PointGroup class
 * A Group that holds Points, and can therefore be handed to any function
 * that asks for a PointCollection.
 *
 * Thin by design: all the grouping behaviour lives in Group, all the
 * "give me points" behaviour is the PointCollection interface, and this
 * class exists to fix the type constraint to Point and join the two. It is
 * the counterpart to PointCloud - same interface to a fit function,
 * completely different provenance:
 *
 *   PointCloud  - one scanner acquisition, bulk undifferentiated data.
 *   PointGroup  - N individually measured Points, each one a real feature
 *                 with its own observations, sigma and history, any of
 *                 which may also belong to other groups.
 *
 * A thousand reflectorless shots automated across a factory wall land here,
 * not in a point cloud: they are genuinely individual measurements that
 * would each stand up on their own, and losing that by dumping them into
 * undifferentiated bulk data would throw away exactly the pedigree that
 * makes them worth having.
 */
class OI_CORE_EXPORT PointGroup : public Group, public PointCollection
{
    friend class OiJob;
    Q_OBJECT

public:

    explicit PointGroup(QObject *parent = 0);
    PointGroup(const QString &name, QObject *parent = 0);

    PointGroup(const PointGroup &copy, QObject *parent = 0);
    PointGroup &operator=(const PointGroup &copy);

    ~PointGroup();

    //#########################
    //PointCollection interface
    //#########################

    QList<Position> collectionPoints() const override;
    int collectionPointCount() const override;

    //####################
    //point-specific access
    //####################

    //! members as Points, skipping any deleted out from under the group.
    //! Use this rather than collectionPoints() when per-point pedigree
    //! (observations, sigma, solved state) matters - that information is
    //! exactly what a PointGroup has and a PointCloud does not.
    QList<QPointer<Point> > getPoints() const;

    //#################
    //save and load XML
    //#################

    //! delegates to Group::toOpenIndyXML for everything except the "type"
    //! attribute, which ProjectExchanger reads to decide whether to
    //! reconstruct a Group or a PointGroup on load
    QDomElement toOpenIndyXML(QDomDocument &xmlDoc) const;

    //###############
    //display methods
    //###############

    QString getDisplayType() const;

};

}

#endif // POINTGROUP_H
