/****************************************************************************
**
** Copyright (c) 2009-2012, Jaco Naude
**
** This file is part of Qtilities which is released under the following
** licensing options.
**
** Option 1: Open Source
** Under this license Qtilities is free software: you can
** redistribute it and/or modify it under the terms of the GNU General
** Public License as published by the Free Software Foundation, either
** version 3 of the License, or (at your option) any later version.
**
** Qtilities is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Qtilities. If not, see http://www.gnu.org/licenses/.
**
** Option 2: Commercial
** Alternatively, this library is also released under a commercial license
** that allows the development of closed source proprietary applications
** without restrictions on licensing. For more information on this option,
** please see the project website's licensing page:
** http://www.qtilities.org/licensing.html
**
** If you are unsure which license is appropriate for your use, please
** contact support@qtilities.org.
**
****************************************************************************/

#include "ObserverTreeModel.h"
#include "QtilitiesCoreGuiConstants.h"
#include "ObserverMimeData.h"
#include "QtilitiesApplication.h"
#include "ObserverTreeModelBuilder.h"

#include <SubjectTypeFilter.h>
#include <QtilitiesCoreConstants.h>
#include <Observer.h>
#include <ActivityPolicyFilter.h>
#include <Logger.h>
#include <QtilitiesCategory.h>

#include <QMessageBox>
#include <QIcon>
#include <QDropEvent>
#include <QFileIconProvider>

#include <stdio.h>
#include <time.h>

using namespace Qtilities::CoreGui::Constants;
using namespace Qtilities::CoreGui::Icons;
using namespace Qtilities::Core::Properties;
using namespace Qtilities::Core;
using namespace Qtilities::Core::Constants;

#define PROGRESS_BAR_DISPLAY_THRESHOLD 10

struct Qtilities::CoreGui::ObserverTreeModelData  {
    ObserverTreeModelData() : tree_model_up_to_date(true),
        tree_rebuild_queued(false),
        tree_building_threading_enabled(false) {}


    QPointer<ObserverTreeItem>  rootItem;
    QPointer<Observer>          selection_parent;
    QModelIndex                 selection_index;
    QList<QPointer<QObject> >   selected_objects;
    QString                     type_grouping_name;
    bool                        read_only;
    QFileIconProvider           icon_provider;

    QList<QPointer<QObject> >   new_selection;
    QList<QPointer<QObject> >   queued_selection;

    bool                        tree_model_up_to_date;
    QThread                     tree_builder_thread;
    ObserverTreeModelBuilder    tree_builder;
    bool                        tree_rebuild_queued;
    bool                        tree_building_threading_enabled;

    QStringList                 expanded_items;
};

Qtilities::CoreGui::ObserverTreeModel::ObserverTreeModel(QObject* parent) :
    QAbstractItemModel(parent),
    AbstractObserverItemModel()
{
    d = new ObserverTreeModelData;

    // Init root data
    d->rootItem = new ObserverTreeItem();
    d->selection_parent = 0;
    d->type_grouping_name = QString();
    d->read_only = false;

    qRegisterMetaType<Qtilities::CoreGui::ObserverTreeItem>("Qtilities::CoreGui::ObserverTreeItem");
}

Qtilities::CoreGui::ObserverTreeModel::~ObserverTreeModel() {
    if (d->rootItem)
        delete d->rootItem;
    delete d;
}

bool Qtilities::CoreGui::ObserverTreeModel::setObserverContext(Observer* observer) {
    if (d_observer) {
        d_observer->disconnect(this);
        clearTreeStructure();
        reset();
    }

    if (!observer)
        return false;

    if (!AbstractObserverItemModel::setObserverContext(observer))
        return false;

    // Check if this observer has a subject type filter installed
    for (int i = 0; i < d_observer->subjectFilters().count(); i++) {
        SubjectTypeFilter* subject_type_filter = qobject_cast<SubjectTypeFilter*> (d_observer->subjectFilters().at(i));
        if (subject_type_filter) {
            if (!subject_type_filter->groupName().isEmpty()) {
                d->type_grouping_name = subject_type_filter->groupName();
            }
            break;
        }
    }

    // The first time the observer context is set we rebuild the tree:
    if (observer->observerName() != qti_def_GLOBAL_OBJECT_POOL)
        recordObserverChange();

    connect(d_observer,SIGNAL(destroyed()),SLOT(handleObserverContextDeleted()));
    connect(d_observer,SIGNAL(layoutChanged(QList<QPointer<QObject> >)),SLOT(recordObserverChange(QList<QPointer<QObject> >)));
    connect(d_observer,SIGNAL(dataChanged(Observer*)),SLOT(handleContextDataChanged(Observer*)));

    // If a selection parent does not exist, we set observer as the selection parent:
    if (!d->selection_parent)
        d->selection_parent = observer;

    return true;
}

int Qtilities::CoreGui::ObserverTreeModel::columnPosition(AbstractObserverItemModel::ColumnID column_id) const {
    if (column_id == AbstractObserverItemModel::ColumnName) {
        return 0;
    } else if (column_id == AbstractObserverItemModel::ColumnChildCount) {
        return 1;
    } else if (column_id == AbstractObserverItemModel::ColumnAccess) {
        return 2;
    } else if (column_id == AbstractObserverItemModel::ColumnTypeInfo) {
        return 3;
    } else if (column_id == AbstractObserverItemModel::ColumnCategory) {
        return -1;
    } else if (column_id == AbstractObserverItemModel::ColumnSubjectID) {
        return -1;
    } else if (column_id == AbstractObserverItemModel::ColumnLast) {
        return 3;
    }

    return -1;
}

