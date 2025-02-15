/*
 * BtTreeModel.cpp is part of Brewtarget, and is Copyright the following
 * authors 2009-2021
 * - Matt Young <mfsy@yahoo.com>
 * - Mik Firestone <mikfire@gmail.com>
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
#include "BtTreeModel.h"

#include <cstring>

#include <QAbstractItemModel>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QModelIndex>
#include <QObject>
#include <QStringBuilder>
#include <Qt>
#include <QVariant>

#include "AncestorDialog.h"
#include "BtFolder.h"
#include "BtTreeItem.h"
#include "BtTreeView.h"
#include "RecipeFormatter.h"
#include "database/ObjectStoreWrapper.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Misc.h"
#include "model/Recipe.h"
#include "model/Yeast.h"
#include "model/BrewNote.h"
#include "model/Style.h"
#include "model/Water.h"
#include "utils/BtStringConst.h"
#include "PersistentSettings.h"

namespace {
   NamedEntity * getElement(BtTreeItem::Type oType, int id) {
      switch (oType) {
         case BtTreeItem::Type::RECIPE:
            // .:TODO:. For now we just pull the raw pointer out of the shared pointer, but the rest of this code needs refactoring
            return ObjectStoreWrapper::getById<Recipe>(id).get();
         case BtTreeItem::Type::EQUIPMENT:
            return ObjectStoreWrapper::getById<Equipment>(id).get();
         case BtTreeItem::Type::FERMENTABLE:
            return ObjectStoreWrapper::getById<Fermentable>(id).get();
         case BtTreeItem::Type::HOP:
            return ObjectStoreWrapper::getById<Hop>(id).get();
         case BtTreeItem::Type::MISC:
            return ObjectStoreWrapper::getById<Misc>(id).get();
         case BtTreeItem::Type::STYLE:
            return ObjectStoreWrapper::getById<Style>(id).get();
         case BtTreeItem::Type::YEAST:
            return ObjectStoreWrapper::getById<Yeast>(id).get();
         case BtTreeItem::Type::WATER:
            return ObjectStoreWrapper::getById<Water>(id).get();
         case BtTreeItem::Type::FOLDER:
            break;
         default:
            return nullptr;
      }

      return nullptr;
   }
}

// =========================================================================
// ============================ CLASS STUFF ================================
// =========================================================================

BtTreeModel::BtTreeModel(BtTreeView * parent, TypeMasks type) :
   QAbstractItemModel(parent) {
   // Initialize the tree structure
   int items = 0;
   this->rootItem = new BtTreeItem();

   switch (type) {
      case RECIPEMASK:
         rootItem->insertChildren(items, 1, BtTreeItem::Type::RECIPE);
         connect(&ObjectStoreTyped<Recipe>::getInstance(), &ObjectStoreTyped<Recipe>::signalObjectInserted, this, &BtTreeModel::elementAddedRecipe);
         connect(&ObjectStoreTyped<Recipe>::getInstance(), &ObjectStoreTyped<Recipe>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedRecipe);
         // Brewnotes need love too!
         connect(&ObjectStoreTyped<BrewNote>::getInstance(), &ObjectStoreTyped<BrewNote>::signalObjectInserted, this, &BtTreeModel::elementAddedBrewNote);
         connect(&ObjectStoreTyped<BrewNote>::getInstance(), &ObjectStoreTyped<BrewNote>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedBrewNote);
         // And some versioning stuff, because why not?
         connect(&ObjectStoreTyped<Recipe>::getInstance(), &ObjectStoreTyped<Recipe>::signalPropertyChanged, this, &BtTreeModel::recipePropertyChanged);
         this->itemType = BtTreeItem::Type::RECIPE;
         _mimeType = "application/x-brewtarget-recipe";
         m_maxColumns = BtTreeItem::RECIPENUMCOLS;
         break;
      case EQUIPMASK:
         rootItem->insertChildren(items, 1, BtTreeItem::Type::EQUIPMENT);
         connect(&ObjectStoreTyped<Equipment>::getInstance(), &ObjectStoreTyped<Equipment>::signalObjectInserted, this, &BtTreeModel::elementAddedEquipment);
         connect(&ObjectStoreTyped<Equipment>::getInstance(), &ObjectStoreTyped<Equipment>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedEquipment);
         this->itemType = BtTreeItem::Type::EQUIPMENT;
         _mimeType = "application/x-brewtarget-recipe";
         m_maxColumns = BtTreeItem::EQUIPMENTNUMCOLS;
         break;
      case FERMENTMASK:
         rootItem->insertChildren(items, 1, BtTreeItem::Type::FERMENTABLE);
         connect(&ObjectStoreTyped<Fermentable>::getInstance(), &ObjectStoreTyped<Fermentable>::signalObjectInserted, this, &BtTreeModel::elementAddedFermentable);
         connect(&ObjectStoreTyped<Fermentable>::getInstance(), &ObjectStoreTyped<Fermentable>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedFermentable);
         this->itemType = BtTreeItem::Type::FERMENTABLE;
         _mimeType = "application/x-brewtarget-ingredient";
         m_maxColumns = BtTreeItem::FERMENTABLENUMCOLS;
         break;
      case HOPMASK:
         rootItem->insertChildren(items, 1, BtTreeItem::Type::HOP);
         connect(&ObjectStoreTyped<Hop>::getInstance(), &ObjectStoreTyped<Hop>::signalObjectInserted, this, &BtTreeModel::elementAddedHop);
         connect(&ObjectStoreTyped<Hop>::getInstance(), &ObjectStoreTyped<Hop>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedHop);
         this->itemType = BtTreeItem::Type::HOP;
         _mimeType = "application/x-brewtarget-ingredient";
         m_maxColumns = BtTreeItem::HOPNUMCOLS;
         break;
      case MISCMASK:
         rootItem->insertChildren(items, 1, BtTreeItem::Type::MISC);
         connect(&ObjectStoreTyped<Misc>::getInstance(), &ObjectStoreTyped<Misc>::signalObjectInserted, this, &BtTreeModel::elementAddedMisc);
         connect(&ObjectStoreTyped<Misc>::getInstance(), &ObjectStoreTyped<Misc>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedMisc);
         this->itemType = BtTreeItem::Type::MISC;
         _mimeType = "application/x-brewtarget-ingredient";
         m_maxColumns = BtTreeItem::MISCNUMCOLS;
         break;
      case STYLEMASK:
         rootItem->insertChildren(items, 1, BtTreeItem::Type::STYLE);
         connect(&ObjectStoreTyped<Style>::getInstance(), &ObjectStoreTyped<Style>::signalObjectInserted, this, &BtTreeModel::elementAddedStyle);
         connect(&ObjectStoreTyped<Style>::getInstance(), &ObjectStoreTyped<Style>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedStyle);
         this->itemType = BtTreeItem::Type::STYLE;
         _mimeType = "application/x-brewtarget-recipe";
         m_maxColumns = BtTreeItem::STYLENUMCOLS;
         break;
      case YEASTMASK:
         rootItem->insertChildren(items, 1, BtTreeItem::Type::YEAST);
         connect(&ObjectStoreTyped<Yeast>::getInstance(), &ObjectStoreTyped<Yeast>::signalObjectInserted, this, &BtTreeModel::elementAddedYeast);
         connect(&ObjectStoreTyped<Yeast>::getInstance(), &ObjectStoreTyped<Yeast>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedYeast);
         this->itemType = BtTreeItem::Type::YEAST;
         _mimeType = "application/x-brewtarget-ingredient";
         m_maxColumns = BtTreeItem::YEASTNUMCOLS;
         break;
      case WATERMASK:
         rootItem->insertChildren(items, 1, BtTreeItem::Type::WATER);
         connect(&ObjectStoreTyped<Water>::getInstance(), &ObjectStoreTyped<Water>::signalObjectInserted, this, &BtTreeModel::elementAddedWater);
         connect(&ObjectStoreTyped<Water>::getInstance(), &ObjectStoreTyped<Water>::signalObjectDeleted,  this, &BtTreeModel::elementRemovedWater);
         this->itemType = BtTreeItem::Type::WATER;
         _mimeType = "application/x-brewtarget-ingredient";
         m_maxColumns = BtTreeItem::WATERNUMCOLS;
         break;
      default:
         qWarning() << QString("Invalid treemask: %1").arg(type);
   }

   this->treeMask = type;
   this->parentTree = parent;
   this->loadTreeModel();
   return;
}

BtTreeModel::~BtTreeModel() {
   // Qt automatically handles the disconnection of any signals we were listening to
   delete rootItem;
   rootItem = nullptr;
}

// =========================================================================
// =================== AbstractItemModel STUFF =============================
// =========================================================================

BtTreeItem * BtTreeModel::item(const QModelIndex & index) const {
   if (index.isValid()) {
      BtTreeItem * item = static_cast<BtTreeItem *>(index.internalPointer());
      if (item) {
         return item;
      }
   }

   return rootItem;
}

int BtTreeModel::rowCount(const QModelIndex & parent) const {
   if (! parent.isValid()) {
//      qDebug() << Q_FUNC_INFO << "No parent. Root item has" << this->rootItem->childCount() << "children";
      return this->rootItem->childCount();
   }

//   qDebug() << Q_FUNC_INFO << "Parent has" << this->item(parent)->childCount() << "children";
   return this->item(parent)->childCount();
}

int BtTreeModel::columnCount(const QModelIndex & parent) const {
   Q_UNUSED(parent)
   return m_maxColumns;
}

Qt::ItemFlags BtTreeModel::flags(const QModelIndex & index) const {
   if (!index.isValid()) {
      return Qt::ItemIsDropEnabled;
   }

   return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
          Qt::ItemIsDropEnabled;
}

QModelIndex BtTreeModel::index(int row, int column, const QModelIndex & parent) const {
   BtTreeItem * pItem, *cItem;

   if (parent.isValid() && parent.column() >= columnCount(parent)) {
      return QModelIndex();
   }

   pItem = item(parent);
   cItem = pItem->child(row);

   if (cItem) {
      return createIndex(row, column, cItem);
   } else {
      return QModelIndex();
   }
}

QModelIndex BtTreeModel::parent(QModelIndex const & index) const {
   if (!index.isValid()) {
      return QModelIndex();
   }

   BtTreeItem * cItem = item(index);

   if (cItem == nullptr) {
      return QModelIndex();
   }

   BtTreeItem * pItem = cItem->parent();

   if (pItem == rootItem || pItem == nullptr) {
      return QModelIndex();
   }

   return createIndex(pItem->childNumber(), 0, pItem);
}

QModelIndex BtTreeModel::first() {
   BtTreeItem * pItem;

   // get the first item in the list, which is the place holder
   pItem = rootItem->child(0);
   if (pItem->childCount() > 0) {
      return createIndex(0, 0, pItem->child(0));
   }

   return QModelIndex();
}

QVariant BtTreeModel::data(const QModelIndex & index, int role) const {
   int maxColumns;

   if (treeMask == FOLDERMASK) {
      maxColumns = BtTreeItem::FOLDERNUMCOLS;
   } else {
      maxColumns = m_maxColumns;
   }

   if (!rootItem || !index.isValid() || index.column() < 0 || index.column() >= maxColumns) {
      return QVariant();
   }

   BtTreeItem * itm = item(index);

   QFont font;
   switch (role) {
      case Qt::ToolTipRole:
         return toolTipData(index);
      case Qt::DisplayRole:
         return itm->data(index.column());
      case Qt::DecorationRole:
         if (index.column() == 0 && itm->type() == BtTreeItem::Type::FOLDER) {
            return QIcon(":images/folder.png");
         }
         break;
      default:
         break;
   }

   return QVariant();
}

QVariant BtTreeModel::toolTipData(const QModelIndex & index) const {
   RecipeFormatter * whiskey = new RecipeFormatter();

   switch (treeMask) {
      case RECIPEMASK:
         return whiskey->getToolTip(qobject_cast<Recipe *>(thing(index)));
      case STYLEMASK:
         return whiskey->getToolTip(qobject_cast<Style *>(thing(index)));
      case EQUIPMASK:
         return whiskey->getToolTip(qobject_cast<Equipment *>(thing(index)));
      case FERMENTMASK:
         return whiskey->getToolTip(qobject_cast<Fermentable *>(thing(index)));
      case HOPMASK:
         return whiskey->getToolTip(qobject_cast<Hop *>(thing(index)));
      case MISCMASK:
         return whiskey->getToolTip(qobject_cast<Misc *>(thing(index)));
      case YEASTMASK:
         return whiskey->getToolTip(qobject_cast<Yeast *>(thing(index)));
      case WATERMASK:
         return whiskey->getToolTip(qobject_cast<Water *>(thing(index)));
      default:
         return item(index)->name();
   }
}

// This is much better, assuming the rest can be made to work
QVariant BtTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
   if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
      return QVariant();
   }

   switch (treeMask) {
      case RECIPEMASK:
         return recipeHeader(section);
      case EQUIPMASK:
         return equipmentHeader(section);
      case FERMENTMASK:
         return fermentableHeader(section);
      case HOPMASK:
         return hopHeader(section);
      case MISCMASK:
         return miscHeader(section);
      case YEASTMASK:
         return yeastHeader(section);
      case STYLEMASK:
         return styleHeader(section);
      case FOLDERMASK:
         return folderHeader(section);
      case WATERMASK:
         return waterHeader(section);
      default:
         return QVariant();
   }
}

QVariant BtTreeModel::recipeHeader(int section) const {
   switch (section) {
      case BtTreeItem::RECIPENAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::RECIPEANCCOUNT:
         return QVariant(tr("Snapshots"));
      case BtTreeItem::RECIPEBREWDATECOL:
         return QVariant(tr("Brew Date"));
      case BtTreeItem::RECIPESTYLECOL:
         return QVariant(tr("Style"));
   }

   qWarning() << QString("BtTreeModel::getRecipeHeader Bad column: %1").arg(section);
   return QVariant();
}

QVariant BtTreeModel::equipmentHeader(int section) const {
   switch (section) {
      case BtTreeItem::EQUIPMENTNAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::EQUIPMENTBOILTIMECOL:
         return QVariant(tr("Boil Time"));
   }

   qWarning() << QString("BtTreeModel::getEquipmentHeader Bad column: %1").arg(section);
   return QVariant();
}

QVariant BtTreeModel::fermentableHeader(int section) const {
   switch (section) {
      case BtTreeItem::FERMENTABLENAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::FERMENTABLECOLORCOL:
         return QVariant(tr("Color"));
      case BtTreeItem::FERMENTABLETYPECOL:
         return QVariant(tr("Type"));
   }

   qWarning() << QString("BtTreeModel::getFermentableHeader Bad column: %1").arg(section);
   return QVariant();
}

QVariant BtTreeModel::hopHeader(int section) const {
   switch (section) {
      case BtTreeItem::HOPNAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::HOPFORMCOL:
         return QVariant(tr("Type"));
      case BtTreeItem::HOPUSECOL:
         return QVariant(tr("Use"));
   }

   qWarning() << QString("BtTreeModel::getHopHeader Bad column: %1").arg(section);
   return QVariant();
}

QVariant BtTreeModel::miscHeader(int section) const {
   switch (section) {
      case BtTreeItem::MISCNAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::MISCTYPECOL:
         return QVariant(tr("Type"));
      case BtTreeItem::MISCUSECOL:
         return QVariant(tr("Use"));
   }

   qWarning() << QString("BtTreeModel::getMiscHeader Bad column: %1").arg(section);
   return QVariant();
}

QVariant BtTreeModel::yeastHeader(int section) const {
   switch (section) {
      case BtTreeItem::YEASTNAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::YEASTTYPECOL:
         return QVariant(tr("Type"));
      case BtTreeItem::YEASTFORMCOL:
         return QVariant(tr("Form"));
   }

   qWarning() << QString("BtTreeModel::getYeastHeader Bad column: %1").arg(section);
   return QVariant();
}

QVariant BtTreeModel::styleHeader(int section) const {
   switch (section) {
      case BtTreeItem::STYLENAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::STYLECATEGORYCOL:
         return QVariant(tr("Category"));
      case BtTreeItem::STYLENUMBERCOL:
         return QVariant(tr("Number"));
      case BtTreeItem::STYLELETTERCOL:
         return QVariant(tr("Letter"));
      case BtTreeItem::STYLEGUIDECOL:
         return QVariant(tr("Guide"));
   }

   qWarning() << QString("BtTreeModel::getYeastHeader Bad column: %1").arg(section);
   return QVariant();
}

QVariant BtTreeModel::folderHeader(int section) const {
   switch (section) {
      case BtTreeItem::FOLDERNAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::FOLDERPATHCOL:
         return QVariant(tr("PATH"));
      case BtTreeItem::FOLDERFULLCOL:
         return QVariant(tr("FULLPATH"));
   }

   qWarning() << QString("BtTreeModel::getFolderHeader Bad column: %1").arg(section);
   return QVariant();
}

QVariant BtTreeModel::waterHeader(int section) const {
   switch (section) {
      case BtTreeItem::WATERNAMECOL:
         return QVariant(tr("Name"));
      case BtTreeItem::WATERCACOL:
         return QVariant(tr("Ca"));
      case BtTreeItem::WATERHCO3COL:
         return QVariant(tr("HCO3"));
      case BtTreeItem::WATERSO4COL:
         return QVariant(tr("SO4"));
      case BtTreeItem::WATERCLCOL:
         return QVariant(tr("Cl"));
      case BtTreeItem::WATERNACOL:
         return QVariant(tr("Na"));
      case BtTreeItem::WATERMGCOL:
         return QVariant(tr("Mg"));
      case BtTreeItem::WATERpHCOL:
         return QVariant(tr("pH"));
   }
   qWarning() << QString("BtTreeModel::waterHeader Bad column: %1").arg(section);
   return QVariant();

}

bool BtTreeModel::insertRow(int row, const QModelIndex & parent, QObject * victim, std::optional<BtTreeItem::Type> victimType) {
   if (! parent.isValid()) {
      return false;
   }

   BtTreeItem * pItem = item(parent);
   auto type = pItem->type();

   bool success = true;

   beginInsertRows(parent, row, row);
   success = pItem->insertChildren(row, 1, type);
   if (victim && success) {
      type = (victimType ? *victimType : type);
      BtTreeItem * added = pItem->child(row);
      added->setData(type, victim);
   }
   endInsertRows();

   return success;
}

bool BtTreeModel::removeRows(int row, int count, const QModelIndex & parent) {
   BtTreeItem * pItem = item(parent);

   this->beginRemoveRows(parent, row, row + count - 1);
   bool success = pItem->removeChildren(row, count);
   this->endRemoveRows();

   return success;
}

// =========================================================================
// ====================== BREWTARGET STUFF =================================
// =========================================================================

// One find method for all things. This .. is nice
QModelIndex BtTreeModel::findElement(NamedEntity * thing, BtTreeItem * parent) {
   BtTreeItem * pItem;
   QList<BtTreeItem *> folders;

   int i;

   if (parent == nullptr) {
      pItem = rootItem->child(0);
   } else {
      pItem = parent;
   }

   if (! thing) {
      return createIndex(0, 0, pItem);
   }

   folders.append(pItem);

   // Recursion. Wonderful.
   while (! folders.isEmpty()) {
      BtTreeItem * target = folders.takeFirst();
      for (i = 0; i < target->childCount(); ++i) {
         // If we've found what we are looking for, return
         if (target->child(i)->thing() == thing) {
            return createIndex(i, 0, target->child(i));
         }

         // If we have a folder, or we are looking for a brewnote and have a
         // recipe in hand, push the child onto the stack
         if (target->child(i)->type() == BtTreeItem::Type::FOLDER ||
             (qobject_cast<BrewNote *>(thing) && target->child(i)->type() == BtTreeItem::Type::RECIPE)) {
            folders.append(target->child(i));
         }
      }
   }
   return QModelIndex();
}

QList<NamedEntity *> BtTreeModel::elements() {
   QList<NamedEntity *> elements;
   //
   // .:TBD:. This switch would not work if more than one flag were set in the mask, but I don't know if that ever
   // happens.  OTOH, if not, why bother having a set of flags and not just an enum?
   //
   switch (this->treeMask) {
      case RECIPEMASK:
         for (NamedEntity * elem : ObjectStoreWrapper::getAllDisplayableRaw<Recipe>()) {
            elements.append(elem);
         }
         break;
      case EQUIPMASK:
         for (NamedEntity * elem : ObjectStoreWrapper::getAllDisplayableRaw<Equipment>()) {
            elements.append(elem);
         }
         break;
      case FERMENTMASK:
         for (NamedEntity * elem : ObjectStoreWrapper::getAllDisplayableRaw<Fermentable>()) {
            elements.append(elem);
         }
         break;
      case HOPMASK:
         for (NamedEntity * elem : ObjectStoreWrapper::getAllDisplayableRaw<Hop>()) {
            elements.append(elem);
         }
         break;
      case MISCMASK:
         for (NamedEntity * elem : ObjectStoreWrapper::getAllDisplayableRaw<Misc>()) {
            elements.append(elem);
         }
         break;
      case YEASTMASK:
         for (NamedEntity * elem : ObjectStoreWrapper::getAllDisplayableRaw<Yeast>()) {
            elements.append(elem);
         }
         break;
      case STYLEMASK:
         for (NamedEntity * elem : ObjectStoreWrapper::getAllDisplayableRaw<Style>()) {
            elements.append(elem);
         }
         break;
      case WATERMASK:
         for (NamedEntity * elem : ObjectStoreWrapper::getAllDisplayableRaw<Water>()) {
            elements.append(elem);
         }
         break;
      default:
         qWarning() << QString("Invalid treemask: %1").arg(treeMask);
   }
   return elements;
}

void BtTreeModel::loadTreeModel() {
   int i;

   QModelIndex ndxLocal;
   BtTreeItem * local = nullptr;
   QList<NamedEntity *> elems = this->elements();

   qDebug() << Q_FUNC_INFO << "Got " << elems.length() << "elements matching type mask" << this->treeMask;

   for (NamedEntity * elem : elems) {

      if (! elem->folder().isEmpty()) {
         ndxLocal = findFolder(elem->folder(), rootItem->child(0), true);
         // I cannot imagine this failing, but what the hell
         if (! ndxLocal.isValid()) {
            qWarning() << "Invalid return from findFolder in loadTreeModel()";
            continue;
         }
         local = item(ndxLocal);
         i = local->childCount();
      } else {
         local = rootItem->child(0);
         i = local->childCount();
         ndxLocal = createIndex(i, 0, local);
      }

      if (!this->insertRow(i, ndxLocal, elem, this->itemType)) {
         qWarning() << "Insert failed in loadTreeModel()";
         continue;
      }

      // If we have brewnotes, set them up here.
      if (treeMask & RECIPEMASK) {
         Recipe * holdmebeer = qobject_cast<Recipe *>(elem);
         if (PersistentSettings::value(PersistentSettings::Names::showsnapshots, false).toBool() && holdmebeer->hasAncestors()) {
            setShowChild(ndxLocal, true);
            addAncestoralTree(holdmebeer, i, local);
            addBrewNoteSubTree(holdmebeer, i, local, false);
         } else {
            addBrewNoteSubTree(holdmebeer, i, local);
         }
      }
      observeElement(elem);
   }
}

void BtTreeModel::addAncestoralTree(Recipe * rec, int i, BtTreeItem * parent) {
   BtTreeItem * temp = parent->child(i);
   int j = 0;

   for (Recipe * stor : rec->ancestors()) {
      // insert the ancestor. This is most of magic. One day, I understood it.
      // Now I simply copy/paste it
      if (! insertRow(j, createIndex(i, 0, temp), stor, BtTreeItem::Type::RECIPE)) {
         qWarning() << "Ancestor insert failed in loadTreeModel()";
         continue;
      }
      // we need to find the index of what we just inserted
      QModelIndex cIndex = findElement(stor, temp);
      // and set showChild on it
      setShowChild(cIndex, true);

      // finally, add this ancestors brewnotes but do not recurse
      addBrewNoteSubTree(stor, j, temp, false);
      observeElement(stor);
      ++j;
   }
}

void BtTreeModel::addBrewNoteSubTree(Recipe * rec, int i, BtTreeItem * parent, bool recurse) {
   QList<BrewNote *> notes = recurse ? RecipeHelper::brewNotesForRecipeAndAncestors(*rec) : rec->brewNotes();
   BtTreeItem * temp = parent->child(i);

   int j = 0;

   for (BrewNote * note : notes) {
      // In previous insert loops, we ignore the error and soldier on. So we
      // will do that here too
      if (! insertRow(j, createIndex(i, 0, temp), note, BtTreeItem::Type::BREWNOTE)) {
         qWarning() << "Brewnote insert failed in loadTreeModel()";
         continue;
      }
      observeElement(note);
      ++j;
   }
}

template<class T>
T * BtTreeModel::getItem(QModelIndex const & index) const {
   return index.isValid() ? this->item(index)->getData<T>() : nullptr;
}
//
// Instantiate the above template function for the types that are going to use it
//
template Recipe      * BtTreeModel::getItem<Recipe     >(QModelIndex const & index) const;
template Equipment   * BtTreeModel::getItem<Equipment  >(QModelIndex const & index) const;
template Fermentable * BtTreeModel::getItem<Fermentable>(QModelIndex const & index) const;
template Hop         * BtTreeModel::getItem<Hop        >(QModelIndex const & index) const;
template Misc        * BtTreeModel::getItem<Misc       >(QModelIndex const & index) const;
template Yeast       * BtTreeModel::getItem<Yeast      >(QModelIndex const & index) const;
template BrewNote    * BtTreeModel::getItem<BrewNote   >(QModelIndex const & index) const;
template Style       * BtTreeModel::getItem<Style      >(QModelIndex const & index) const;
template BtFolder    * BtTreeModel::getItem<BtFolder   >(QModelIndex const & index) const;
template Water       * BtTreeModel::getItem<Water      >(QModelIndex const & index) const;

NamedEntity * BtTreeModel::thing(const QModelIndex & index) const {
   return index.isValid() ? item(index)->thing() : nullptr;
}

template<class T>
bool BtTreeModel::itemIs(QModelIndex const & index) const {
   return this->type(index) == BtTreeItem::typeOf<T>();
}
//
// Instantiate the above template function for the types that are going to use it
//
template bool BtTreeModel::itemIs<Recipe     >(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<Equipment  >(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<Fermentable>(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<Hop        >(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<Misc       >(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<Yeast      >(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<BrewNote   >(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<Style      >(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<BtFolder   >(QModelIndex const & index) const;
template bool BtTreeModel::itemIs<Water      >(QModelIndex const & index) const;

std::optional<BtTreeItem::Type> BtTreeModel::type(const QModelIndex & index) const {
   return index.isValid() ? std::optional<BtTreeItem::Type>{item(index)->type()} : std::nullopt;
}

QString BtTreeModel::name(const QModelIndex & idx) {
   return idx.isValid() ? item(idx)->name() : QString();
}

int BtTreeModel::mask() {
   return treeMask;
}

void BtTreeModel::copySelected(QList< QPair<QModelIndex, QString>> toBeCopied) {
   bool failed = false;
   while (! toBeCopied.isEmpty()) {
      QPair<QModelIndex, QString> thisPair = toBeCopied.takeFirst();
      QModelIndex ndx = thisPair.first;
      QString name = thisPair.second;

      //
      // Note that the type of the local variable copy is different in each case statement (std::shared_ptr<Equipment>,
      // std::shared_ptr<Fermentable>, etc) which is why the setName and insert calls cannot be pulled out of the
      // switch.
      //
      auto theType = type(ndx);
      if (!theType) {
         qWarning() << Q_FUNC_INFO << "Unknown type for ndx" << ndx;
      } else {
         switch (*theType) {
            case BtTreeItem::Type::EQUIPMENT: {
                  auto copy = ObjectStoreWrapper::copy(*this->getItem<Equipment>(ndx)); // Create a deep copy.
                  copy->setName(name);
                  ObjectStoreWrapper::insert(copy);
                  this->folderChanged(copy.get());
               }
               break;
            case BtTreeItem::Type::FERMENTABLE: {
                  auto copy = ObjectStoreWrapper::copy(*this->getItem<Fermentable>(ndx)); // Create a deep copy.
                  copy->setName(name);
                  ObjectStoreWrapper::insert(copy);
                  this->folderChanged(copy.get());
               }
               break;
            case BtTreeItem::Type::HOP: {
                  auto copy = ObjectStoreWrapper::copy(*this->getItem<Hop>(ndx)); // Create a deep copy.
                  copy->setName(name);
                  ObjectStoreWrapper::insert(copy);
                  this->folderChanged(copy.get());
               }
               break;
            case BtTreeItem::Type::MISC: {
                  auto copy = ObjectStoreWrapper::copy(*this->getItem<Misc>(ndx)); // Create a deep copy.
                  copy->setName(name);
                  ObjectStoreWrapper::insert(copy);
                  this->folderChanged(copy.get());
               }
               break;
            case BtTreeItem::Type::RECIPE: {
                  auto copy = ObjectStoreWrapper::copy(*this->getItem<Recipe>(ndx)); // Create a deep copy.
                  qDebug() <<
                     Q_FUNC_INFO << "display:" <<  copy->display() << "isLocked:" << copy->locked() <<
                     "hasDescendants:" << copy->hasDescendants();
                  copy->setName(name);
                  ObjectStoreWrapper::insert(copy);
                  this->folderChanged(copy.get());
               }
               break;
            case BtTreeItem::Type::STYLE: {
                  auto copy = ObjectStoreWrapper::copy(*this->getItem<Style>(ndx)); // Create a deep copy.
                  copy->setName(name);
                  ObjectStoreWrapper::insert(copy);
                  this->folderChanged(copy.get());
               }
               break;
            case BtTreeItem::Type::YEAST: {
                  auto copy = ObjectStoreWrapper::copy(*this->getItem<Yeast>(ndx)); // Create a deep copy.
                  copy->setName(name);
                  ObjectStoreWrapper::insert(copy);
                  this->folderChanged(copy.get());
               }
               break;
            case BtTreeItem::Type::WATER: {
                  auto copy = ObjectStoreWrapper::copy(*this->getItem<Water>(ndx)); // Create a deep copy.
                  copy->setName(name);
                  ObjectStoreWrapper::insert(copy);
                  this->folderChanged(copy.get());
               }
               break;
            case BtTreeItem::Type::BREWNOTE:
            case BtTreeItem::Type::FOLDER:
               // These cases shouldn't arise (I think!) but the compiler will emit a warning if we don't explicitly
               // have code to handle them (which is good!).
               qWarning() << Q_FUNC_INFO << "Unexpected item type" << static_cast<int>(*theType);
               break;
         }
      }
      if (failed) {
         QMessageBox::warning(nullptr,
                              tr("Could not copy"),
                              tr("There was an unexpected error creating %1").arg(name));
         return;
      }
   }
}

void BtTreeModel::deleteSelected(QModelIndexList victims) {
   QModelIndexList toBeDeleted = victims; // trust me

   // There are black zones of shadow close to our daily paths,
   // and now and then some evil soul breaks a passage through.

   while (! toBeDeleted.isEmpty()) {
      QModelIndex ndx = toBeDeleted.takeFirst();
      auto theType = type(ndx);
      if (!theType) {
         qWarning() << Q_FUNC_INFO << "Unknown type for ndx" << ndx;
         continue;
      }
      switch (*theType) {
         case BtTreeItem::Type::EQUIPMENT:
            ObjectStoreWrapper::softDelete(*this->getItem<Equipment>(ndx));
            break;
         case BtTreeItem::Type::FERMENTABLE:
            ObjectStoreWrapper::softDelete(*this->getItem<Fermentable>(ndx));
            break;
         case BtTreeItem::Type::HOP:
            ObjectStoreWrapper::softDelete(*this->getItem<Hop>(ndx));
            break;
         case BtTreeItem::Type::MISC:
            ObjectStoreWrapper::softDelete(*this->getItem<Misc>(ndx));
            break;
         case BtTreeItem::Type::RECIPE:
            {
               auto rec = this->getItem<Recipe>(ndx);
               int deletewhat = PersistentSettings::value(PersistentSettings::Names::deletewhat, Recipe::DESCENDANT).toInt();
               if (deletewhat == Recipe::DESCENDANT) {
                  this->revertRecipeToPreviousVersion(ndx);
               } else {
                  for (auto ancestor : rec->ancestors()) {
                     ObjectStoreWrapper::softDelete(*ancestor);
                  }
               }
               ObjectStoreWrapper::softDelete(*rec);
            }
            break;
         case BtTreeItem::Type::STYLE:
            ObjectStoreWrapper::softDelete(*this->getItem<Style>(ndx));
            break;
         case BtTreeItem::Type::YEAST:
            ObjectStoreWrapper::softDelete(*this->getItem<Yeast>(ndx));
            break;
         case BtTreeItem::Type::BREWNOTE:
            ObjectStoreWrapper::softDelete(*this->getItem<BrewNote>(ndx));
            break;
         case BtTreeItem::Type::WATER:
            ObjectStoreWrapper::softDelete(*this->getItem<Water>(ndx));
            break;
         case BtTreeItem::Type::FOLDER:
            // We want to delete the contents of the folder (and remove it from from the model) before remove the folder
            // itself, otherwise the QModelIndex values for the contents will not be valid.
            this->deleteSelected(allChildren(ndx));
            this->removeFolder(ndx);
            break;
      }
   }
   return;
}

// =========================================================================
// ============================ FOLDER STUFF ===============================
// =========================================================================

// The actual magic shouldn't be hard. Once we trap the signal, find the
// recipe, remove it from the parent and add it to the target folder.
// It is not easy. Indexes are ephemeral things. We MUST calculate the insert
// index after we have removed the recipe. BAD THINGS happen otherwise.
//
void BtTreeModel::folderChanged(QString name) {
   qDebug() << Q_FUNC_INFO << name;

   NamedEntity * test = qobject_cast<NamedEntity *>(sender());
   if (test) {
      this->folderChanged(test);
   }

   return;
}

void BtTreeModel::folderChanged(NamedEntity * test) {
   qDebug() << Q_FUNC_INFO << test;

   // Find it.
   QModelIndex ndx = findElement(test);
   if (! ndx.isValid()) {
      qWarning() << Q_FUNC_INFO << "Could not find element";
      return;
   }

   QModelIndex pIndex;
   bool expand = true;

   pIndex = parent(ndx); // Get the parent
   // If the parent isn't valid, its the root
   if (!pIndex.isValid()) {
      pIndex = createIndex(0, 0, rootItem->child(0));
   }

   int ii = item(ndx)->childNumber();
   // Remove it
   if (!this->removeRows(ii, 1, pIndex)) {
      qWarning() << Q_FUNC_INFO << "Could not remove row" << ii;
      return;
   }

   // Find the new parent
   // That's awkward, but dropping a folder prolly does need a the folder
   // created.
   QModelIndex newNdx = findFolder(test->folder(), rootItem->child(0), true);
   if (! newNdx.isValid()) {
      newNdx = createIndex(0, 0, rootItem->child(0));
      expand = false;
   }

   BtTreeItem * local = item(newNdx);
   int jj = local->childCount();

   if (!insertRow(jj, newNdx, test, this->itemType)) {
      qWarning() << Q_FUNC_INFO << "Could not insert row" << jj;
      return;
   }
   // If we have brewnotes, set them up here.
   if (treeMask & RECIPEMASK) {
      addBrewNoteSubTree(qobject_cast<Recipe *>(test), jj, local);
   }

   if (expand) {
      emit expandFolder(treeMask, newNdx);
   }
   return;
}

bool BtTreeModel::addFolder(QString name) {
   return findFolder(name, rootItem->child(0), true).isValid();
}

bool BtTreeModel::removeFolder(QModelIndex ndx) {
   if (! ndx.isValid()) {
      return false;
   }

   int i = -1;
   QModelIndex pInd = parent(ndx);

   if (! pInd.isValid()) {
      return false;
   }

   BtTreeItem * start = item(ndx);

   // Remove the victim.
   i = start->childNumber();
   return removeRows(i, 1, pInd);
}

QModelIndexList BtTreeModel::allChildren(QModelIndex ndx) {
   QModelIndexList leafNodes;

   // Don't send an invalid index or something that isn't a folder
   if (! ndx.isValid() || type(ndx) != BtTreeItem::Type::FOLDER) {
      return leafNodes;
   }

   BtTreeItem * start = item(ndx);
   QList<BtTreeItem *> folders;
   folders.append(start);

   while (! folders.isEmpty()) {
      BtTreeItem * target = folders.takeFirst();

      for (int i = 0; i < target->childCount(); ++i) {
         BtTreeItem * next = target->child(i);
         // If a folder, push it onto the folders stack for later processing
         if (next->type() == BtTreeItem::Type::FOLDER) {
            folders.append(next);
         } else { // Leafnode
            leafNodes.append(createIndex(i, 0, next));
         }
      }
   }
   return leafNodes;
}

bool BtTreeModel::renameFolder(BtFolder * victim, QString newName) {
   QModelIndex ndx = findFolder(victim->fullPath(), nullptr, false);
   QModelIndex pInd;
   QString targetPath = newName % "/" % victim->name();
   QPair<QString, BtTreeItem *> f;
   QList<QPair<QString, BtTreeItem *>> folders;
   // This space is important       ^
   int i, kids, src;

   if (! ndx.isValid()) {
      return false;
   }

   pInd = parent(ndx);
   if (! pInd.isValid()) {
      return false;
   }

   BtTreeItem * start = item(ndx);
   f.first  = targetPath;
   f.second = start;

   folders.append(f);

   while (! folders.isEmpty()) {
      // This looks weird, but it is needed for later
      f = folders.takeFirst();
      targetPath = f.first;
      BtTreeItem * target = f.second;

      // As we move things, childCount changes. This makes sure we loop
      // through all of the kids
      kids = target->childCount();
      src = 0;
      // Ok. We have a start and an index.
      for (i = 0; i < kids; ++i) {
         // This looks weird and it is. As we move children out, the 0 items
         // changes to the next child. In the case of a folder, though, we
         // don't move it, so we need to get the item beyond that.
         BtTreeItem * next = target->child(src);
         // If a folder, push it onto the folders stack for latter processing
         if (next->type() == BtTreeItem::Type::FOLDER) {
            QPair<QString, BtTreeItem *> newTarget;
            newTarget.first = targetPath % "/" % next->name();
            newTarget.second = next;
            folders.append(newTarget);
            src++;
         } else { // Leafnode
            next->thing()->setFolder(targetPath);
         }
      }
   }
   // Last thing is to remove the victim.
   i = start->childNumber();
   return removeRows(i, 1, pInd);
}

QModelIndex BtTreeModel::createFolderTree(QStringList dirs, BtTreeItem * parent, QString pPath) {
   BtTreeItem * pItem = parent;

   // Start the loop. We are going to return ndx at the end,
   // so we need to declare and initialize outside of the loop
   QModelIndex ndx = createIndex(pItem->childCount(), 0, pItem);

   // Need to call this because we are adding different things with different
   // column counts. Just using the rowsAboutToBeAdded throws ugly errors and
   // then a sigsegv
   emit layoutAboutToBeChanged();
   for (QString cur : dirs) {
      QString fPath;
      BtFolder * temp = new BtFolder();

      // If the parent item is a folder, use its full path
      if (pItem->type() == BtTreeItem::Type::FOLDER) {
         fPath = pItem->getData<BtFolder>()->fullPath() % "/" % cur;
      } else {
         fPath = pPath % "/" % cur;   // If it isn't we need the parent path
      }

      fPath.replace(QRegExp("//"), "/");

      // Set the full path, which will set the name and the path
      temp->setfullPath(fPath);
      int i = pItem->childCount();

      pItem->insertChildren(i, 1, BtTreeItem::Type::FOLDER);
      pItem->child(i)->setData(BtTreeItem::Type::FOLDER, temp);

      // Set the parent item to point to the newly created tree
      pItem = pItem->child(i);

      // And this for the return
      ndx = createIndex(pItem->childCount(), 0, pItem);
   }
   emit layoutChanged();

   // May K&R have mercy on my soul
   return ndx;
}

QModelIndex BtTreeModel::findFolder(QString name, BtTreeItem * parent, bool create) {
   BtTreeItem * pItem;
   QStringList dirs;
   QString current, fullPath, targetPath;
   int i;

   pItem = parent ? parent : rootItem->child(0);

   // Upstream interfaces should handle this for me, but I like belt and
   // suspenders
   name = name.simplified();
   // I am assuming asking me to find an empty name means find the root of the
   // tree.
   if (name.isEmpty()) {
      return createIndex(0, 0, pItem);
   }

   // Prepare all the variables for the first loop

#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
   dirs = name.split("/", QString::SkipEmptyParts);
#else
   dirs = name.split("/", Qt::SkipEmptyParts);
#endif

   if (dirs.isEmpty()) {
      return QModelIndex();
   }

   current = dirs.takeFirst();
   fullPath = "/";
   targetPath = fullPath % current;

   i = 0;

   // Time to get funky with no recursion!
   while (i < pItem->childCount()) {
      BtTreeItem * kid = pItem->child(i);
      // The kid is a folder
      if (kid->type() == BtTreeItem::Type::FOLDER) {
         // The folder name matches the part we are looking at
         if (kid->getData<BtFolder>()->isFolder(targetPath)) {
            // If there are no more subtrees to look for, we found it
            if (dirs.isEmpty()) {
               return createIndex(i, 0, kid);
            }
            // Otherwise, we found a parent folder in our path
            else {
               // get the next folder in the path
               current = dirs.takeFirst();
               // append that to the fullPath we are looking for
               fullPath = targetPath;
               targetPath = fullPath % "/" % current;

               // Set the parent to the folder
               pItem = kid;
               // Reset the counter
               i = 0;
               // And do the time warp again!
               continue;
            }
         }
      }
      // If we got this far, it wasn't a folder or it wasn't a match.
      i++;
   }
   // If we get here, we found no match.

   // If we are supposed to create something, then lets get busy
   if (create) {
      // push the current dir back on the stack
      dirs.prepend(current);
      // And start with the madness
      return createFolderTree(dirs, pItem, fullPath);
   }

   // If we weren't supposed to create, we drop to here and return an empty
   // index.
   return QModelIndex();
}

// =========================================================================
// ============================ SLOT STUFF ===============================
// =========================================================================

void BtTreeModel::elementChanged() {
   NamedEntity * d = qobject_cast<NamedEntity *>(sender());
   if (!d) {
      return;
   }

   QModelIndex ndxLeft = findElement(d);
   if (! ndxLeft.isValid()) {
      return;
   }

   QModelIndex ndxRight = createIndex(ndxLeft.row(), columnCount(ndxLeft) - 1, ndxLeft.internalPointer());
   emit dataChanged(ndxLeft, ndxRight);
}

/* I don't like this part, but Qt's signal/slot mechanism are pretty
 * simplistic and do a string compare on signatures. Each one of these one
 * liners is required to give the right signature and to be able to call
 * addElement() properly
 */
