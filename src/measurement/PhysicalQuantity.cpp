/*
 * measurement/PhysicalQuantity.cpp is part of Brewtarget, and is copyright the following
 * authors 2021-2022:
 * - Matt Young <mfsy@yahoo.com>
 *
 * Brewtarget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Brewtarget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "measurement/PhysicalQuantity.h"

#include <QDebug>

#include "utils/EnumStringMapping.h"

namespace {
   EnumStringMapping const physicalQuantityToName {
      {QObject::tr("Mass")          , Measurement::PhysicalQuantity::Mass          },
      {QObject::tr("Volume")        , Measurement::PhysicalQuantity::Volume        },
      {QObject::tr("Time")          , Measurement::PhysicalQuantity::Time          },
      {QObject::tr("Temperature")   , Measurement::PhysicalQuantity::Temperature   },
      {QObject::tr("Color")         , Measurement::PhysicalQuantity::Color         },
      {QObject::tr("Density")       , Measurement::PhysicalQuantity::Density       },
      {QObject::tr("Mixed")         , Measurement::PhysicalQuantity::Mixed         },
      {QObject::tr("DiastaticPower"), Measurement::PhysicalQuantity::DiastaticPower}
   };
}

QString Measurement::getDisplayName(Measurement::PhysicalQuantity physicalQuantity) {
   auto returnValue = physicalQuantityToName.enumToString(physicalQuantity);
   // It's a coding error if we don't find a result!
   if (!returnValue) {
      qCritical() << Q_FUNC_INFO << "No mapping defined for PhysicalQuantity #" << static_cast<int>(physicalQuantity);
      Q_ASSERT(false); // Stop here on debug builds
   }
   return *returnValue;
}