int Qtilities::CoreGui::ObserverTreeModel::columnVisiblePosition(AbstractObserverItemModel::ColumnID column_id) const {
    int start = 0;
    if (column_id == AbstractObserverItemModel::ColumnName) {
        start = 0;
    } else if (column_id == AbstractObserverItemModel::ColumnChildCount) {
        start = 1;
    } else if (column_id == AbstractObserverItemModel::ColumnAccess) {
        start = 2;
    } else if (column_id == AbstractObserverItemModel::ColumnTypeInfo) {
        start = 3;
    } else if (column_id == AbstractObserverItemModel::ColumnCategory) {
        return -1;
    } else if (column_id == AbstractObserverItemModel::ColumnSubjectID) {
        return -1;
    } else if (column_id == AbstractObserverItemModel::ColumnLast) {
        start = 4;
    }

    // Now we need to subtract hidden columns from current:
    int current = start;
    if (activeHints()) {
        if (start == 1) {
            if (!(activeHints()->itemViewColumnHint() & ObserverHints::ColumnNameHint))
                return -1;
        }
        if (start > 1) {
            if (!(activeHints()->itemViewColumnHint() & ObserverHints::ColumnChildCountHint))
                --current;
        }
        if (start > 2) {
            if (!(activeHints()->itemViewColumnHint() & ObserverHints::ColumnAccessHint))
                --current;
        }
        if (start > 3) {
            if (!(activeHints()->itemViewColumnHint() & ObserverHints::ColumnTypeInfoHint))
                --current;
        }
        if (start == 4) {
            --current;
        }
    }

    return current;
}

QStack<int> Qtilities::CoreGui::ObserverTreeModel::getParentHierarchy(const QModelIndex& index) const {
    QStack<int> parent_hierarchy;
    ObserverTreeItem* item = getItem(index);
    if (!item)
        return parent_hierarchy;

    ObserverTreeItem* parent_item = item->parentItem();
    Observer* parent_observer = qobject_cast<Observer*> (parent_item->getObject());
    // Handle the cases where the parent is a category item:
    if (!parent_observer) {
        while (parent_item->itemType() == ObserverTreeItem::CategoryItem)
            parent_item = parent_item->parentItem();
        parent_observer = qobject_cast<Observer*> (parent_item->getObject());
    }

    // Check if the parent observer is contained:
    if (!parent_observer) {
        if (parent_item->getObject()) {
            for (int i = 0; i < parent_item->getObject()->children().count(); i++) {
                QObject* child = parent_item->getObject()->children().at(i);
                if (child) {
                    parent_observer = qobject_cast<Observer*> (child);
                }
            }
        }
    }

    while (parent_observer) {
        parent_hierarchy.push_front(parent_observer->observerID());
        parent_item = parent_item->parentItem();
        if (parent_item) {
            parent_observer = qobject_cast<Observer*> (parent_item->getObject());
            // Handle the cases where the parent is a category item:
            if (!parent_observer) {
                while (parent_item->itemType() == ObserverTreeItem::CategoryItem)
                    parent_item = parent_item->parentItem();
                parent_observer = qobject_cast<Observer*> (parent_item->getObject());
            }
            // Check if the parent observer is contained:
            if (!parent_observer) {
                if (parent_item->getObject()) {
                    for (int i = 0; i < parent_item->getObject()->children().count(); i++) {
                        QObject* child = parent_item->getObject()->children().at(i);
                        if (child) {
                            parent_observer = qobject_cast<Observer*> (child);
                        }
                    }
                }
            }
        } else
            parent_observer = 0;
    }

    return parent_hierarchy;
}