void BtTreeModel::elementAddedRecipe(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<Recipe     >(victimId)));
}
void BtTreeModel::elementAddedEquipment(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<Equipment  >(victimId)));
}
void BtTreeModel::elementAddedFermentable(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<Fermentable>(victimId)));
}
void BtTreeModel::elementAddedHop(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<Hop        >(victimId)));
}
void BtTreeModel::elementAddedMisc(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<Misc       >(victimId)));
}
void BtTreeModel::elementAddedStyle(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<Style      >(victimId)));
}
void BtTreeModel::elementAddedYeast(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<Yeast      >(victimId)));
}
void BtTreeModel::elementAddedBrewNote(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<BrewNote   >(victimId)));
}
void BtTreeModel::elementAddedWater(int victimId) {
   this->elementAdded(qobject_cast<NamedEntity *>(ObjectStoreWrapper::getByIdRaw<Water      >(victimId)));
}

// I guess this isn't too bad. Better than this same function copied 7 times
void BtTreeModel::elementAdded(NamedEntity * victim) {

   if (!victim->display()) {
      return;
   }

   QModelIndex pIdx;
   auto lType = this->itemType;
   if (qobject_cast<BrewNote *>(victim)) {
      auto brewNote = qobject_cast<BrewNote *>(victim);
      Recipe * recipe = ObjectStoreWrapper::getByIdRaw<Recipe>(brewNote->getRecipeId());
      pIdx = findElement(recipe);
      lType = BtTreeItem::Type::BREWNOTE;
   } else {
      pIdx = createIndex(0, 0, rootItem->child(0));
   }

   if (!pIdx.isValid()) {
      return;
   }

   int breadth = rowCount(pIdx);
   if (!insertRow(breadth, pIdx, victim, lType)) {
      return;
   }

   // We need some special processing here to add brewnotes on a recipe import
   if (qobject_cast<Recipe *>(victim)) {
      Recipe * parent = qobject_cast<Recipe *>(victim);
      QList<BrewNote *> notes = parent->brewNotes();

      if (notes.size()) {
         pIdx = findElement(parent);
         lType = BtTreeItem::Type::BREWNOTE;
         int row = 0;
         for (BrewNote * note : notes) {
            insertRow(row++, pIdx, note, lType);
         }
      }
   }
   observeElement(victim);
   return;
}

