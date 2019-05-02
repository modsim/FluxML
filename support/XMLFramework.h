#ifndef XMLFRAMEWORK_H
#define XMLFRAMEWORK_H

#include "XMLException.h"

namespace flux {
namespace xml {
namespace framework {

/**
 * Initialisiert das XML/DOM-Framework
 */
void initialize();

/**
 * Terminiert das XML/DOM-Framework
 */
void terminate();

} // namespace flux::xml::framework
} // namespace flux::xml
} // namespace flux

#endif