QVariant Qtilities::CoreGui::ObserverTreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (!d_observer)
        return QVariant();

    if (!d->tree_model_up_to_date)
        return QVariant();

    // ------------------------------------
    // Handle Name Column
    // ------------------------------------
    if (index.column() == columnPosition(ColumnName)) {
        // ------------------------------------
        // Qt::DisplayRole and Qt::EditRole
        // ------------------------------------
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return "Invalid Item";
            QObject* obj = item->getObject();
            if (obj) {
                // If this is a category item we just use objectName:
                if (item->itemType() == ObserverTreeItem::CategoryItem)
                    return item->objectName();
                else {
                    // Otherwise we must get the name of the object in the parent observer's context:
                    // First get the parent observer:
                    Observer* obs = 0;
                    if (item->parentItem()) {
                        if (item->parentItem()->itemType() == ObserverTreeItem::CategoryItem) {
                            obs = item->parentItem()->containedObserver();
                        } else {
                            obs = qobject_cast<Observer*> (item->parentItem()->getObject());
                        }
                    }

                    // Check the modification state of the object if it implements IModificationNotifier:
                    bool is_modified = false;
                    if (activeHints()->modificationStateDisplayHint() == ObserverHints::CharacterModificationStateDisplay && role == Qt::DisplayRole) {
                        IModificationNotifier* mod_iface = qobject_cast<IModificationNotifier*> (obj);
                        if (mod_iface)
                            is_modified = mod_iface->isModified();
                    }

                    QString return_string;
                    // If observer is valid, we get the name, otherwise we just use object name.
                    if (obs)
                        return_string = obs->subjectDisplayedNameInContext(obj);
                    else
                        return_string = obj->objectName();

                    if (is_modified)
                        return return_string + "*";
                    else
                        return return_string;
                }
            } else {
                // We might get in here when a view tries to repaint itself (for example when a message box dissapears above it) befire
                // the tree has been rebuilt properly.
                return tr("Invalid object");
            }
        // ------------------------------------
        // Qt::CheckStateRole
        // ------------------------------------
        } else if (role == Qt::CheckStateRole) {
            // In here we need to manually get the top level of each index since the active context is
            // not representitive of all indexes we get in here:
            QObject* obj = getObject(index);
            if (!obj)
                return QVariant();

            QVariant subject_activity = QVariant();
            Observer* local_selection_parent = parentOfIndex(index);

            if (local_selection_parent) {
                // Once we have the local parent, we can check if it must display activity and if so, we return
                // the activity of obj in that context.

                // We need to check a few things things:
                // 1. Do we use the observer hints?
                // 2. If not, does the selection have hints?
                ObserverHints* hints_to_use_for_selection = 0;
                if (model->use_observer_hints && local_selection_parent->displayHints())
                    hints_to_use_for_selection = local_selection_parent->displayHints();
                else if (!model->use_observer_hints)
                    hints_to_use_for_selection = activeHints();

                if (hints_to_use_for_selection) {
                    if (hints_to_use_for_selection->activityDisplayHint() == ObserverHints::CheckboxActivityDisplay) {
                        ActivityPolicyFilter* activity_filter = 0;
                        for (int i = 0; i < local_selection_parent->subjectFilters().count(); i++) {
                            activity_filter = qobject_cast<ActivityPolicyFilter*> (local_selection_parent->subjectFilters().at(i));
                            if (activity_filter) {
                                subject_activity = local_selection_parent->getMultiContextPropertyValue(obj,qti_prop_ACTIVITY_MAP);
                            }
                        }
                    }
                }
            }

            if (subject_activity.isValid()) {
                if (subject_activity.toBool())
                    return Qt::Checked;
                else
                    return Qt::Unchecked;
            } else
                return QVariant();
        // ------------------------------------
        // Qt::DecorationRole
        // ------------------------------------
        } else if (role == Qt::DecorationRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            // Check if it has the OBJECT_ICON shared property set.
            if (obj) {
                // If this is a category item we just use objectName:
                if (item->itemType() == ObserverTreeItem::CategoryItem)
                    return d->icon_provider.icon(QFileIconProvider::Folder);
                else {
                    SharedProperty icon_property = ObjectManager::getSharedProperty(obj,qti_prop_DECORATION);
                    if (icon_property.isValid())
                        return icon_property.value();
                }
            }
        // ------------------------------------
        // Qt::WhatsThisRole
        // ------------------------------------
        } else if (role == Qt::WhatsThisRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            // Check if it has the OBJECT_ICON shared property set.
            if (obj) {
                SharedProperty icon_property = ObjectManager::getSharedProperty(obj,qti_prop_WHATS_THIS);
                if (icon_property.isValid())
                    return icon_property.value();                        
            }
        // ------------------------------------
        // Qt::SizeHintRole
        // ------------------------------------
        } else if (role == Qt::SizeHintRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            // Check if it has the OBJECT_ICON shared property set.
            if (obj) {
                SharedProperty icon_property = ObjectManager::getSharedProperty(obj,qti_prop_SIZE_HINT);
                if (icon_property.isValid())
                    return icon_property.value();
            }
        // ------------------------------------
        // Qt::StatusTipRole
        // ------------------------------------
        } else if (role == Qt::StatusTipRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            // Check if it has the OBJECT_ICON shared property set.
            if (obj) {
                SharedProperty icon_property = ObjectManager::getSharedProperty(obj,qti_prop_STATUSTIP);
                if (icon_property.isValid())
                    return icon_property.value();
            }
        // ------------------------------------
        // Qt::FontRole
        // ------------------------------------
        } else if (role == Qt::FontRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            // Check if it has the OBJECT_ICON shared property set.
            if (obj) {
                SharedProperty icon_property = ObjectManager::getSharedProperty(obj,qti_prop_FONT);
                if (icon_property.isValid())
                    return icon_property.value();
            }
        // ------------------------------------
        // Qt::TextAlignmentRole
        // ------------------------------------
        } else if (role == Qt::TextAlignmentRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            // Check if it has the OBJECT_ICON shared property set.
            if (obj) {
                SharedProperty icon_property = ObjectManager::getSharedProperty(obj,qti_prop_TEXT_ALIGNMENT);
                if (icon_property.isValid())
                    return icon_property.value();
            }
        // ------------------------------------
        // Qt::BackgroundRole
        // ------------------------------------
        } else if (role == Qt::BackgroundRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            // Check if it has the OBJECT_ICON shared property set.
            if (obj) {
                SharedProperty icon_property = ObjectManager::getSharedProperty(obj,qti_prop_BACKGROUND);
                if (icon_property.isValid())
                    return icon_property.value();
            }
        // ------------------------------------
        // Qt::ForegroundRole
        // ------------------------------------
        } else if (role == Qt::ForegroundRole) {
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            // Check if it has the OBJECT_ICON shared property set.
            if (obj) {
                SharedProperty icon_property = ObjectManager::getSharedProperty(obj,qti_prop_FOREGROUND);
                if (icon_property.isValid())
                    return icon_property.value();
            }
        // ------------------------------------
        // Qt::ToolTipRole
        // ------------------------------------
        } else if (role == Qt::ToolTipRole) {
            // Check if the object has an OBJECT_TOOLTIP shared property to show.
            ObserverTreeItem* item = getItem(index);
            if (!item)
                return QVariant();
            QObject* obj = item->getObject();

            if (obj) {
                SharedProperty tooltip = ObjectManager::getSharedProperty(obj,qti_prop_TOOLTIP);
                if (tooltip.isValid())
                    return tooltip.value();
            }
        }
    // ------------------------------------
    // Handle Child Count Column
    // ------------------------------------
    } else if (index.column() == columnPosition(ColumnChildCount)) {
        if (role == Qt::DisplayRole) {
            // Check if it is an observer, in that case we return treeCount() on the observer
            Observer* observer = qobject_cast<Observer*> (getItem(index)->getObject());
            if (observer) {
                return observer->treeCount();
            } else {
                QObject* obj = getItem(index)->getObject();
                // Handle the case where the child is the parent of an observer
                int count = 0;
                if (obj){
                    foreach (QObject* child, obj->children()) {
                        Observer* child_observer = qobject_cast<Observer*> (child);
                        if (child_observer)
                            count += child_observer->treeCount();
                    }
                }

                if (count == 0) {
                    if (getItem(index)->itemType() == ObserverTreeItem::CategoryItem) {
                        Observer* parent_observer = qobject_cast<Observer*> (getItem(index)->containedObserver());
                        if (parent_observer)
                            return parent_observer->subjectReferencesByCategory(getItem(index)->category()).count();
                        else
                            return QVariant();
                    } else
                        return QVariant();
                } else
                    return count;
            }
        }
    // ------------------------------------
    // Handle Subject Type Info Column
    // ------------------------------------
    } else if (index.column() == columnPosition(ColumnTypeInfo)) {
        if (role == Qt::DisplayRole) {
            QObject* obj = getObject(index);
            if (obj) {
                if (obj->metaObject()) {
                    if (QString(QLatin1String(obj->metaObject()->className())).split("::").last() != "QObject")
                        return QString(QLatin1String(obj->metaObject()->className())).split("::").last();
                }
            }
            return QVariant();
        }
    // ------------------------------------
    // Handle Access Column
    // ------------------------------------
    } else if (index.column() == columnPosition(ColumnAccess)) {
        if (role == Qt::DecorationRole) {
            // First handle categories:
            ObserverTreeItem* item = getItem(index);
            if (item) {
                // If this is a category item we check if it has access & scope defined.
                if (item->itemType() == ObserverTreeItem::CategoryItem) {
                    // Get the parent observer:
                    Observer* obs = 0;
                    if (item->parentItem()) {
                        if (item->parentItem()->itemType() == ObserverTreeItem::CategoryItem) {
                            obs = item->parentItem()->containedObserver();
                        } else {
                            obs = qobject_cast<Observer*> (item->parentItem()->getObject());
                        }
                    }
                    // If observer is valid, we get the name, otherwise we just use object name.
                    if (obs) {
                        if (obs->accessModeScope() == Observer::CategorizedScope) {
                            if (obs->accessMode(item->category()) == Observer::ReadOnlyAccess)
                                return QIcon(qti_icon_READ_ONLY_16x16);
                            else if (obs->accessMode(item->category()) == Observer::LockedAccess)
                                return QIcon(qti_icon_LOCKED_16x16);
                            else
                                return QVariant();
                        }
                    }
                } else {
                    QObject* obj = getObject(index);
                    if (!obj)
                        return QVariant();
                    Observer* observer = qobject_cast<Observer*> (obj);
                    if (!observer) {
                        // Handle the case where the child is the parent of an observer
                        foreach (QObject* child, obj->children()) {
                            observer = qobject_cast<Observer*> (child);
                            if (observer)
                                break;
                        }
                    }

                    if (observer) {
                        if (observer->accessModeScope() == Observer::GlobalScope) {
                            if (observer->accessMode() == Observer::FullAccess)
                                return QVariant();
                            if (observer->accessMode() == Observer::ReadOnlyAccess)
                                return QIcon(qti_icon_READ_ONLY_16x16);
                            if (observer->accessMode() == Observer::LockedAccess)
                                return QIcon(qti_icon_LOCKED_16x16);
                        } else {
                            // Inspect the object to see if it has the qti_prop_ACCESS_MODE observer property.
                            QVariant mode = d_observer->getMultiContextPropertyValue(obj,qti_prop_ACCESS_MODE);
                            if (mode.toInt() == (int) Observer::ReadOnlyAccess)
                                return QIcon(qti_icon_READ_ONLY_16x16);
                            if (mode.toInt() == Observer::LockedAccess)
                                return QIcon(qti_icon_LOCKED_16x16);
                        }
                    } else {
                        // Inspect the object to see if it has the qti_prop_ACCESS_MODE observer property.
                        QVariant mode = d_observer->getMultiContextPropertyValue(obj,qti_prop_ACCESS_MODE);
                        if (mode.toInt() == (int) Observer::ReadOnlyAccess)
                            return QIcon(qti_icon_READ_ONLY_16x16);
                        if (mode.toInt() == Observer::LockedAccess)
                            return QIcon(qti_icon_LOCKED_16x16);
                    }
                }
            }
        }
    }

    return QVariant();
}