void BtTreeModel::elementRemovedRecipe(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}
void BtTreeModel::elementRemovedEquipment(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}
void BtTreeModel::elementRemovedFermentable(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}
void BtTreeModel::elementRemovedHop(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}
void BtTreeModel::elementRemovedMisc(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}
void BtTreeModel::elementRemovedStyle(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}
void BtTreeModel::elementRemovedYeast(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}
void BtTreeModel::elementRemovedBrewNote(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}
void BtTreeModel::elementRemovedWater(int victimId, std::shared_ptr<QObject> victim) {
   this->elementRemoved(qobject_cast<NamedEntity *>(victim.get()));
}

void BtTreeModel::elementRemoved(NamedEntity * victim) {

   if (!victim) {
      return;
   }

   QModelIndex index = findElement(victim);
   if (!index.isValid()) {
      return;
   }

   QModelIndex pIndex = parent(index);
   if (!pIndex.isValid()) {
      return;
   }

   if (!this->removeRows(index.row(), 1, pIndex)) {
      return;
   }

   disconnect(victim, nullptr, this, nullptr);
   return;
}

void BtTreeModel::recipePropertyChanged(int recipeId, BtStringConst const & propertyName) {
   // If a Recipe's ancestor ID has changed then it might be because a new ancestor has been created
   // .:TBD:. We could probably get away with propertyName == PropertyNames::Recipe::ancestorId here because
   // we always use the same constants for property names.
   if (propertyName != PropertyNames::Recipe::ancestorId) {
      qDebug() << Q_FUNC_INFO << "Ignoring change to" << propertyName << "on Recipe" << recipeId;
      return;
   }

   Recipe * descendant = ObjectStoreWrapper::getByIdRaw<Recipe>(recipeId);
   if (!descendant) {
      qCritical() << Q_FUNC_INFO << "Unable to find Recipe" << recipeId << "to check its change to" << propertyName;
      return;
   }

   int ancestorId = descendant->getAncestorId();

   if (ancestorId <= 0 || ancestorId == descendant->key()) {
      qDebug() << Q_FUNC_INFO << "No ancestor (" << ancestorId << ") on Recipe" << recipeId;
      return;
   }

   Recipe * ancestor = ObjectStoreWrapper::getByIdRaw<Recipe>(ancestorId);
   if (!ancestor) {
      qCritical() << Q_FUNC_INFO << "Unable to find Recipe" << ancestorId << "set as ancestor to Recipe" << recipeId;
      return;
   }

   this->versionedRecipe(ancestor, descendant);

   return;
}


