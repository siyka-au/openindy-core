#ifndef GEODETICFUNCTION_H
#define GEODETICFUNCTION_H

#include "function.h"

namespace oi{

/*!
 * \brief The GeodeticFunction class
 * for future use only
 */
class OI_CORE_EXPORT GeodeticFunction : public Function
{
    Q_OBJECT

public:
    GeodeticFunction(QObject *parent = 0) : Function(parent){}

    virtual ~GeodeticFunction(){}

};

}

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a ## b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "pluginabi.h"

#define GEODETICFUNCTION_PREFIX de.openIndy.plugin.function.geodeticFunction.v
#define GeodeticFunction_iidd STR2(CONCAT2(GEODETICFUNCTION_PREFIX, OI_PLUGIN_ABI_VERSION))

#endif // GEODETICFUNCTION_H