Qt::ItemFlags Qtilities::CoreGui::ObserverTreeModel::flags(const QModelIndex &index) const {
    if (!d->tree_model_up_to_date)
        return Qt::NoItemFlags;

     if (!index.isValid())
         return Qt::NoItemFlags;

     Qt::ItemFlags item_flags = Qt::ItemIsEnabled;
     ObserverTreeItem* item = getItem(index);
     if (!item)
         return item_flags;

     // Selectable Items hint always uses the top level observer for good reason. Otherwise
     // we end up in situation where one part of tree can be selected and other part not.
     // When we select a part which is not selectable then, we can't select the part which must
     // be selectable.
     if (model->hints_top_level_observer) {
         if (model->hints_top_level_observer->itemSelectionControlHint() == ObserverHints::SelectableItems)
             item_flags |= Qt::ItemIsSelectable;
         else
             item_flags &= ~Qt::ItemIsSelectable;
     } else {
         if (model->hints_default->itemSelectionControlHint() == ObserverHints::SelectableItems)
             item_flags |= Qt::ItemIsSelectable;
         else
             item_flags &= ~Qt::ItemIsSelectable;
     }

     // Handle category items
     if (item->itemType() == ObserverTreeItem::CategoryItem) {
         item_flags &= ~Qt::ItemIsEditable;
         item_flags &= ~Qt::ItemIsUserCheckable;
     } else {
         // The naming control hint we get from the active hints since the user must click
         // in order to edit the name. The click will update activeHints() for us.
         if (activeHints()->namingControlHint() == ObserverHints::EditableNames && !d->read_only)
             item_flags |= Qt::ItemIsEditable;
         else
             item_flags &= ~Qt::ItemIsEditable;

         // For the activity display we need to manually get the top level of each index
         // since the active context is not representitive of all indexes we get in here:
         Observer* local_selection_parent = parentOfIndex(index);
         if (local_selection_parent) {
             if (local_selection_parent->displayHints()) {
                 if ((local_selection_parent->displayHints()->activityControlHint() == ObserverHints::CheckboxTriggered))
                     item_flags |= Qt::ItemIsUserCheckable;
                 else
                     item_flags &= ~Qt::ItemIsUserCheckable;
             } else {
                 item_flags &= ~Qt::ItemIsUserCheckable;
             }
         } else {
             item_flags &= ~Qt::ItemIsUserCheckable;
         }
     }

     // Handle drag & drop hints:
     if (index.column() != columnPosition(ColumnName)) {
         item_flags &= ~Qt::ItemIsDragEnabled;
         if (!d->read_only)
            item_flags &= ~Qt::ItemIsDropEnabled;
     } else {
         if (item->itemType() == ObserverTreeItem::CategoryItem) {
             item_flags &= ~Qt::ItemIsDragEnabled;
             item_flags &= ~Qt::ItemIsDropEnabled;
         } else if (item->itemType() == ObserverTreeItem::TreeItem) {
             // For items we need to check the drag drop hint of the parent of the index:
             Observer* local_selection_parent = parentOfIndex(index);
             if (local_selection_parent) {
                 // We need to check a few things things:
                 // 1. Do we the observer hints?
                 // 2. If so, does the observer have hints?
                 ObserverHints* hints_to_use = 0;
                 if (!model->use_observer_hints)
                     hints_to_use = activeHints();
                 else {
                     if (local_selection_parent->displayHints())
                        hints_to_use = local_selection_parent->displayHints();
                 }

                 if (hints_to_use) {
                     // Check if drags are allowed:
                     if (hints_to_use->dragDropHint() & ObserverHints::AllowDrags)
                         item_flags |= Qt::ItemIsDragEnabled;
                     else
                         item_flags &= ~Qt::ItemIsDragEnabled;

                 } else {
                     item_flags &= ~Qt::ItemIsDragEnabled;
                 }
             } else {
                 item_flags &= ~Qt::ItemIsDragEnabled;
             }
             item_flags &= ~Qt::ItemIsDropEnabled;
         } else if (item->itemType() == ObserverTreeItem::TreeNode) {
             // Check if the node (observer) accepts drops or allows drags:
             Observer* obs = qobject_cast<Observer*> (item->getObject());
             if (obs) {
                 // We need to check a few things:
                 // 1. Do we the observer hints?
                 // 2. If so, does the observer have hints?
                 ObserverHints* hints_to_use = 0;
                 if (!model->use_observer_hints)
                     hints_to_use = activeHints();
                 else {
                     if (obs->displayHints())
                        hints_to_use = obs->displayHints();
                 }

                 if (hints_to_use) {
                     // Check if drags are allowed:
                     if (hints_to_use->dragDropHint() & ObserverHints::AllowDrags)
                         item_flags |= Qt::ItemIsDragEnabled;
                     else
                         item_flags &= ~Qt::ItemIsDragEnabled;
                     // Check if drops are accepted:
                     if ((hints_to_use->dragDropHint() & ObserverHints::AcceptDrops) && !d->read_only)
                         item_flags |= Qt::ItemIsDropEnabled;
                     else
                         item_flags &= ~Qt::ItemIsDropEnabled;
                 } else {
                     item_flags &= ~Qt::ItemIsDragEnabled;
                     item_flags &= ~Qt::ItemIsDropEnabled;
                 }
             } else {
                 item_flags &= ~Qt::ItemIsDragEnabled;
                 item_flags &= ~Qt::ItemIsDropEnabled;
             }
         }
     }

     return item_flags;
}