void BtTreeModel::observeElement(NamedEntity * d) {
   if (! d) {
      return;
   }

   if (qobject_cast<BrewNote *>(d)) {
      connect(d, SIGNAL(brewDateChanged(const QDate &)), this, SLOT(elementChanged()));
   } else {
      connect(d, SIGNAL(changedName(QString)), this, SLOT(elementChanged()));
      // connect(d, SIGNAL(changedFolder(QString)), this, SLOT(folderChanged(QString)));
   }
}


// =========================================================================
// ===================== DRAG AND DROP STUFF ===============================
// =========================================================================
bool BtTreeModel::dropMimeData(QMimeData const * data,
                               Qt::DropAction action,
                               int row,
                               int column,
                               QModelIndex const & parent) {
   qInfo() << Q_FUNC_INFO;

   QByteArray encodedData;

   if (data->hasFormat(_mimeType)) {
      encodedData = data->data(_mimeType);
   } else if (data->hasFormat("application/x-brewtarget-folder")) {
      encodedData = data->data("application/x-brewtarget-folder");
   } else {
      return false;   // Don't know what we got, but we don't want it
   }

   if (! parent.isValid()) {
      return false;
   }

   QString target = "";
   if (this->itemIs<BtFolder>(parent)) {
      target = this->getItem<BtFolder>(parent)->fullPath();
   } else {
      NamedEntity * something = this->thing(parent);

      // Did you know there's a space between elements in a tree, and you can
      // actually drop things there? If somebody drops something there, don't
      // do anything
      if (! something) {
         return false;
      }

      target = something->folder();
   }

   // Pull the stream apart and do that which needs done. Late binding ftw!
   for (QDataStream stream{&encodedData, QIODevice::ReadOnly}; !stream.atEnd(); ) {
      int oTypeRaw;
      int id;
      QString name = "";
      QString text;
      stream >> oTypeRaw >> id >> name;
      BtTreeItem::Type oType = static_cast<BtTreeItem::Type>(oTypeRaw);
      NamedEntity * elem = getElement(oType, id);

      if (elem == nullptr && oType != BtTreeItem::Type::FOLDER) {
         return false;
      }

      // this is the work.
      if (oType != BtTreeItem::Type::FOLDER) {
         qDebug() << Q_FUNC_INFO << "Moving" << elem << "from folder" << elem->folder() << "to folder" << target;
         // Dropping an item in a folder just means setting the folder name on that item
         elem->setFolder(target);
         // Now we have to update our own model (ie that of BtTreeModel) so that the display will update!
         this->folderChanged(elem);

      } else {
         // I need the actual folder object that got dropped.
         BtFolder * victim = new BtFolder;
         victim->setfullPath(name);

         renameFolder(victim, target);
      }
   }

   return true;
}

