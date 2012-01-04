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

#include "ObserverTreeModelBuilder.h"
#include <QtilitiesCoreGui>

#include <stdio.h>
#include <time.h>

using namespace QtilitiesCoreGui;



struct Qtilities::CoreGui::ObserverTreeModelBuilderPrivateData  {
    ObserverTreeModelBuilderPrivateData() : hints(0),
        root_item(0),
        task("Tree Model Builder"),
        threading_enabled(false) {}

    QMutex                          build_lock;
    ObserverHints*                  hints;
    bool                            use_hints;
    ObserverTreeItem*               root_item;
    Task                            task;
    QThread*                        thread;
    bool                            threading_enabled;
};

Qtilities::CoreGui::ObserverTreeModelBuilder::ObserverTreeModelBuilder(ObserverTreeItem* item, bool use_observer_hints, ObserverHints* observer_hints, QObject* parent) : QObject(parent) {
    d = new ObserverTreeModelBuilderPrivateData;

    d->task.setTaskType(ITask::TaskLocal);
    d->hints = observer_hints;
    d->use_hints = use_observer_hints;
    setRootItem(item);

    OBJECT_MANAGER->registerObject(&d->task,QtilitiesCategory("Tasks"));
}

Qtilities::CoreGui::ObserverTreeModelBuilder::~ObserverTreeModelBuilder() {
    delete d;
}

void Qtilities::CoreGui::ObserverTreeModelBuilder::setOriginThread(QThread* thread) {
    d->thread = thread;
}

void Qtilities::CoreGui::ObserverTreeModelBuilder::setRootItem(ObserverTreeItem* item) {
    d->root_item = item;
    if (d->root_item) {
        d->task.setDisplayName("Loading: " + d->root_item->objectName());
        d->task.setObjectName("Loading Tree: " + d->root_item->objectName());
    }
}

int Qtilities::CoreGui::ObserverTreeModelBuilder::taskID() const {
    return d->task.taskID();
}

void Qtilities::CoreGui::ObserverTreeModelBuilder::setUseObserverHints(bool use_observer_hints) {
    d->use_hints = use_observer_hints;
}

void Qtilities::CoreGui::ObserverTreeModelBuilder::setActiveHints(ObserverHints* active_hints) {
    d->hints = active_hints;
}

void Qtilities::CoreGui::ObserverTreeModelBuilder::setThreadingEnabled(bool is_enabled) {
    d->threading_enabled = is_enabled;
}

void Qtilities::CoreGui::ObserverTreeModelBuilder::startBuild() {
    d->build_lock.lock();

    // Get number of children under observer tree:
    Observer* obs = qobject_cast<Observer*> (d->root_item->getObject());
    int tree_count = -1;
    if (obs)
        tree_count = obs->treeCount();

    //qDebug() << "Tree count: " << tree_count;
    d->task.startTask(tree_count);
    QApplication::processEvents();
    buildRecursive(d->root_item);

    if (d->threading_enabled) {
        d->root_item->moveToThread(d->thread);
        moveToThread(d->thread);
    }

    d->task.completeTask(ITask::TaskSuccessful);
    //printStructure(root_item);
    d->build_lock.unlock();
    emit buildCompleted(d->root_item);
}