QVariant Qtilities::CoreGui::ObserverTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if ((section == columnPosition(ColumnName)) && (orientation == Qt::Horizontal) && (role == Qt::DisplayRole)) {
        if (d->type_grouping_name.isEmpty())
            return tr("Contents Tree");
        else
            return d->type_grouping_name;
    } else if ((section == columnPosition(ColumnChildCount)) && (orientation == Qt::Horizontal) && (role == Qt::DecorationRole)) {
        return QIcon(qti_icon_CHILD_COUNT_22x22);
    } else if ((section == columnPosition(ColumnChildCount)) && (orientation == Qt::Horizontal) && (role == Qt::ToolTipRole)) {
        return tr("Child Count");
    } else if ((section == columnPosition(ColumnTypeInfo)) && (orientation == Qt::Horizontal) && (role == Qt::DecorationRole)) {
        return QIcon(qti_icon_TYPE_INFO_22x22);
    } else if ((section == columnPosition(ColumnTypeInfo)) && (orientation == Qt::Horizontal) && (role == Qt::ToolTipRole)) {
        return tr("Type");
    } else if ((section == columnPosition(ColumnAccess)) && (orientation == Qt::Horizontal) && (role == Qt::DecorationRole)) {
        return QIcon(qti_icon_ACCESS_16x16);
    } else if ((section == columnPosition(ColumnAccess)) && (orientation == Qt::Horizontal) && (role == Qt::ToolTipRole)) {
        return tr("Access");
    }

     return QVariant();
}