void BtTreeModel::revertRecipeToPreviousVersion(QModelIndex ndx) {
   BtTreeItem * node = item(ndx);
   BtTreeItem * pNode = node->parent();
   QModelIndex pIndex = parent(ndx);

   // The recipe referred to by the index is the one that's about to be deleted
   auto recipeToRevert = this->getItem<Recipe>(ndx);

   auto ancestor = recipeToRevert->revertToPreviousVersion();

   // don't do anything if there is nothing to do
   if (!ancestor) {
      return;
   }

   // Remove all the rows associated with the about-to-be-deleted Recipe
   removeRows(0, node->childCount(), ndx);

   // Put the ancestor into the tree
   if (! insertRow(pIndex.row(), pIndex, ancestor, BtTreeItem::Type::RECIPE)) {
      qWarning() << Q_FUNC_INFO << "Could not add ancestor to tree";
   }

   // Find the ancestor in the tree
   QModelIndex ancNdx = findElement(ancestor);
   if (! ancNdx.isValid()) {
      qWarning() << Q_FUNC_INFO << "Couldn't find the ancestor";
   }

   // Add the ancestor's brewnotes to the descendant
   addBrewNoteSubTree(ancestor, ancNdx.row(), pNode);

   return;
}

// This is detaching a Recipe from its previous versions
void BtTreeModel::orphanRecipe(QModelIndex ndx) {
   BtTreeItem* node = item(ndx);
   BtTreeItem* pNode = node->parent();
   QModelIndex pIndex = parent(ndx);

   // I need the recipe referred to by the index
   auto orphan = this->getItem<Recipe>(ndx);

   // don't do anything if there is nothing to do
   if ( ! orphan->hasAncestors() ) {
      return;
   }

   // And I need its immediate ancestor
   auto ancestor = orphan->ancestors().at(0);

   // Deal with the soon-to-be orphan first
   // Remove all the rows associated with the orphan
   removeRows(0,node->childCount(),ndx);

   // This looks weird, but I think it will do what I need -- set
   // the ancestor_id to itself and reload the ancestors array. setAncestor
   // handles the locked and display flags
   orphan->setAncestor(*orphan);
   // Display all of its brewnotes
   addBrewNoteSubTree(orphan, ndx.row(), pNode, false);

   // set the ancestor to visible. Not sure this is required?
   ancestor->setDisplay(true);
   ancestor->setLocked(false);

   // Put the ancestor into the tree
   if ( ! insertRow(pIndex.row(), pIndex, ancestor, BtTreeItem::Type::RECIPE) ) {
      qWarning() << Q_FUNC_INFO << "Could not add ancestor to tree";
   }

   // Find the ancestor in the tree
   QModelIndex ancNdx = findElement(ancestor);
   if ( ! ancNdx.isValid() ) {
      qWarning() << Q_FUNC_INFO << "Couldn't find the ancestor";
   }

   // Add the ancestor's brewnotes to the descendant
   addBrewNoteSubTree(ancestor,ancNdx.row(),pNode);

   return;
}