void Qtilities::CoreGui::ObserverTreeModelBuilder::buildRecursive(ObserverTreeItem* item) {
    // In here we build the complete structure of all the children below item.
    Observer* observer = qobject_cast<Observer*> (item->getObject());
    ObserverTreeItem* new_item;

    d->task.addCompletedSubTasks(1,"Building item: " + item->objectName());

    if (!d->threading_enabled)
        QApplication::processEvents();

    if (!observer && item->getObject()) {
        // Handle cases where a non-observer based child is the parent of an observer.
        // Observer containment tree building approach.
        foreach (QObject* child, item->getObject()->children()) {
            Observer* child_observer = qobject_cast<Observer*> (child);
            if (child_observer)
                observer = child_observer;
        }
    }

    if (!observer && item->getObject()) {
        // Handle cases where the item is a category item
        if (item->itemType() == ObserverTreeItem::CategoryItem) {
            // Get the observer from the parent of item
            if (item->parentItem()) {
                Observer* parent_observer = item->containedObserver();
                if (parent_observer) {
                    // Now add all items belonging to this category
                    QList<QObject*> objects_for_category = parent_observer->subjectReferencesByCategory(item->category());
                    for (int i = 0; i < objects_for_category.count(); i++) {
                        // Storing all information in the data vector here can improve performance
                        QObject* object = objects_for_category.at(i);
                        Observer* obs = qobject_cast<Observer*> (object);
                        QVector<QVariant> column_data;
                        column_data << QVariant(parent_observer->subjectNameInContext(object));
                        if (obs) {
                            new_item = new ObserverTreeItem(object,item,column_data,ObserverTreeItem::TreeNode);
                        } else {
                            new_item = new ObserverTreeItem(object,item,column_data,ObserverTreeItem::TreeItem);
                        }
                        item->appendChild(new_item);
                        buildRecursive(new_item);
                    }
                }
            }
        }
    }

    if (observer) {
        // If this observer is locked we don't show its children:
        if (observer->accessMode() != Observer::LockedAccess) {
            // Check the HierarchicalDisplay hint of the observer:
            // Remember this is an recursive function, we can't use hints directly since thats linked to the selection parent.
            bool use_categorized;
            ObserverHints* hints_to_use = 0;
            if (d->use_hints) {
                if (observer->displayHints()) {
                    use_categorized = (observer->displayHints()->hierarchicalDisplayHint() == ObserverHints::CategorizedHierarchy);
                    hints_to_use = observer->displayHints();
                } else
                    use_categorized = false;
            } else {
                use_categorized = (d->hints->hierarchicalDisplayHint() == ObserverHints::CategorizedHierarchy);
                hints_to_use = d->hints;
            }

            if (use_categorized) {
                // Create items for each category:
                QList<QtilitiesCategory> categories = observer->subjectCategories();
                foreach (QtilitiesCategory category, categories) {
                    // Check the category against the displayed category list:
                    bool valid_category = true;
                    if (hints_to_use) {
                        QList<QtilitiesCategory> displayed_categories = hints_to_use->displayedCategories();
                        if (hints_to_use->categoryFilterEnabled()) {
                            if (hints_to_use->hasInversedCategoryDisplay()) {
                                if (!displayed_categories.contains(category))
                                    valid_category = true;
                                else
                                    valid_category = false;
                            } else {
                                if (displayed_categories.contains(category))
                                    valid_category = true;
                                else
                                    valid_category = false;
                            }
                        }
                    }

                    // Only add valid categories:
                    if (valid_category) {
                        // Ok here we need to create items for each category level and add the items underneath it.
                        int level_counter = 0;
                        QList<ObserverTreeItem*> tree_item_list;
                        while (level_counter < category.categoryDepth()) {
                            QStringList category_levels = category.toStringList(level_counter+1);

                            // Get the correct parent:
                            ObserverTreeItem* correct_parent;
                            if (tree_item_list.count() == 0)
                                correct_parent = item;
                            else
                                correct_parent = tree_item_list.last();

                            // Check if the parent item already has a category for this level:
                            ObserverTreeItem* existing_item = correct_parent->childWithName(category_levels.last());
                            if (!existing_item) {
                                // Create a category for the first level and add all items under this category to the tree:
                                QVector<QVariant> category_columns;
                                category_columns << category_levels.last();
                                QObject* category_item = new QObject();
                                // Check the access mode of this category and add it to the category object:
                                QtilitiesCategory shortened_category(category_levels);
                                Observer::AccessMode category_access_mode = observer->accessMode(shortened_category);
                                if (category_access_mode != Observer::InvalidAccess) {
                                    SharedProperty access_mode_property(qti_prop_ACCESS_MODE,(int) observer->accessMode(shortened_category));
                                    ObjectManager::setSharedProperty(category_item,access_mode_property);
                                }
                                category_item->setObjectName(category_levels.last());

                                // Create new item:
                                new_item = new ObserverTreeItem(category_item,correct_parent,category_columns,ObserverTreeItem::CategoryItem);
                                new_item->setContainedObserver(observer);
                                new_item->setCategory(category_levels);

                                // Append new item to correct parent item:
                                if (tree_item_list.count() == 0)
                                    item->appendChild(new_item);
                                else
                                    tree_item_list.last()->appendChild(new_item);
                                tree_item_list.push_back(new_item);

                                // If this item has locked access, we don't dig into any items underneath it:
                                if (observer->accessMode(shortened_category) != Observer::LockedAccess) {
                                    buildRecursive(new_item);
                                } else {
                                    break;
                                }
                            } else {
                                tree_item_list.push_back(existing_item);
                            }

                            // Increment the level counter:
                            ++level_counter;
                        }
                    }
                }

                // Here we need to add all items which do not belong to a specific category:
                // Get the list of uncategorized items from the observer
                QList<QObject*> uncat_list = observer->subjectReferencesByCategory(QtilitiesCategory());
                QStringList uncat_names = observer->subjectNamesByCategory(QtilitiesCategory());
                for (int i = 0; i < uncat_list.count(); i++) {
                    Observer* obs = qobject_cast<Observer*> (uncat_list.at(i));
                    QVector<QVariant> column_data;
                    column_data << QVariant(uncat_names.at(i));
                    if (obs) {
                        new_item = new ObserverTreeItem(uncat_list.at(i),item,column_data,ObserverTreeItem::TreeNode);;
                        item->appendChild(new_item);
                        // If this item has locked access, we don't dig into any items underneath it:
                        if (obs->accessMode(QtilitiesCategory()) != Observer::LockedAccess)
                            buildRecursive(new_item);
                    } else {
                        new_item = new ObserverTreeItem(uncat_list.at(i),item,column_data,ObserverTreeItem::TreeItem);
                        item->appendChild(new_item);
                    }
                }
            } else {
                for (int i = 0; i < observer->subjectCount(); i++) {
                    // Storing all information in the data vector here can improve performance:
                    Observer* obs = qobject_cast<Observer*> (observer->subjectAt(i));
                    QVector<QVariant> column_data;
                    column_data << QVariant(observer->subjectNames().at(i));
                    if (obs)
                        new_item = new ObserverTreeItem(observer->subjectAt(i),item,column_data,ObserverTreeItem::TreeNode);
                    else
                        new_item = new ObserverTreeItem(observer->subjectAt(i),item,column_data,ObserverTreeItem::TreeItem);
                    item->appendChild(new_item);
                    buildRecursive(new_item);
                }
            }
        }
    }
}

void Qtilities::CoreGui::ObserverTreeModelBuilder::printStructure(ObserverTreeItem* item, int level) {
    if (level == 0) {
        item = d->root_item;
        qDebug() << "Tree Debug (" << level << "): Object = " << item->objectName() << ", Parent = None, Child Count = " << item->childCount();
    } else
        qDebug() << "Tree Debug (" << level << "): Object = " << item->objectName() << ", Parent = " << item->parentItem()->objectName() << ", Child Count = " << item->childCount();

    for (int i = 0; i < item->childCount(); i++) {
        printStructure(item->child(i),level+1);
    }
}