QModelIndex Qtilities::CoreGui::ObserverTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (!d->tree_model_up_to_date)
        return QModelIndex();

    if (!hasIndex(row, column, parent) || !d->rootItem)
        return QModelIndex();

    ObserverTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<ObserverTreeItem*>(parent.internalPointer());

    ObserverTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex Qtilities::CoreGui::ObserverTreeModel::parent(const QModelIndex &index) const {
    if (!d->tree_model_up_to_date)
        return QModelIndex();

    if (!index.isValid())
        return QModelIndex();

    ObserverTreeItem *childItem = getItem(index);
    if (!childItem)
        return QModelIndex();
    ObserverTreeItem *parentItem = childItem->parentItem();
    if (!parentItem)
        return QModelIndex();
    if (!parentItem->getObject())
        return QModelIndex();

    if (parentItem == d->rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool Qtilities::CoreGui::ObserverTreeModel::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) {
    Q_UNUSED(data)
    Q_UNUSED(action)

    if (!d->tree_model_up_to_date)
        return false;

    Observer* obs = 0;
    if (row == -1 && column == -1) {
        // In this case we need to check if we dropped on an observer, in that case
        // we don't get the parent, we use parent.
        ObserverTreeItem* item = getItem(parent);
        if (item) {
            if (item->getObject()) {
                obs = qobject_cast<Observer*> (item->getObject());
            }
        }

        if (!obs)
            obs = parentOfIndex(parent);
    } else
        obs = parentOfIndex(index(row,column,parent));
    if (obs) {
        const ObserverMimeData* observer_mime_data = qobject_cast<const ObserverMimeData*> (CLIPBOARD_MANAGER->mimeData());
        if (observer_mime_data) {
            // Try to attach it:
            QString error_msg;
            if (d_observer->canAttach(const_cast<ObserverMimeData*> (observer_mime_data),&error_msg) == Observer::Allowed) {
                // Now check the proposed action of the event.
                if (observer_mime_data->dropAction() == Qt::MoveAction) {
                    // Attempt to move the dragged objects:
                    OBJECT_MANAGER->moveSubjects(observer_mime_data->subjectList(),observer_mime_data->sourceID(),d_observer->observerID());
                } else if (observer_mime_data->dropAction() == Qt::CopyAction) {
                    // Attempt to copy the dragged objects:
                    // Either do all or nothing:
                    if (obs->canAttach(const_cast<ObserverMimeData*> (observer_mime_data),0,true) != Observer::Rejected) {
                        QList<QPointer<QObject> > dropped_list = obs->attachSubjects(const_cast<ObserverMimeData*> (observer_mime_data));
                        if (dropped_list.count() != observer_mime_data->subjectList().count()) {
                            LOG_WARNING(QString(tr("The drop operation completed partially. %1/%2 objects were drop successfully.").arg(dropped_list.count()).arg(observer_mime_data->subjectList().count())));
                        } else {
                            LOG_INFO(QString(tr("The drop operation completed successfully on %1 objects.").arg(dropped_list.count())));
                        }
                    }
                }
            } else
                LOG_ERROR(QString(tr("The drop operation could not be completed. All objects could not be accepted by the destination context. Error message: ") + error_msg));
        } else
            LOG_ERROR(QString(tr("The drop operation could not be completed. The clipboard manager does not contain a valid mime data object.")));
    } else
        LOG_ERROR(QString(tr("The drop operation could not be completed. A suitable context to place your dropped data in could not be found.")));

    CLIPBOARD_MANAGER->clearMimeData();
    return true;
}

Qt::DropActions Qtilities::CoreGui::ObserverTreeModel::supportedDropActions () const {
    if (!d->tree_model_up_to_date)
        return Qt::IgnoreAction;

    Qt::DropActions drop_actions = 0;
    if (!d->read_only)
        drop_actions |= Qt::CopyAction;
    //drop_actions |= Qt::MoveAction;
    return drop_actions;
}

/*void Qtilities::CoreGui::ObserverTreeModel::dropEvent(QDropEvent* event) {
    // This function is called for Qt::MoveAction:
    if (event->proposedAction() == Qt::MoveAction || event->proposedAction() == Qt::CopyAction) {
        const ObserverMimeData* observer_mime_data = qobject_cast<const ObserverMimeData*> (event->mimeData());
        if (observer_mime_data) {
            if (observer_mime_data->sourceID() == d_observer->observerID()) {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Drop Operation Failed"));
                msgBox.setText(QString(tr("The drop operation could not be completed. The destination and source is the same.")));
                msgBox.exec();
                return;
            }

            if (obs->canAttach(const_cast<ObserverMimeData*> (observer_mime_data)) == Observer::Allowed) {
                // Now check the proposed action of the event.
                if (event->proposedAction() == Qt::MoveAction) {
                    event->accept();
                    OBJECT_MANAGER->moveSubjects(observer_mime_data->subjectList(),observer_mime_data->sourceID(),obs->observerID());
                } else if (event->proposedAction() == Qt::CopyAction) {
                    event->accept();

                    // Attempt to copy the dragged objects
                    // For now we discard objects that cause problems during attachment and detachment
                    for (int i = 0; i < observer_mime_data->subjectList().count(); i++) {
                        // Attach to destination
                        if (!obs->attachSubject(observer_mime_data->subjectList().at(i))) {
                            QMessageBox msgBox;
                            msgBox.setWindowTitle(tr("Drop Operation Failed"));
                            msgBox.setText(QString(tr("Attachment of your object(s) failed in the destination context.")));
                            msgBox.exec();
                        }
                    }
                }
            } else {
                QMessageBox msgBox;
                msgBox.setWindowTitle(tr("Drop Operation Failed"));
                msgBox.setText(QString(tr("The drop operation could not be completed. The destination observer cannot accept all the objects in your selection.")));
                msgBox.exec();
            }
        }
    }

    return;
}*/

bool Qtilities::CoreGui::ObserverTreeModel::setData(const QModelIndex &set_data_index, const QVariant &value, int role) {
    if (!d->tree_model_up_to_date)
        return false;

    if (d->read_only)
        return false;

    if (set_data_index.column() == columnPosition(AbstractObserverItemModel::ColumnName)) {
        if (role == Qt::EditRole || role == Qt::DisplayRole) {
            ObserverTreeItem* item = getItem(set_data_index);
            QObject* obj = item->getObject();
            Observer* local_selection_parent = d->selection_parent;

            if (!local_selection_parent)
                return false;

            // Check if the object has a qti_prop_NAME property, if not we set the name using setObjectName()
            if (obj) {
                if (ObjectManager::getSharedProperty(obj, qti_prop_NAME).isValid()) {
                    // Now check if this observer uses an instance name
                    MultiContextProperty instance_names = ObjectManager::getMultiContextProperty(obj,qti_prop_ALIAS_MAP);
                    if (instance_names.isValid() && instance_names.hasContext(local_selection_parent->observerID()))
                        local_selection_parent->setMultiContextPropertyValue(obj,qti_prop_ALIAS_MAP,value);
                    else
                        local_selection_parent->setMultiContextPropertyValue(obj,qti_prop_NAME,value);
                } else {
                    obj->setObjectName(value.toString());
                }
            }

            // We cannot emit dataChanged(index,index) here since changing the name might do a replace
            // and then the persistant indexes are out of date.
            return true;
        } else if (role == Qt::CheckStateRole) {
            ObserverTreeItem* item = getItem(set_data_index);
            QObject* obj = item->getObject();
            Observer* local_selection_parent = parentOfIndex(set_data_index);

            if (local_selection_parent) {
                if (local_selection_parent->displayHints()) {
                    if (local_selection_parent->displayHints()->activityControlHint() == ObserverHints::CheckboxTriggered && local_selection_parent->displayHints()->activityDisplayHint() == ObserverHints::CheckboxActivityDisplay) {
                        // Now we need to check if the observer has an activity policy filter installed
                        ActivityPolicyFilter* activity_filter = 0;
                        for (int i = 0; i < local_selection_parent->subjectFilters().count(); i++) {
                            activity_filter = qobject_cast<ActivityPolicyFilter*> (local_selection_parent->subjectFilters().at(i));
                            if (activity_filter) {
                                // The value comming in here is always Qt::Checked.
                                // We get the current check state from the qti_prop_ACTIVITY_MAP property and change that.
                                QVariant current_activity = local_selection_parent->getMultiContextPropertyValue(obj,qti_prop_ACTIVITY_MAP);
                                if (current_activity.toBool()) {
                                    local_selection_parent->setMultiContextPropertyValue(obj,qti_prop_ACTIVITY_MAP,QVariant(false));
                                } else {
                                    local_selection_parent->setMultiContextPropertyValue(obj,qti_prop_ACTIVITY_MAP,QVariant(true));
                                }

                                handleContextDataChanged(local_selection_parent);
                            }
                        }
                    }
                }
            }
            return true;
        }
    }
    return false;
}

int Qtilities::CoreGui::ObserverTreeModel::rowCount(const QModelIndex &parent) const {
    if (!d->tree_model_up_to_date)
        return 0;

    ObserverTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<ObserverTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int Qtilities::CoreGui::ObserverTreeModel::columnCount(const QModelIndex &parent) const {
    if (!d->tree_model_up_to_date)
        return columnPosition(AbstractObserverItemModel::ColumnLast) + 1;

    if (parent.isValid()) {
         return columnPosition(AbstractObserverItemModel::ColumnLast) + 1;
     } else
         return d->rootItem->columnCount();
}

void Qtilities::CoreGui::ObserverTreeModel::recordObserverChange(QList<QPointer<QObject> > new_selection) {
    if (d->tree_model_up_to_date) {
        d->new_selection = new_selection;
        //qDebug() << "Recording observer change on model: " << objectName() << ". The tree was up to date, thus rebuilding it.";
        rebuildTreeStructure();
    } else {
        d->tree_rebuild_queued = true;
        d->queued_selection = new_selection;
        //qDebug() << QString("Received tree rebuild request in " + d_observer->observerName() + "'s view. The current tree is being rebuilt, thus queueing this change.");
    }
}

void Qtilities::CoreGui::ObserverTreeModel::clearTreeStructure() {
    #ifdef QTILITIES_BENCHMARKING
    qDebug() << "Clearing tree structure on view: " << objectName();
    #endif

    beginResetModel();
    d->tree_model_up_to_date = false;
    deleteRootItem();
    QVector<QVariant> columns;
    columns.push_back(QString("Child Count"));
    columns.push_back(QString("Access"));
    columns.push_back(QString("Type Info"));
    columns.push_back(QString("Object Tree"));
    d->rootItem = new ObserverTreeItem(0,0,columns);
    d->rootItem->setObjectName("Root Item");

    d->tree_model_up_to_date = true;
    endResetModel();
}

void Qtilities::CoreGui::ObserverTreeModel::rebuildTreeStructure() {
    #ifdef QTILITIES_BENCHMARKING
    qDebug() << "Rebuilding tree structure on view: " << objectName();
    #endif

    // The view will call setExpandedItems() in its slot.
    emit treeModelBuildAboutToStart();

    // Rebuild the tree structure:
    beginResetModel();
    //reset();
    d->tree_model_up_to_date = false;

    emit treeModelBuildStarted(d->tree_builder.taskID());
    deleteRootItem();
    QVector<QVariant> columns;
    columns.push_back(QString(tr("Child Count")));
    columns.push_back(QString(tr("Access")));
    columns.push_back(QString(tr("Type Info")));
    columns.push_back(QString(tr("Object Tree")));
    d->rootItem = new ObserverTreeItem(0,0,columns);
    d->rootItem->setObjectName("Root Item");
    ObserverTreeItem* top_level_observer_item = new ObserverTreeItem(d_observer,d->rootItem);
    d->rootItem->appendChild(top_level_observer_item);

    d->tree_rebuild_queued = false;

    if (d->tree_building_threading_enabled) {
        d->tree_builder.setRootItem(top_level_observer_item);
        d->tree_builder.setUseObserverHints(model->use_observer_hints);
        d->tree_builder.setActiveHints(activeHints());
        d->tree_builder.setThreadingEnabled(true);

        top_level_observer_item->setParent(0);
        top_level_observer_item->moveToThread(&d->tree_builder_thread);
        d->tree_builder.moveToThread(&d->tree_builder_thread);
        d->tree_builder.setOriginThread(thread());

        connect(&d->tree_builder,SIGNAL(buildCompleted(ObserverTreeItem*)),SLOT(receiveBuildObserverTreeItem(ObserverTreeItem*)));

        d->tree_builder_thread.start();
        QMetaObject::invokeMethod(&d->tree_builder, "startBuild", Qt::QueuedConnection);
    } else {
        d->tree_rebuild_queued = false;
        //top_level_observer_item->moveToThread(&d->tree_builder_thread);
        d->tree_builder.setRootItem(top_level_observer_item);
        d->tree_builder.setUseObserverHints(model->use_observer_hints);
        d->tree_builder.setActiveHints(activeHints());
        connect(&d->tree_builder,SIGNAL(buildCompleted(ObserverTreeItem*)),SLOT(receiveBuildObserverTreeItem(ObserverTreeItem*)),Qt::UniqueConnection);
        d->tree_builder.startBuild();
    }
}

void Qtilities::CoreGui::ObserverTreeModel::receiveBuildObserverTreeItem(ObserverTreeItem* item) {
    //qDebug() << "Received prebuilt model from tree builder on view for observer: " << d_observer->observerName();

    item->setParent(d->rootItem);

    if (d->tree_building_threading_enabled) {
        d->tree_builder_thread.quit();
        disconnect(&d->tree_builder,SIGNAL(buildCompleted(ObserverTreeItem*)),this,SLOT(receiveBuildObserverTreeItem(ObserverTreeItem*)));
    }

    QList<QPointer<QObject> > new_selection;
    if (d->tree_rebuild_queued) {
        new_selection = d->queued_selection;
    } else {
        new_selection = d->new_selection;
    }

    //emit layoutAboutToBeChanged();
    d->tree_model_up_to_date = true;

    //emit layoutChanged();
    endResetModel();
    emit treeModelBuildEnded();

    if (d->tree_rebuild_queued) {
        rebuildTreeStructure();
    } else {
        // Handle item selection after tree has been rebuilt:
        if (d->new_selection.count() > 0) {
            emit selectObjects(new_selection);
        } else if (d->selected_objects.count() > 0)
            emit selectObjects(d->selected_objects);
        else
            emit selectObjects(QList<QPointer<QObject> >());

        // Restore expanded items:
        QModelIndexList complete_match_list;
        foreach (QString item, d->expanded_items) {
            complete_match_list.append(match(index(0,columnPosition(AbstractObserverItemModel::ColumnName)),Qt::DisplayRole,QVariant::fromValue(item),1,Qt::MatchRecursive));
            complete_match_list.append(match(index(0,columnPosition(AbstractObserverItemModel::ColumnName)),Qt::DisplayRole,QVariant::fromValue(item + "*"),1,Qt::MatchRecursive));
        }
        emit expandItemsRequest(complete_match_list);
    }
}

Qtilities::Core::Observer* Qtilities::CoreGui::ObserverTreeModel::calculateSelectionParent(QModelIndexList index_list) {
    if (index_list.count() == 1) {
        d->selection_parent = parentOfIndex(index_list.front());
        d->selection_index = index_list.front();

        // Do some hints debugging:
        /*if (d->selection_parent) {
            qDebug() << "New selection parent: " << d->selection_parent->objectName() << " with display hints: " << d->selection_parent->displayHints()->displayFlagsHint();
        }*/

        // Get the hints from the observer:
        if (d->selection_parent) {
            model->hints_selection_parent = d->selection_parent->displayHints();
        }

        emit selectionParentChanged(d->selection_parent);
        return d->selection_parent;
    } else {
        return 0;
    }
}

int Qtilities::CoreGui::ObserverTreeModel::getSubjectID(const QModelIndex &index) const {
    QObject* obj = getItem(index)->getObject();
    if (d_observer)
        return d_observer->getMultiContextPropertyValue(obj,qti_prop_OBSERVER_MAP).toInt();
    else
        return -1;
}

QObject* Qtilities::CoreGui::ObserverTreeModel::getObject(const QModelIndex &index) const {
    ObserverTreeItem* item = getItem(index);
    if (item)
        return item->getObject();
    else
        return 0;
}

Qtilities::Core::Observer* Qtilities::CoreGui::ObserverTreeModel::selectionParent() const {
    return d->selection_parent;
}

Qtilities::Core::Observer* Qtilities::CoreGui::ObserverTreeModel::parentOfIndex(const QModelIndex &index) const {
    Observer* local_selection_parent = 0;
    ObserverTreeItem* item = getItem(index);
    if (!item)
        return 0;

    if (item->itemType() != ObserverTreeItem::CategoryItem) {
        ObserverTreeItem* item_parent = item->parentItem();
        if (!item_parent)
            return 0;
        if (item_parent->getObject())
            local_selection_parent = qobject_cast<Observer*> (item_parent->getObject());
        else
            return 0;

        if (!local_selection_parent) {
            // Handle the cases where the parent is a category item
            if (item_parent->itemType() == ObserverTreeItem::CategoryItem) {
                local_selection_parent = item_parent->containedObserver();
            }

            // Handle the cases where the parent is an object which has this object's parent observer as its child.
            if (!local_selection_parent) {
                if (item_parent->getObject()) {
                    for (int i = 0; i < item_parent->getObject()->children().count(); i++) {
                        local_selection_parent = qobject_cast<Observer*> (item_parent->getObject()->children().at(i));
                        if (local_selection_parent)
                            break;
                    }
                }
            }
        }
    } else {
        local_selection_parent = item->containedObserver();
    }
    return local_selection_parent;  
}

void Qtilities::CoreGui::ObserverTreeModel::handleContextDataChanged(Observer* observer) {
    if (!observer)
        return;

    handleContextDataChanged(findObject(observer));
}

void Qtilities::CoreGui::ObserverTreeModel::handleContextDataChanged(const QModelIndex &set_data_index) {
    // We get the indexes for the complete context since activity of many objects might change:
    // Warning: This is not going to work for categorized hierarchy observers.
    QModelIndex parent_index = parent(set_data_index);
    int column_count = columnVisiblePosition(ColumnLast);
    int row_count = rowCount(parent_index) - 1;
    QModelIndex top_left = index(0,0,parent_index);
    QModelIndex bottom_right = index(row_count,column_count,parent_index);
    if (hasIndex(row_count,column_count,parent_index))
        emit dataChanged(top_left,bottom_right);
}

void Qtilities::CoreGui::ObserverTreeModel::setSelectedObjects(QList<QPointer<QObject> > selected_objects) {
    d->selected_objects = selected_objects;
}

void Qtilities::CoreGui::ObserverTreeModel::setReadOnly(bool read_only) {
    if (d->read_only == read_only)
        return;

    d->read_only = read_only;
}

bool Qtilities::CoreGui::ObserverTreeModel::readOnly() const {
    return d->read_only;
}

QModelIndexList Qtilities::CoreGui::ObserverTreeModel::getPersistentIndexList() const {
    return persistentIndexList();
}

void Qtilities::CoreGui::ObserverTreeModel::setExpandedItems(QStringList expanded_items) {
    d->expanded_items = expanded_items;
}

QModelIndex Qtilities::CoreGui::ObserverTreeModel::findObject(QObject* obj) const {
    // Loop through all items indexes in the tree and check the object of each ObserverTreeItem at each index.
    QModelIndex root = index(0,0);
    return findObject(root,obj);
}

QModelIndex Qtilities::CoreGui::ObserverTreeModel::findObject(const QModelIndex& root, QObject* obj) const {
    QModelIndex correct_index;
    if (root.isValid()) {
        // Check this index:
        ObserverTreeItem* item = getItem(root);
        if (item) {
            if (item->getObject() == obj) {
                return root;
            }

            // Ok it was not this index, loop through all children under this index:
            for (int i = 0; i < item->childCount(); i++) {
                QModelIndex child_index = index(i,0,root);
                correct_index = findObject(child_index,obj);
                if (correct_index.isValid()) {
                    return correct_index;
                }
            }
        }
    }
    return QModelIndex();
}

QModelIndexList Qtilities::CoreGui::ObserverTreeModel::getAllIndexes(ObserverTreeItem* item) const {
    static QModelIndexList indexes;

    if (!item) {
        indexes.clear();
        item = getItem(index(0,0));
    }

    if (!item)
        return indexes;

    // Add root and call getAllIndexes on all its children.
    QModelIndex index = findObject(item->getObject());
    indexes << index;
    for (int i = 0; i < item->childCount(); i++) {
        getAllIndexes(item->child(i));
    }

    return indexes;
}

Qtilities::CoreGui::ObserverTreeItem* Qtilities::CoreGui::ObserverTreeModel::findObject(ObserverTreeItem* item, QObject* obj) const {
    // Check item:
    if (item->getObject() == obj)
        return item;

    // Check all the children of item and traverse into them.
    for (int i = 0; i < item->childCount(); i++) {
        ObserverTreeItem* tree_item = findObject(item->childItemReferences().at(i),obj);
        if (tree_item)
            return tree_item;
    }

    return 0;
}

void Qtilities::CoreGui::ObserverTreeModel::deleteRootItem() {
    if (!d->rootItem)
        return;

    delete d->rootItem;
    d->rootItem = 0;
}

Qtilities::CoreGui::ObserverTreeItem* Qtilities::CoreGui::ObserverTreeModel::getItem(const QModelIndex &index) const {
    if (index.isValid()) {
        ObserverTreeItem *item = static_cast<ObserverTreeItem*>(index.internalPointer());
        if (item)
            return item;
    }

    return 0;
}

void Qtilities::CoreGui::ObserverTreeModel::handleObserverContextDeleted() {
    clearTreeStructure();
    d->selection_parent = 0;
}