void BtTreeModel::spawnRecipe(QModelIndex ndx) {
   Q_ASSERT(ndx.isValid());
   auto ancestor = this->getItem<Recipe>(ndx);
   Q_ASSERT(ancestor);
   std::shared_ptr<Recipe> descendant = std::make_shared<Recipe>(*ancestor);
   // We want to store the new Recipe first so that it gets an ID...
   ObjectStoreWrapper::insert<Recipe>(descendant);
   // ...then we can connect it to the one it was copied from
   descendant->setAncestor(*ancestor);
   qDebug() <<
      Q_FUNC_INFO << "Created descendant Recipe" << descendant->key() << "of Recipe" << ancestor->key() <<
      "(at position" << ndx.row() << ")";

   //
   // First, we remove the ancestor from the tree
   //
   // NB: Inserting the descendant in the database will have generated a signal resulting in a call to
   // BtTreeModel::elementAddedRecipe(), so we can't assume that ndx is still valid, hence the reassignement here.
   //
   ndx = this->findElement(ancestor);
   if (!removeRows(ndx.row(), 1, this->parent(ndx))) {
      qCritical() << Q_FUNC_INFO << "Could not find Recipe" << ancestor->key() << "in display tree";
   }

   // Now we need to find the descendant in the tree. This has to be done
   // after we removed the ancestor row, otherwise the index will be wrong.
   QModelIndex decNdx = findElement(descendant.get());
   this->showAncestors(decNdx);

   emit dataChanged(decNdx, decNdx);
   emit recipeSpawn(descendant.get());

   return;
}

void BtTreeModel::versionedRecipe(Recipe * ancestor, Recipe * descendant) {
   qDebug() << Q_FUNC_INFO << "Updating tree now that Recipe" << descendant->key() << "has ancestor Recipe" << ancestor->key();

   // like before, remove the ancestor
   QModelIndex ndx = findElement(ancestor);
   if (!removeRows(ndx.row(), 1, this->parent(ndx))) {
      qCritical() << Q_FUNC_INFO << "Could not find Recipe" << ancestor->key() << "in display tree";
   }

   // add the descendant in, but get the index only after we removed the
   // ancestor
   QModelIndex decNdx = findElement(descendant);
   this->showAncestors(decNdx);

   // do not mess with this order. We have to signal the data is in the tree
   // first (dataChanged), then put it in the right folder (folderChanged) and
   // finally tell MainWindow something happened (recipeSpawn).
   // Any other order doesn't work, or dumps core
   emit dataChanged(decNdx, decNdx);
   this->folderChanged(descendant);
   emit recipeSpawn(descendant);
   return;
}


QStringList BtTreeModel::mimeTypes() const {
   QStringList types;
   // accept whatever type we like, and folders
   types << _mimeType << "application/x-brewtarget-folder";

   return types;
}

Qt::DropActions BtTreeModel::supportedDropActions() const {
   return Qt::CopyAction | Qt::MoveAction;
}


// =========================================================================
// ===================== RECIPE VERSION STUFF ==============================
// =========================================================================
//
bool BtTreeModel::showChild(QModelIndex child) const {
   BtTreeItem * node = item(child);
   return node->showMe();
}

void BtTreeModel::setShowChild(QModelIndex child, bool val) {
   BtTreeItem * node = item(child);
   return node->setShowMe(val);
}

void BtTreeModel::showAncestors(QModelIndex ndx) {

   if (! ndx.isValid()) {
      return;
   }

   BtTreeItem * node = item(ndx);
   auto descendant = this->getItem<Recipe>(ndx);
   QList<Recipe *> ancestors = descendant->ancestors();

   removeRows(0, node->childCount(), ndx);

   // add the brewnotes for this version back
   addBrewNoteSubTree(descendant, ndx.row(), node->parent(), false);

   // set showChild on the leaf node. I use this for drawing menus
   setShowChild(ndx, true);

   // Now loop through the ancestors. The nature of the beast is nearest
   // ancestors are first
   for (auto ancestor : ancestors) {
      int j = node->childCount();
      if (! insertRow(j, ndx, ancestor, BtTreeItem::Type::RECIPE)) {
         qWarning() << "Could not add ancestoral brewnotes";
      }
      QModelIndex cIndex = findElement(ancestor, node);
      setShowChild(cIndex, true);
      // ew, but apparently this has to happen here.
      emit dataChanged(cIndex, cIndex);

      // add the brewnotes to the ancestors, but make sure we don't recurse
      addBrewNoteSubTree(ancestor, j, node, false);
   }
   return;
}

void BtTreeModel::hideAncestors(QModelIndex ndx) {
   BtTreeItem * node = item(ndx);

   // This has no potential to be clever. None.
   if (! ndx.isValid()) {
      return;
   }

   // remove all the currently shown children
   removeRows(0, node->childCount(), ndx);
   auto descendant = this->getItem<Recipe>(ndx);

   // put the brewnotes back, including those from the ancestors.
   addBrewNoteSubTree(descendant, ndx.row(), node->parent());

   // This is for menus
   setShowChild(ndx, false);

   // Now we just need to mark each ancestor invisible again
   for (auto ancestor : descendant->ancestors()) {
      QModelIndex aIndex = findElement(ancestor, node);
      setShowChild(aIndex, false);
      emit dataChanged(aIndex, aIndex);
   }
   return;
}

// more cleverness must happen. Wonder if I can figure it out.
void BtTreeModel::catchAncestors(bool showem) {
   QModelIndex ndxLocal;
   BtTreeItem * local = nullptr;
   QList<NamedEntity *> elems = elements();

   foreach (NamedEntity * elem, elems) {
      Recipe * rec = qobject_cast<Recipe *>(elem);

      local = rootItem->child(0);
      ndxLocal = findElement(elem, local);

      if (rec->hasAncestors()) {
         showem ? showAncestors(ndxLocal) : hideAncestors(ndxLocal);
      }
   }
   return;
}
