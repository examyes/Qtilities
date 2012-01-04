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

#include "ObserverData.h"
#include "ObserverHints.h"
#include "IExportableFormatting.h"
#include "ActivityPolicyFilter.h"
#include "ObserverRelationalTable.h"

#include <stdio.h>
#include <time.h>

#include <QDomElement>

using namespace Qtilities::Core::Interfaces;

quint32 MARKER_OBS_DATA_SECTION = 0xDEADBEEF;

void Qtilities::Core::ObserverData::setExportVersion(Qtilities::ExportVersion version) {
    IExportable::setExportVersion(version);

    if (display_hints)
        display_hints->setExportVersion(version);

    // This is a bad way to do it... Fix sometime.
    QList<QtilitiesCategory> new_categories;
    for (int i = 0; i < categories.count(); i++) {
        QtilitiesCategory category(categories.at(i));
        category.setExportVersion(exportVersion());
        new_categories << category;
    }

    categories.clear();
    categories.append(new_categories);
}

Qtilities::Core::Interfaces::IExportable::ExportModeFlags Qtilities::Core::ObserverData::supportedFormats() const {
    IExportable::ExportModeFlags flags = 0;
    flags |= IExportable::Binary;
    flags |= IExportable::XML;

    return flags;
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::ObserverData::exportBinary(QDataStream& stream) const {
    #ifdef QTILITIES_BENCHMARKING
    time_t start,end;
    time(&start);
    #endif

    if (exportVersion() == Qtilities::Qtilities_1_0) {
        IExportable::Result result = exportBinaryExt_1_0(stream,ExportData);
        #ifdef QTILITIES_BENCHMARKING
        time(&end);
        double diff = difftime(end,start);
        LOG_WARNING("Observer (" + observer->observerName() + ") took " + QString::number(diff) + " seconds to export (Binary -> Qtilities_1_0).");
        #endif
        return result;
    } else if (exportVersion() < Qtilities::Qtilities_1_0)
        return IExportable::FailedTooOld;
    else if (exportVersion() > Qtilities::Qtilities_1_0)
        return IExportable::FailedTooNew;

    return IExportable::Incomplete;
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::ObserverData::importBinary(QDataStream& stream, QList<QPointer<QObject> >& import_list) {
    #ifdef QTILITIES_BENCHMARKING
    time_t start,end;
    time(&start);
    #endif

    if (exportVersion() == Qtilities::Qtilities_1_0) {
        IExportable::Result result = importBinaryExt_1_0(stream,import_list);
        #ifdef QTILITIES_BENCHMARKING
        time(&end);
        double diff = difftime(end,start);
        LOG_WARNING("Observer (" + observer->observerName() + ") took " + QString::number(diff) + " seconds to import (Binary -> Qtilities_1_0).");
        #endif
        return result;
    } else if (exportVersion() < Qtilities::Qtilities_1_0)
        return IExportable::FailedTooOld;
    else if (exportVersion() > Qtilities::Qtilities_1_0)
        return IExportable::FailedTooNew;

    return IExportable::Incomplete;
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::ObserverData::exportXml(QDomDocument* doc, QDomElement* object_node) const {
    #ifdef QTILITIES_BENCHMARKING
    time_t start,end;
    time(&start);
    #endif

    if (exportVersion() == Qtilities::Qtilities_1_0) {
        IExportable::Result result = exportXmlExt_1_0(doc,object_node,ExportData);
        #ifdef QTILITIES_BENCHMARKING
        time(&end);
        double diff = difftime(end,start);
        LOG_WARNING("Observer (" + observer->observerName() + ") took " + QString::number(diff) + " seconds to export (XML -> Qtilities_1_0).");
        #endif
        return result;
    } else if (exportVersion() < Qtilities::Qtilities_1_0)
        return IExportable::FailedTooOld;
    else if (exportVersion() > Qtilities::Qtilities_1_0)
        return IExportable::FailedTooNew;

    return IExportable::Incomplete;
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::ObserverData::importXml(QDomDocument* doc, QDomElement* object_node, QList<QPointer<QObject> >& import_list) {
    #ifdef QTILITIES_BENCHMARKING
    time_t start,end;
    time(&start);
    #endif

    if (exportVersion() == Qtilities::Qtilities_1_0) {
        IExportable::Result result = importXmlExt_1_0(doc,object_node,import_list);
        #ifdef QTILITIES_BENCHMARKING
        time(&end);
        double diff = difftime(end,start);
        LOG_WARNING("Observer (" + observer->observerName() + ") took " + QString::number(diff) + " seconds to import (XML -> Qtilities_1_0).");
        #endif
        return result;
    } else if (exportVersion() < Qtilities::Qtilities_1_0)
        return IExportable::FailedTooOld;
    else if (exportVersion() > Qtilities::Qtilities_1_0)
        return IExportable::FailedTooNew;

    return IExportable::Incomplete;
}

IExportable::Result Qtilities::Core::ObserverData::exportBinaryExt(QDataStream& stream, ExportItemFlags export_flags) const {
    if (exportVersion() == Qtilities::Qtilities_1_0)
        return exportBinaryExt_1_0(stream,export_flags);
    else if (exportVersion() < Qtilities::Qtilities_1_0)
        return IExportable::FailedTooOld;
    else if (exportVersion() > Qtilities::Qtilities_1_0)
        return IExportable::FailedTooNew;

    return IExportable::Incomplete;
}

IExportable::Result Qtilities::Core::ObserverData::exportXmlExt(QDomDocument* doc, QDomElement* object_node, ExportItemFlags export_flags) const {
    if (exportVersion() == Qtilities::Qtilities_1_0)
        return exportXmlExt_1_0(doc,object_node,export_flags);
    else if (exportVersion() < Qtilities::Qtilities_1_0)
        return IExportable::FailedTooOld;
    else if (exportVersion() > Qtilities::Qtilities_1_0)
        return IExportable::FailedTooNew;

    return IExportable::Incomplete;
}

IExportable::Result Qtilities::Core::ObserverData::exportBinaryExt_1_0(QDataStream& stream, ExportItemFlags export_flags) const {
    stream << MARKER_OBS_DATA_SECTION;
    // Export the flags used:
    stream << (quint32) export_flags;

    // We define a succesfull operation as an export which is able to export all subjects.
    bool success = true;
    bool complete = true;

    ObserverRelationalTable* relational_table = 0;
    if (export_flags & ExportRelationalData) {
        // Export relational data about the observer:
        relational_table = new ObserverRelationalTable(observer,true);
        relational_table->setExportVersion(Qtilities::Qtilities_1_0);
        if (relational_table->exportBinary(stream) != IExportable::Complete) {
            if (relational_table)
                delete relational_table;
            return IExportable::Failed;
        }
    }

    if (export_flags & ExportData) {
        // -----------------------------------
        // Observer Data
        // -----------------------------------
        stream << MARKER_OBS_DATA_SECTION;
        stream << (quint32) subject_limit;
        stream << observer_description;
        stream << (quint32) access_mode;
        stream << (quint32) access_mode_scope;
        stream << (quint32) object_deletion_policy;

        // Visitor ID (only when needed)
        if (export_flags & ExportVisitorIDs) {
            int visitor_id = -1;
            if (ObjectManager::propertyExists(observer,qti_prop_VISITOR_ID)) {
                QVariant prop_variant = observer->property(qti_prop_VISITOR_ID);
                if (prop_variant.isValid() && prop_variant.canConvert<SharedProperty>()) {
                    SharedProperty prop = prop_variant.value<SharedProperty>();
                    if (prop.isValid()) {
                         visitor_id = prop.value().toInt();
                    }
                }
            }
            stream << (qint32) visitor_id;
        }

        // Stream categories
        stream << (quint32) categories.count();
        for (int i = 0; i < categories.count(); i++) {
            categories.at(i).exportBinary(stream);
        }

        stream << deliver_qtilities_property_changed_events;

        if (display_hints) {
            // Indicates that this observer has hints.
            if (display_hints->isExportable()) {
                stream << (bool) true;
                if (display_hints->exportBinary(stream) != IExportable::Complete) {
                    if (relational_table)
                        delete relational_table;
                    return IExportable::Failed;
                }
            } else {
                stream << (bool) false;
            }
        } else {
            stream << (bool) false;
        }
        stream << MARKER_OBS_DATA_SECTION;

        // -----------------------------------
        // Subject Filters
        // -----------------------------------
        int exportable_filters_count = 0;
        for (int i = 0; i < subject_filters.count(); i++) {
            if (subject_filters.at(i)->isExportable())
                ++exportable_filters_count;
        }

        stream << (quint32) exportable_filters_count;
        // Stream all subject filters:
        for (int i = 0; i < subject_filters.count(); i++) {
            if (subject_filters.at(i)->isExportable()) {
                subject_filters.at(i)->setExportVersion(exportVersion());
                if (!subject_filters.at(i)->instanceFactoryInfo().exportBinary(stream,exportVersion())) {
                    if (relational_table)
                        delete relational_table;
                    return IExportable::Failed;
                }
                if (subject_filters.at(i)->exportBinary(stream) != IExportable::Complete) {
                    if (relational_table)
                        delete relational_table;
                    return IExportable::Failed;
                }
                LOG_TRACE(QString("%1/%2: Exporting subject filter \"%3\"...").arg(i+1).arg(subject_filters.count()).arg(subject_filters.at(i)->filterName()));
            }
        }
        stream << MARKER_OBS_DATA_SECTION;

        // -----------------------------------
        // Make List Of Exportable Subjects
        // -----------------------------------
        QList<IExportable*> exportable_list;
        bool list_complete = true;
        if (export_flags & ExportVisitorIDs)
            exportable_list = getLimitedExportsList(subject_list.toQList(),IExportable::Binary,&list_complete);
        else {
            for (int l = 0; l < subject_list.count(); l++) {
                IExportable* iface = qobject_cast<IExportable*> (subject_list.at(l));
                if (iface)
                    exportable_list << iface;
            }
        }

        if (!list_complete)
            complete = false;

        // -----------------------------------
        // Export List Of Exportable Subjects
        // -----------------------------------
        qint32 iface_count = exportable_list.count();
        stream << iface_count;
        LOG_TRACE(QString(QObject::tr("%1 exportable subjects found under this observer's level of hierarchy.")).arg(iface_count));

        // Now check all subjects for the IExportable interface.
        for (int i = 0; i < exportable_list.count(); i++) {
            QCoreApplication::processEvents();
            IExportable* iface = exportable_list.at(i);
            QObject* obj = iface->objectBase();
            LOG_TRACE(QString("%1/%2: Exporting \"%3\"...").arg(i).arg(iface_count).arg(observer->subjectNameInContext(obj)));
            if (!iface->instanceFactoryInfo().exportBinary(stream,exportVersion())) {
                if (relational_table)
                    delete relational_table;
                return IExportable::Failed;
            }

            iface->setExportVersion(exportVersion());
            iface->setApplicationExportVersion(applicationExportVersion());

            // Visitor ID (only when needed)
            if (export_flags & ExportVisitorIDs) {
                int visitor_id = -1;
                if (ObjectManager::propertyExists(iface->objectBase(),qti_prop_VISITOR_ID)) {
                    QVariant prop_variant = iface->objectBase()->property(qti_prop_VISITOR_ID);
                    if (prop_variant.isValid() && prop_variant.canConvert<SharedProperty>()) {
                        SharedProperty prop = prop_variant.value<SharedProperty>();
                        if (prop.isValid()) {
                             visitor_id = prop.value().toInt();
                        }
                    }
                }
                stream << (qint32) visitor_id;
            }

            // Check if it is an observer:
            IExportable::Result result;
            Observer* obs = qobject_cast<Observer*> (iface->objectBase());
            if (obs) {
                 ExportItemFlags child_obs_flags = export_flags;
                 child_obs_flags &= ~ExportRelationalData;

                 IExportableObserver* export_iface_obs = qobject_cast<IExportableObserver*> (obs->objectBase());
                 Q_ASSERT(export_iface_obs);
                 result = export_iface_obs->exportBinaryExt(stream,child_obs_flags);
            } else
                 result = iface->exportBinary(stream);

            // Now export the needed properties about this subject:
            if (result == IExportable::Incomplete || result == IExportable::Failed)
                complete = false;
        }

        stream << MARKER_OBS_DATA_SECTION;
    }

    if (relational_table)
        delete relational_table;

    if (success) {
        if (complete) {
            LOG_DEBUG(QObject::tr("Binary export of observer ") + observer->observerName() + QString(QObject::tr(" was Successful (complete).")));
            return IExportable::Complete;
        } else {
            LOG_DEBUG(QObject::tr("Binary export of observer ") + observer->observerName() + QString(QObject::tr(" was Successful (incomplete).")));
            return IExportable::Incomplete;
        }
    } else {
        LOG_WARNING(QObject::tr("Binary export of observer ") + observer->observerName() + QObject::tr(" failed."));
        return IExportable::Failed;
    }
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::ObserverData::importBinaryExt_1_0(QDataStream& stream, QList<QPointer<QObject> >& import_list) {
    observer->startProcessingCycle();

    quint32 ui32;
    stream >> ui32;
    if (ui32 != MARKER_OBS_DATA_SECTION) {
        LOG_ERROR(QObject::tr("Observer binary import failed to detect marker at start of import. Import will fail at ") + Q_FUNC_INFO);
        observer->endProcessingCycle();
        return IExportable::Failed;
    }

    stream >> ui32;
    ExportModeFlags export_flags = (ExportModeFlags) ui32;

    // We define a succesfull operation as an import which is able to import all subjects.
    bool success = true;
    bool complete = true;

    // Create a custom internal import list which will only store this observer and all its children:
    QList<QPointer<QObject> > internal_import_list;

    // Check if relational data was exported:
    ObserverRelationalTable readback_table;
    if (export_flags & ExportRelationalData) {
        // First stream the relational table
        if (readback_table.importBinary(stream,import_list) == IExportable::Failed)
            return IExportable::Failed;
    }

    if (export_flags & ExportData) {
        stream >> ui32;
        if (ui32 != MARKER_OBS_DATA_SECTION) {
            LOG_ERROR(QObject::tr("Observer binary import failed to detect marker located after factory data. Import will fail at ") + Q_FUNC_INFO);
            observer->endProcessingCycle();
            return IExportable::Failed;
        }

        // -----------------------------------
        // Observer Data
        // -----------------------------------
        stream >> ui32;
        subject_limit = ui32;
        stream >> observer_description;
        stream >> ui32;
        access_mode = ui32;
        stream >> ui32;
        access_mode_scope = ui32;
        stream >> ui32;
        object_deletion_policy = ui32;

        if (export_flags & ExportVisitorIDs) {
            qint32 visitor_id;
            stream >> visitor_id;
            SharedProperty visitor_id_prop(qti_prop_VISITOR_ID,visitor_id);
            ObjectManager::setSharedProperty(observer,visitor_id_prop);
        }

        // Stream categories
        stream >> ui32;
        int category_count = ui32;
        for (int i = 0; i < category_count; i++) {
            QtilitiesCategory category(stream,exportVersion());
            categories.push_back(category);
        }

        stream >> deliver_qtilities_property_changed_events;

        bool has_hints;
        stream >> has_hints;
        if (has_hints) {
            if (!display_hints)
                display_hints = new ObserverHints();
            display_hints->setExportVersion(exportVersion());
            if (display_hints->importBinary(stream,import_list) == IExportable::Failed)
                return IExportable::Failed;
        }

        stream >> ui32;
        if (ui32 != MARKER_OBS_DATA_SECTION) {
            LOG_ERROR(QObject::tr("Observer binary import failed to detect marker located after ObserverData. Import will fail at ") + Q_FUNC_INFO);
            observer->endProcessingCycle();
            return IExportable::Failed;
        }

        // -----------------------------------
        // Subject Filters
        // -----------------------------------
        stream >> ui32;
        int subject_filter_count = ui32;
        for (int i = 0; i < subject_filter_count; i++) {
            // Get the factory data of the subject filter:
            InstanceFactoryInfo instanceFactoryInfo;
            if (!instanceFactoryInfo.importBinary(stream,exportVersion())) {
                observer->endProcessingCycle();
                return IExportable::Failed;
            } else {
                AbstractSubjectFilter* new_filter = qobject_cast<AbstractSubjectFilter*> (OBJECT_MANAGER->createInstance(instanceFactoryInfo));
                if (new_filter) {
                    new_filter->setExportVersion(exportVersion());
                    new_filter->setObjectName(instanceFactoryInfo.d_instance_name);
                    LOG_TRACE(QString("%1/%2: Importing subject filter \"%3\"...").arg(i+1).arg(subject_filter_count).arg(instanceFactoryInfo.d_instance_name));
                    new_filter->importBinary(stream,import_list);
                    observer->installSubjectFilter(new_filter);
                } else {
                    LOG_ERROR(QString(QObject::tr("%1/%2: Importing subject filter \"%3\" failed. Import cannot continue at %4")).arg(i+1).arg(subject_filter_count).arg(instanceFactoryInfo.d_instance_name).arg(Q_FUNC_INFO));
                    observer->endProcessingCycle();
                    return IExportable::Failed;
                }
            }
        }

        stream >> ui32;
        if (ui32 != MARKER_OBS_DATA_SECTION) {
            LOG_ERROR(QObject::tr("Observer binary import failed to detect marker located after subject filters. Import will fail at ") + Q_FUNC_INFO);
            observer->endProcessingCycle();
            return IExportable::Failed;
        }

        // Count the number of IExportable subjects first.
        qint32 iface_count = 0;
        stream >> iface_count;
        LOG_TRACE(QString(QObject::tr("%1 exportable subject(s) found under this observer's level of hierarchy.")).arg(iface_count));

        // Now check all subjects for the IExportable interface.
        for (int i = 0; i < iface_count; i++) {
            QCoreApplication::processEvents();
            if (!success)
                break;

            InstanceFactoryInfo instanceFactoryInfo;
            if (!instanceFactoryInfo.importBinary(stream,exportVersion())) {
                observer->endProcessingCycle();
                return IExportable::Failed;
            }
            if (instanceFactoryInfo.isValid()) {
                LOG_TRACE(QString(QObject::tr("%1/%2: Importing subject type \"%3\" in factory \"%4\"...")).arg(i+1).arg(iface_count).arg(instanceFactoryInfo.d_instance_tag).arg(instanceFactoryInfo.d_factory_tag));

                IFactoryProvider* ifactory = OBJECT_MANAGER->referenceIFactoryProvider(instanceFactoryInfo.d_factory_tag);
                if (ifactory) {
                    QObject* new_instance = ifactory->createInstance(instanceFactoryInfo);
                    if (new_instance) {
                        new_instance->setObjectName(instanceFactoryInfo.d_instance_name);
                        import_list.append(new_instance);
                        IExportable* export_iface = qobject_cast<IExportable*> (new_instance);
                        if (export_iface) {
                            export_iface->setExportVersion(exportVersion());
                            export_iface->setApplicationExportVersion(applicationExportVersion());
                            internal_import_list << export_iface->objectBase();

                            // Get VisitorID if needed:
                            if (export_flags & ExportVisitorIDs) {
                                qint32 visitor_id;
                                stream >> visitor_id;
                                SharedProperty visitor_id_prop(qti_prop_VISITOR_ID,visitor_id);
                                ObjectManager::setSharedProperty(export_iface->objectBase(),visitor_id_prop);
                            }

                            // Check if it is an observer: if so we must use internal_import_list, not import_list:
                            Observer* obs = qobject_cast<Observer*> (export_iface->objectBase());
                            IExportable::Result result;
                            if (obs)
                                result = obs->importBinary(stream,internal_import_list);
                            else
                                result = export_iface->importBinary(stream,import_list);

                            if (result == IExportable::Complete) {
                                success = observer->attachSubject(new_instance,Observer::ObserverScopeOwnership,0,true);
                            } else if (result == IExportable::Incomplete) {
                                success = observer->attachSubject(new_instance,Observer::ObserverScopeOwnership,0,true);
                                complete = false;
                            } else if (result == IExportable::Failed) {
                                success = false;
                            }
                        } else {
                            // Handle deletion of import_list not needed here.
                            success = false;
                            break;
                        }
                    } else {
                        LOG_WARNING(QString(QObject::tr("Factory tag %1 does not exist in the factory %2. This item will be skipped and the import will be incomplete.")).arg(instanceFactoryInfo.d_instance_tag).arg(instanceFactoryInfo.d_factory_tag));
                        complete = false;
                        break;
                    }
                } else {
                    observer->endProcessingCycle();
                    return IExportable::Failed;
                }
            }
        }
        stream >> ui32;
        if (ui32 != MARKER_OBS_DATA_SECTION) {
            LOG_ERROR(QObject::tr("Observer binary import failed to detect end marker. Import will fail at ") + Q_FUNC_INFO);
            observer->endProcessingCycle();
            return IExportable::Failed;
        }
    }

    if (export_flags & ExportRelationalData) {
        internal_import_list << observer;

        // Construct relationships:
        if (!constructRelationships(internal_import_list,&readback_table))
            complete = false;

        // Cross-check the constructed table:
        ObserverRelationalTable constructed_table(observer,true);
        if (!constructed_table.compare(readback_table)) {
            LOG_WARNING(QString(QObject::tr("Relational verification failed on observer: %1")).arg(observer->observerName()));
            complete = false;
        } else {
            LOG_INFO(QString(QObject::tr("Relational verification successful on observer: %1")).arg(observer->observerName()));
        }

        // Remove all relational properties used.
        ObserverRelationalTable::removeRelationalProperties(observer);
    }

    observer->endProcessingCycle();

    if (success) {
        if (complete) {
            LOG_DEBUG(QObject::tr("Binary import of observer ") + observer->observerName() + QObject::tr(" section was Successful (complete)."));
            return IExportable::Complete;
        } else {
            LOG_DEBUG(QObject::tr("Binary import of observer ") + observer->observerName() + QObject::tr(" section was Successful (incomplete."));
            return IExportable::Incomplete;
        }
    } else {
        LOG_WARNING(QObject::tr("Binary import of observer ") + observer->observerName() + QObject::tr(" section failed."));
        return IExportable::Failed;
    }
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::ObserverData::exportXmlExt_1_0(QDomDocument* doc, QDomElement* object_node, ExportItemFlags export_flags) const {
    object_node->setAttribute("ExportFlags",QString::number(export_flags));

    IExportable::Result result = IExportable::Complete;

    ObserverRelationalTable* relational_table = 0;
    if (export_flags & ExportRelationalData) {
        QDomElement relational_data = doc->createElement("RelationalData");
        object_node->appendChild(relational_data);
        // Export relational data about the observer:
        relational_table = new ObserverRelationalTable(observer,true);
        relational_table->setExportVersion(Qtilities::Qtilities_1_0);
        if (relational_table->exportXml(doc,&relational_data) != IExportable::Complete) {
            if (relational_table)
                delete relational_table;
            return IExportable::Failed;
        }
    }

    if (export_flags & ExportData) {
        // 1. The data of this item is added to a new data node:
        QDomElement subject_data = doc->createElement("Data");

        // Observer data:
        QDomElement observer_data = doc->createElement("ObserverData");
        // Add parameters as attributes:
        if (subject_limit != -1)
            observer_data.setAttribute("SubjectLimit",subject_limit);
        if (!observer_description.isEmpty())
            observer_data.setAttribute("Description",observer_description);
        if (access_mode != Observer::FullAccess)
            observer_data.setAttribute("AccessMode",Observer::accessModeToString((Observer::AccessMode) access_mode));        
        if (access_mode != Observer::GlobalScope)
            observer_data.setAttribute("AccessModeScope",Observer::accessModeScopeToString((Observer::AccessModeScope) access_mode_scope));
        if (object_deletion_policy != Observer::DeleteImmediately)
            observer_data.setAttribute("ObjectDeletionPolicy",Observer::objectDeletionPolicyToString((Observer::ObjectDeletionPolicy) object_deletion_policy));

        // Check there are any attributes under observer data:
        if (observer_data.attributes().count() > 0 || observer_data.childNodes().count() > 0)
            subject_data.appendChild(observer_data);

        // Visitor ID (only when needed)
        if (export_flags & ExportVisitorIDs) {
            int visitor_id = -1;
            if (ObjectManager::propertyExists(observer,qti_prop_VISITOR_ID)) {
                QVariant prop_variant = observer->property(qti_prop_VISITOR_ID);
                if (prop_variant.isValid() && prop_variant.canConvert<SharedProperty>()) {
                    SharedProperty prop = prop_variant.value<SharedProperty>();
                    if (prop.isValid()) {
                         visitor_id = prop.value().toInt();
                    }
                }
            }
            object_node->setAttribute("VisitorID",visitor_id);
        }

        // Categories:
        if (categories.count() > 0) {
            QDomElement categories_node = doc->createElement("Categories");
            object_node->appendChild(categories_node);
            for (int i = 0; i < categories.count(); i++) {
                QDomElement category = doc->createElement("Category");
                categories_node.appendChild(category);
                categories.at(i).exportXml(doc,&category);
            }
        }

        // Observer hints:
        if (display_hints) {
            if (display_hints->isExportable()) {
                QDomElement hints_data = doc->createElement("ObserverHints");
                display_hints->setExportVersion(exportVersion());
                if (display_hints->exportXml(doc,&hints_data) == IExportable::Failed) {
                    if (relational_table)
                        delete relational_table;
                    return IExportable::Failed;
                }
                if (hints_data.attributes().count() > 0 || hints_data.childNodes().count() > 0)
                    subject_data.appendChild(hints_data);
            }
        }

        // Subject filters:
        for (int i = 0; i < subject_filters.count(); i++) {
            if (subject_filters.at(i)->isExportable()) {
                QDomElement subject_filter = doc->createElement("SubjectFilter");
                subject_data.appendChild(subject_filter);
                if (subject_filters.at(i)->instanceFactoryInfo().exportXml(doc,&subject_filter,exportVersion()) == IExportable::Failed) {
                    if (relational_table)
                        delete relational_table;
                    return IExportable::Failed;
                }
                subject_filters.at(i)->setExportVersion(exportVersion());
                if (subject_filters.at(i)->exportXml(doc,&subject_filter) == IExportable::Failed) {
                    if (relational_table)
                        delete relational_table;
                    return IExportable::Failed;
                }
            }
        }

        // Formatting:
        IExportableFormatting* formatting_iface = qobject_cast<IExportableFormatting*> (objectBase());
        if (formatting_iface) {
            if (formatting_iface->exportFormattingXML(doc,&subject_data,exportVersion()) == IExportable::Failed) {
                if (relational_table)
                    delete relational_table;
                return IExportable::Failed;
            }
        }

        if (subject_data.attributes().count() > 0 || subject_data.childNodes().count() > 0)
            object_node->appendChild(subject_data);

        IExportable::Result result = IExportable::Complete;

        // Make List Of Exportable Subjects
        QList<IExportable*> exportable_list;
        bool complete = true;
        if (export_flags & ExportVisitorIDs)
            exportable_list = getLimitedExportsList(subject_list.toQList(),IExportable::XML,&complete);
        else {
            for (int l = 0; l < subject_list.count(); l++) {
                IExportable* iface = qobject_cast<IExportable*> (subject_list.at(l));
                if (iface)
                    exportable_list << iface;
            }
        }

        if (!complete)
            result = Incomplete;

        // Export exportable subjects:
        QDomElement subject_children = doc->createElement("Children");
        if (exportable_list.count() > 0)
            object_node->appendChild(subject_children);
        for (int i = 0; i < exportable_list.count(); i++) {
            Observer* obs = qobject_cast<Observer*> (exportable_list.at(i)->objectBase());
            IExportable* export_iface = exportable_list.at(i);
            if (export_iface) {
                if (export_iface->supportedFormats() & IExportable::XML) {
                    // Create a data item with its factory data as attributes for i:
                    // The item and its factory data:
                    QDomElement subject_item = doc->createElement("TreeItem");
                    subject_children.appendChild(subject_item);
                    // We also append the following information:
                    // 1. Category:
                    if (ObjectManager::propertyExists(export_iface->objectBase(),qti_prop_CATEGORY_MAP)) {
                        QVariant category_variant = observer->getMultiContextPropertyValue(export_iface->objectBase(),qti_prop_CATEGORY_MAP);
                        if (category_variant.isValid()) {
                            QtilitiesCategory category = category_variant.value<QtilitiesCategory>();
                            QDomElement category_item = doc->createElement("Category");
                            subject_item.appendChild(category_item);
                            category.setExportVersion(exportVersion());
                            category.exportXml(doc,&category_item);
                        }
                    }
                    // 2. Is Active:
                    if (ObjectManager::propertyExists(export_iface->objectBase(),qti_prop_ACTIVITY_MAP)) {
                        bool activity = observer->getMultiContextPropertyValue(export_iface->objectBase(),qti_prop_ACTIVITY_MAP).toBool();
                        if (activity)
                            subject_item.setAttribute("Activity","Active");
                        else
                            subject_item.setAttribute("Activity","Inactive");
                    }
                    // 3. Ownership:
                    Observer::ObjectOwnership ownership = observer->subjectOwnershipInContext(export_iface->objectBase());
                    if (ownership != Observer::ObserverScopeOwnership)
                        subject_item.setAttribute("Ownership",Observer::objectOwnershipToString(ownership));

                    // 4. Factory Data:
                    if (export_iface->instanceFactoryInfo().exportXml(doc,&subject_item,exportVersion()) == IExportable::Failed) {
                        if (relational_table)
                            delete relational_table;
                        return IExportable::Failed;
                    }

                    // 5. Visitor ID (only when needed)
                    if (export_flags & ExportVisitorIDs) {
                        int visitor_id = -1;
                        if (ObjectManager::propertyExists(export_iface->objectBase(),qti_prop_VISITOR_ID)) {
                            QVariant prop_variant = export_iface->objectBase()->property(qti_prop_VISITOR_ID);
                            if (prop_variant.isValid() && prop_variant.canConvert<SharedProperty>()) {
                                SharedProperty prop = prop_variant.value<SharedProperty>();
                                if (prop.isValid()) {
                                     visitor_id = prop.value().toInt();
                                }
                            }
                        }
                        subject_item.setAttribute("VisitorID",visitor_id);
                    }

                    // Now we let the export iface export whatever it need to export:
                    export_iface->setExportVersion(exportVersion());
                    export_iface->setApplicationExportVersion(applicationExportVersion());
                    IExportable::Result intermediate_result;
                    if (obs) {
                        ExportItemFlags child_obs_flags = export_flags;
                        child_obs_flags &= ~ExportRelationalData;
                        IExportableObserver* export_iface_obs = qobject_cast<IExportableObserver*> (obs->objectBase());
                        Q_ASSERT(export_iface_obs);

                        // Must create new IObserverExportable interface to handle this situation.
                        intermediate_result = export_iface_obs->exportXmlExt(doc,&subject_item,child_obs_flags);
                    } else
                        intermediate_result = export_iface->exportXml(doc,&subject_item);

                    if (intermediate_result == IExportable::Failed) {
                        if (relational_table)
                            delete relational_table;
                        return IExportable::Failed;
                        LOG_TRACE("TreeItem (" + export_iface->objectBase()->objectName() + ") failed.");
                    } else if (intermediate_result == IExportable::Incomplete) {
                        result = intermediate_result;
                        LOG_TRACE("TreeItem (" + export_iface->objectBase()->objectName() + ") is incomplete.");
                    } else if (intermediate_result == IExportable::Complete) {
                        LOG_TRACE("TreeItem (" + export_iface->objectBase()->objectName() + ") is complete.");
                    }
                } else {
                    LOG_WARNING(QObject::tr("XML export found an interface (") + observer->subjectNameInContext(export_iface->objectBase()) + QObject::tr(" in context ") + observer->observerName() + QObject::tr(") which does not support XML exporting. XML export will be incomplete."));
                    result = IExportable::Incomplete;
                }
            }
        }
    }

    if (relational_table)
        delete relational_table;

    return result;
}

Qtilities::Core::Interfaces::IExportable::Result Qtilities::Core::ObserverData::importXmlExt_1_0(QDomDocument* doc, QDomElement* object_node, QList<QPointer<QObject> >& import_list) {
    QList<QPointer<QObject> > active_subjects;
    observer->startProcessingCycle();
    IExportable::Result result = IExportable::Complete;

    ObserverRelationalTable* readback_table = 0;

    // Create a custom internal import list which will only store this observer and all its children:
    QList<QPointer<QObject> > internal_import_list;

    ExportItemFlags export_flags = ExportData;
    if (object_node->hasAttribute("ExportFlags"))
        export_flags = (ExportItemFlags) object_node->attribute("ExportFlags").toInt();

    if (export_flags & ExportVisitorIDs) {
        if (object_node->hasAttribute("VisitorID")) {
            SharedProperty visitor_id_prop(qti_prop_VISITOR_ID,object_node->attribute("VisitorID").toInt());
            ObjectManager::setSharedProperty(observer,visitor_id_prop);
        }
    }

    // All children underneath the root element gets constructed in here:
    QList<QObject*> constructed_list;
    QDomNodeList childNodes = object_node->childNodes();
    for(int i = 0; i < childNodes.count(); i++)
    {
        QDomNode childNode = childNodes.item(i);
        QDomElement child = childNode.toElement();

        if (child.isNull())
            continue;

        if (export_flags & ExportRelationalData) {
            if (child.tagName() == "RelationalData") {
                readback_table = new ObserverRelationalTable;
                QList<QPointer<QObject> > tmp_import_list;
                readback_table->importXml(doc,&child,tmp_import_list);
            }
        }

        if (export_flags & ExportData) {
            if (child.tagName() == "Data") {
                QDomNodeList dataNodes = child.childNodes();
                for(int i = 0; i < dataNodes.count(); i++)
                {
                    QDomNode dataChildNode = dataNodes.item(i);
                    QDomElement dataChild = dataChildNode.toElement();

                    if (dataChild.isNull())
                        continue;

                    if (dataChild.tagName() == "ObserverHints") {
                        observer->useDisplayHints();
                        display_hints->setExportVersion(exportVersion());
                        if (display_hints->importXml(doc,&dataChild,import_list) == IExportable::Failed)
                            return IExportable::Failed;
                        continue;
                    }

                    if (dataChild.tagName() == "ObserverData") {
                        if (dataChild.hasAttribute("SubjectLimit"))
                            subject_limit = dataChild.attribute("SubjectLimit").toInt();
                        if (dataChild.hasAttribute("Description"))
                            observer_description = dataChild.attribute("Description");
                        if (dataChild.hasAttribute("AccessMode"))
                            access_mode = Observer::stringToAccessMode(dataChild.attribute("AccessMode"));
                        if (dataChild.hasAttribute("AccessModeScope"))
                            access_mode_scope = Observer::stringToAccessModeScope(dataChild.attribute("AccessModeScope"));
                        if (dataChild.hasAttribute("ObjectDeletionPolicy"))
                            object_deletion_policy = Observer::stringToObjectDeletionPolicy(dataChild.attribute("ObjectDeletionPolicy"));

                        // Category stuff:
                        QDomNodeList childNodes = dataChild.childNodes();
                        for(int i = 0; i < childNodes.count(); i++)
                        {
                            QDomNode childNode = childNodes.item(i);
                            QDomElement child = childNode.toElement();

                            if (child.isNull())
                                continue;

                            if (child.tagName() == "Categories") {
                                QDomNodeList categoryNodes = child.childNodes();
                                for(int i = 0; i < categoryNodes.count(); i++)
                                {
                                    QDomNode categoryNode = categoryNodes.item(i);
                                    QDomElement category = categoryNode.toElement();

                                    if (category.isNull())
                                        continue;

                                    if (category.tagName() == "Categories") {
                                        QtilitiesCategory new_category;
                                        new_category.setExportVersion(exportVersion());
                                        new_category.importXml(doc,&category,import_list);
                                        if (new_category.isValid())
                                            categories << new_category;
                                        continue;
                                    }
                                }
                                continue;
                            }
                        }
                        continue;
                    }

                    if (dataChild.tagName() == "SubjectFilter") {
                        // Construct and init the subject filter:
                        InstanceFactoryInfo instanceFactoryInfo(doc,&dataChild,exportVersion());
                        if (instanceFactoryInfo.isValid()) {
                            LOG_TRACE(QString(QObject::tr("Importing subject type \"%1\" in factory \"%2\"...")).arg(instanceFactoryInfo.d_instance_tag).arg(instanceFactoryInfo.d_factory_tag));

                            IFactoryProvider* ifactory = OBJECT_MANAGER->referenceIFactoryProvider(instanceFactoryInfo.d_factory_tag);
                            if (ifactory) {
                                QObject* obj = ifactory->createInstance(instanceFactoryInfo);
                                if (obj) {
                                    obj->setObjectName(instanceFactoryInfo.d_instance_name);
                                    AbstractSubjectFilter* abstract_filter = qobject_cast<AbstractSubjectFilter*> (obj);
                                    if (abstract_filter) {
                                        abstract_filter->setExportVersion(exportVersion());
                                        if (abstract_filter->importXml(doc,&dataChild,import_list) == IExportable::Failed) {
                                            LOG_ERROR(QString(QObject::tr("Failed to import subject filter \"%1\" for tree node: \"%2\". Importing will not continue.")).arg(instanceFactoryInfo.d_instance_tag).arg(observer->observerName()));
                                            delete abstract_filter;
                                            result = IExportable::Failed;
                                        }
                                        if (!observer->installSubjectFilter(abstract_filter)) {
                                            LOG_DEBUG(QString(QObject::tr("Failed to install subject filter \"%1\" for tree node: \"%2\". If this filter already existed this is not a problem.")).arg(instanceFactoryInfo.d_instance_tag).arg(observer->observerName()));
                                            delete abstract_filter;
                                        }
                                    }
                                }
                            }
                        } else
                            LOG_WARNING(QString(QObject::tr("Found invalid factory data for subject filter on tree node: %1")).arg(observer->observerName()));
                        continue;
                    }

                    if (dataChild.tagName() == "Formatting") {
                        IExportableFormatting* formatting_iface = qobject_cast<IExportableFormatting*> (observer->objectBase());
                        if (formatting_iface) {
                            if (formatting_iface->importFormattingXML(doc,&dataChild,exportVersion()) != IExportable::Complete) {
                                LOG_WARNING(QString(QObject::tr("Failed to import formatting for tree node: \"%1\"")).arg(observer->observerName()));
                                result = IExportable::Incomplete;
                            }
                        }
                        continue;
                    }
                }
                continue;
            }

            if (child.tagName() == "Children") {
                QDomNodeList childrenNodes = child.childNodes();
                for(int i = 0; i < childrenNodes.count(); i++)
                {
                    QDomNode childrenChildNode = childrenNodes.item(i);
                    QDomElement childrenChild = childrenChildNode.toElement();

                    if (childrenChild.isNull())
                        continue;

                    if (childrenChild.tagName() == "TreeItem") {
                        // Construct and init the child:
                        InstanceFactoryInfo instanceFactoryInfo(doc,&childrenChild,exportVersion());
                        if (instanceFactoryInfo.isValid()) {
                            LOG_TRACE(QString(QObject::tr("Importing subject type \"%1\" in factory \"%2\"...")).arg(instanceFactoryInfo.d_instance_tag).arg(instanceFactoryInfo.d_factory_tag));

                            IFactoryProvider* ifactory = OBJECT_MANAGER->referenceIFactoryProvider(instanceFactoryInfo.d_factory_tag);
                            if (ifactory) {
                                QObject* obj = ifactory->createInstance(instanceFactoryInfo);
                                if (obj) {
                                    obj->setObjectName(instanceFactoryInfo.d_instance_name);
                                    internal_import_list << obj;
                                    IExportable* iface = qobject_cast<IExportable*> (obj);
                                    if (iface) {
                                        // Attach first before doing import on object:
                                        constructed_list << iface->objectBase();
                                        // Check if we must restore the ownership is active:
                                        Observer::ObjectOwnership ownership = Observer::ObserverScopeOwnership;
                                        if (childrenChild.hasAttribute("Ownership")) {
                                            ownership = Observer::stringToObjectOwnership(childrenChild.attribute("Ownership"));
                                        }
                                        if (observer->attachSubject(iface->objectBase(),ownership)) {
                                            import_list << obj;
                                        } else {
                                            LOG_WARNING(QString(QObject::tr("Failed to attach reconstructed object to tree node: %1. Import will be incomplete.")).arg(observer->observerName()));
                                            delete obj;
                                            result = IExportable::Incomplete;
                                            continue;
                                        }

                                        QDomNodeList subjectChildNodes = childrenChild.childNodes();
                                        for(int i = 0; i < subjectChildNodes.count(); i++)
                                        {
                                            QDomNode subjectChildNode = subjectChildNodes.item(i);
                                            QDomElement subjectChild = subjectChildNode.toElement();

                                            if (subjectChild.isNull())
                                                continue;

                                            if (subjectChild.tagName() == "Category") {
                                                // We just created this object, it will not have a category property yet so no need to check if it needs one:
                                                QtilitiesCategory category;
                                                category.setExportVersion(exportVersion());
                                                category.importXml(doc,&subjectChild,import_list);
                                                MultiContextProperty category_property(qti_prop_CATEGORY_MAP);
                                                category_property.setValue(qVariantFromValue(category),observer->observerID());
                                                if (!ObjectManager::setMultiContextProperty(iface->objectBase(),category_property))
                                                    result = IExportable::Incomplete;
                                            }
                                        }

                                        // Now that we created the item, init its data and children:
                                        iface->setExportVersion(exportVersion());
                                        iface->setApplicationExportVersion(applicationExportVersion());

                                        // Check if it is an observer: if so we must use internal_import_list, not import_list:
                                        Observer* obs = qobject_cast<Observer*> (iface->objectBase());
                                        IExportable::Result intermediate_result;
                                        if (obs)
                                            intermediate_result = iface->importXml(doc,&childrenChild,internal_import_list);
                                        else
                                            intermediate_result = iface->importXml(doc,&childrenChild,import_list);

                                        if (intermediate_result != IExportable::Complete) {
                                            LOG_WARNING(QString(QObject::tr("Failed to reconstruct object completely in tree node: %1. Item %2 will be incomplete.")).arg(observer->observerName()).arg(iface->objectBase()->objectName()));
                                            result = IExportable::Incomplete;
                                        }

                                        // Check if it is active:
                                        if (childrenChild.hasAttribute("Activity")) {
                                            if (childrenChild.attribute("Activity") == QString("Active"))
                                                active_subjects << iface->objectBase();
                                        }

                                        // Get VisitorID if needed:
                                        if (export_flags & ExportVisitorIDs) {
                                            if (childrenChild.hasAttribute("VisitorID")) {
                                                SharedProperty visitor_id_prop(qti_prop_VISITOR_ID,childrenChild.attribute("VisitorID").toInt());
                                                ObjectManager::setSharedProperty(iface->objectBase(),visitor_id_prop);
                                            }
                                        }
                                    } else {
                                        LOG_WARNING(QString(QObject::tr("Found invalid exportable interface on reconstructed object in tree node: %1")).arg(observer->observerName()));
                                        observer->endProcessingCycle();
                                        return IExportable::Failed;
                                    }
                                } else {
                                    LOG_WARNING(QString(QObject::tr("Factory tag %1 does not exist in factory %2. This item will be skipped and the import will be incomplete.")).arg(instanceFactoryInfo.d_instance_tag).arg(instanceFactoryInfo.d_factory_tag));
                                    result = IExportable::Incomplete;
                                }
                            } else {
                                LOG_WARNING(QString(QObject::tr("Factory with name %1 does not exist in the object manager. This item will be skipped and the import will be incomplete.")).arg(instanceFactoryInfo.d_factory_tag));
                                result = IExportable::Incomplete;
                            }
                        } else {
                            result = IExportable::Incomplete;
                            LOG_WARNING(QString(QObject::tr("Found invalid factory data for child on tree node: %1")).arg(observer->observerName()));
                        }
                        continue;
                    }
                }
                continue;
            }
        }
    }

    if (export_flags & ExportRelationalData) {
        internal_import_list << observer;

        // Construct relationships:
        if (!constructRelationships(internal_import_list,readback_table))
            result = IExportable::Incomplete;

        // Cross-check the constructed table:
        ObserverRelationalTable constructed_table(observer,true);
        /*if (verbose_output) {
            LOG_INFO(QString(tr("Relational verification completed on observer: %1. Here is the contents of the reconstructed table.")).arg(obs->observerName()));
            constructed_table.dumpTableInfo();
        }*/
        if (!constructed_table.compare(*readback_table)) {
            LOG_WARNING(QString(QObject::tr("Relational verification failed on observer: %1")).arg(observer->observerName()));
            result = IExportable::Incomplete;
        } else {
            LOG_INFO(QString(QObject::tr("Relational verification successful on observer: %1")).arg(observer->observerName()));
        }

        // Remove all relational properties used.
        ObserverRelationalTable::removeRelationalProperties(observer);
    }

    observer->endProcessingCycle();

    // If active_subjects has items in it we must set them active:
    if (active_subjects.count() > 0) {
        for (int i = 0; i < subject_filters.count(); i++) {
            ActivityPolicyFilter* activity_filter = qobject_cast<ActivityPolicyFilter*> (subject_filters.at(i));
            if (activity_filter) {
                activity_filter->setActiveSubjects(active_subjects,true);
                break;
            }
        }
    }

    return result;
}

bool Qtilities::Core::ObserverData::constructRelationships(QList<QPointer<QObject> >& objects, ObserverRelationalTable* table) const {
    if (!table)
        return false;

    // First check if all the objects in the pointer list are present in the table:
    if (!table->compareObjects(objects)) {
        LOG_ERROR(QString(QObject::tr("Relational table comparison failed. Relationship construction aborted.")));
        return false;
    } else
        LOG_TRACE("Table comparison successfull.");

    QList<Observer*> observer_list = Observer::observerList(objects);

    // Disable subject event filtering on all observers in list:
//    for (int i = 0; i < observer_list.count(); i++) {
//        observer_list.at(i)->toggleSubjectEventFiltering(false);
//    }

    // Fill in all the session ID fields with the current session information
    // and populate the previous session ID field.
    LOG_TRACE("Populating current session ID fields.");
    for (int i = 0; i < objects.count(); i++) {
        int visitor_id = ObserverRelationalTable::getVisitorID(objects.at(i));
        RelationalTableEntry* entry = table->entryWithVisitorID(visitor_id);
        Observer* obs = qobject_cast<Observer*> (objects.at(i));
        if (!obs) {
            // Check children of the object.
            foreach (QObject* child, objects.at(i)->children()) {
                obs = qobject_cast<Observer*> (child);
                if (obs)
                    break;
            }
        }

        if (obs) {
            LOG_DEBUG(QString("Doing session ID mapping on observer \"%1\": Previous ID: %2, Current ID: %3").arg(obs->observerName()).arg(entry->sessionID()).arg(obs->observerID()));
            entry->setPreviousSessionID(entry->sessionID());
            entry->setSessionID(obs->observerID());
        } else {
            entry->setPreviousSessionID(-1);
            entry->setSessionID(-1);
        }
    }

    // Correct the session IDs of all observer properties.
    // Binary exports of observer properties (not shared) stream the complete observer map of the property.
    // Here we need to correct the observer IDs (session IDs) for the current session and remove
    // contexts which was not part of the export.
//    LOG_TRACE("Correcting multi context properties' observer ID fields.");
//    for (int i = 0; i < objects.count(); i++) {
//        // Loop through all dynamic properties and get all the exportable observer properties:
//        int multi_context_property_count = 0;
//        QList<MultiContextProperty> multi_context_property_list;
//        QObject* obj = objects.at(i);
//        for (int p = 0; p < obj->dynamicPropertyNames().count(); p++) {
//            MultiContextProperty multi_context_property = ObjectManager::getMultiContextProperty(obj,obj->dynamicPropertyNames().at(p));
//            // Only exportable properties must be modified here:
//            if (multi_context_property.isValid() && multi_context_property.isExportable()) {
//                ++multi_context_property_count;
//                multi_context_property_list << multi_context_property;
//            }
//        }

//        // We need to map each observer ID in the observer map to the current session ID for that observer.
//        // We do this using the following steps:
//        // -) Loop through all properties.
//        // -) For each property, get each observer ID (previous session ID) in the observer map.
//        // -) Find the object with the previous session ID in the table.
//        // -) Get its current observer ID by casting it to an observer.
//        // -) Create a new property with current observer ID and values from current property.
//        // -) Lastly replace the property with the new property.
//        for (int p = 0; p < multi_context_property_count; p++) {
//            MultiContextProperty current_property = multi_context_property_list.at(p);
//            MultiContextProperty new_property(current_property.propertyName());
//            QMap<quint32, QVariant> local_map = current_property.contextMap();
//            for (int m = 0; m < local_map.count(); m++) {
//                int prev_session_id = (int) local_map.keys().at(m);
//                RelationalTableEntry* entry = table->entryWithPreviousSessionID(prev_session_id);
//                if (!entry) {
//                    LOG_ERROR(QString(QObject::tr("ObjectManager::constructRelationships() failed during observer property reconstruction. Failed to find relational table entry for previous session id: %1")).arg(prev_session_id));
//                    // If you get here on custom properties added by subject filters, make sure you handle the
//                    // import_cycle parameter correctly when initializing and finalizing attachments. What normally
//                    // happens is that the filter adds the property again with the current session ID. This check
//                    // will notice this because it looks at the new property, not the old correct property.
////                    for (int i = 0; i < observer_list.count(); i++) {
////                        observer_list.at(i)->toggleSubjectEventFiltering(true);
////                    }
//                    return false;
//                }

//                int current_session_id = entry->sessionID();
//                new_property.addContext(current_property.value(prev_session_id),current_session_id);
//            }
//            obj->setProperty(current_property.propertyName(),QVariant());
//            ObjectManager::setMultiContextProperty(obj,new_property);
//        }
//    }

    // Now construct the relationships.
    // We do this by taking the following steps:
    // 1. Go through the list and attach each item to all of its parents.
    //    If it is already attached to a parent, the attachment will just fail.
    //    We get the parent by looking it up in the object list. The lookup is performed by
    //    getting the visitor ID on each object in the list until we find a match.
    //    While going through the list we fill in the sessionID fields of each entry in the relational table.
    //    The sessionID is used again in step 3.
    // 2. Once the parents are sorted out, we need to sort out the object ownership.
    //    This is done by simply setting the OBSERVER_qti_prop_OWNERSHIP property on the object.
    //    If the ownership is SpecificObserverOwnership, we need to set the qti_prop_PARENT_ID property
    //    as well.
    // 3. Correct the names of each object in all the contexts to which it is attached.
    bool success = true;

    LOG_TRACE("Processing objects in construction list:");
    for (int i = 0; i < objects.count(); i++) {
        Q_ASSERT(objects.at(i));

        // Get the object Visitor ID property:
        int visitor_id = ObserverRelationalTable::getVisitorID(objects.at(i));
        LOG_TRACE(QString("Busy with object %1/%2: %3").arg(i+1).arg(objects.count()).arg(objects.at(i)->objectName()));

        // Now get this entry in the table:
        RelationalTableEntry* entry = table->entryWithVisitorID(visitor_id);
        if (!entry) {
            LOG_ERROR(QObject::tr("Observer relationship construction failed on object: ") + objects.at(i)->objectName() + QObject::tr(". An attempt will be made to continue with the rest of the relational table."));
            break;
        }

        // Now attach this subject to each parent using ManualOwnership:
        LOG_TRACE("> Attaching object to all needed contexts.");
        for (int e = 0; e < entry->parents().count(); e++) {
            // First get the actual session id (observer ID) for the parent:
            int session_id = table->entryWithVisitorID(entry->parents().at(e))->sessionID();
            Observer* obs = OBJECT_MANAGER->observerReference(session_id);
            if (obs) {
                // If it was already attached we skip this step.
                if (!obs->contains(objects.at(i))) {
                    obs->attachSubject(objects.at(i));
                    LOG_TRACE(">> Attaching object to context: " + obs->observerName());
                } else {
                    LOG_TRACE(">> Object already attached to context: " + obs->observerName());
                }
            } else {
                LOG_ERROR(QString(QObject::tr("Observer ID \"%1\" invalid on object: ")).arg(entry->parents().at(e)) + objects.at(i)->objectName() + QObject::tr(". An attempt will be made to continue with the rest of the relational table."));
                success = false;
            }
        }

        // Now set the ownership property on the object:
        LOG_TRACE("> Restoring correct ownership for object.");
        if ((Observer::ObjectOwnership) entry->ownership() == Observer::ManualOwnership) {
            SharedProperty ownership_property(qti_prop_OWNERSHIP,QVariant(Observer::ManualOwnership));
            ObjectManager::setSharedProperty(objects.at(i),ownership_property);
            SharedProperty observer_parent_property(qti_prop_PARENT_ID,QVariant(-1));
            ObjectManager::setSharedProperty(objects.at(i),observer_parent_property);
            LOG_TRACE(">> Restored object ownership is ManualOwnership.");
        } else if ((Observer::ObjectOwnership) entry->ownership() == Observer::ObserverScopeOwnership) {
            SharedProperty ownership_property(qti_prop_OWNERSHIP,QVariant(Observer::ObserverScopeOwnership));
            ObjectManager::setSharedProperty(objects.at(i),ownership_property);
            SharedProperty observer_parent_property(qti_prop_PARENT_ID,QVariant(-1));
            ObjectManager::setSharedProperty(objects.at(i),observer_parent_property);
            LOG_TRACE(">> Restored object ownership is ObserverScopeOwnership.");
        } else if ((Observer::ObjectOwnership) entry->ownership() == Observer::SpecificObserverOwnership) {
            // Get the session ID of the parent observer:
            RelationalTableEntry* parent_entry = table->entryWithVisitorID(entry->parentVisitorID());
            if (parent_entry) {
                int session_id = parent_entry->sessionID();
                SharedProperty ownership_property(qti_prop_OWNERSHIP,QVariant(Observer::SpecificObserverOwnership));
                ObjectManager::setSharedProperty(objects.at(i),ownership_property);
                SharedProperty observer_parent_property(qti_prop_PARENT_ID,QVariant(session_id));
                ObjectManager::setSharedProperty(objects.at(i),observer_parent_property);
                LOG_TRACE(">> Restored object ownership is SpecificObserverOwnership. Owner context ID: " + QString("%1").arg(session_id));
            } else {
                // This will happen when the object is the top level observer which was exported. In this
                // case we need to check if the object has any parents in the observer relational table entry.
                // If so we flag it as an error, else we know that it is not a problem:
                if (entry->parents().count() > 0) {
                    LOG_ERROR(QString(QObject::tr("Could not find parent with visitor ID (%1) to which object (%2) must be attached with SpecificObserverOwnership.")).arg(entry->parentVisitorID()).arg(objects.at(i)->objectName()));
                    success = false;
                }
            }
        } else if ((Observer::ObjectOwnership) entry->ownership() == Observer::OwnedBySubjectOwnership) {
            SharedProperty ownership_property(qti_prop_OWNERSHIP,QVariant(Observer::OwnedBySubjectOwnership));
            ObjectManager::setSharedProperty(objects.at(i),ownership_property);
            SharedProperty observer_parent_property(qti_prop_PARENT_ID,QVariant(-1));
            ObjectManager::setSharedProperty(objects.at(i),observer_parent_property);
            LOG_TRACE(">> Restored object ownership is OwnedBySubjectOwnership");
        } else {
            if (entry->parents().count() > 0)
                LOG_WARNING(QString(QObject::tr("Could not determine correct ownership for object: %1")).arg(objects.at(i)->objectName()));
        }

        LOG_TRACE("> Restoring instance names across contexts.");
        // Instance names should be correct after the observer IDs have been corrected.
        // Here we just need to correct the qti_prop_NAME_MANAGER_ID property and sync the object name with the qti_prop_NAME property.
        // 1. Correct qti_prop_NAME_MANAGER_ID:
        SharedProperty object_name_manager_id = ObjectManager::getSharedProperty(objects.at(i),qti_prop_NAME_MANAGER_ID);
        if (object_name_manager_id.isValid()) {
            int prev_session_id = object_name_manager_id.value().toInt();
            RelationalTableEntry* entry = table->entryWithPreviousSessionID(prev_session_id);
            if (entry) {
                // The previous name manager was part of this export:
                int current_session_id = entry->sessionID();
                SharedProperty new_name_manager_id(qti_prop_NAME_MANAGER_ID,current_session_id);
                ObjectManager::setSharedProperty(objects.at(i),new_name_manager_id);
                LOG_TRACE(">> qti_prop_NAME_MANAGER_ID:  Restored name manager successfuly.");
            } else {
                // The previous name manager was not part of this export:
                // Assign a new name manager.
                LOG_TRACE(">> qti_prop_NAME_MANAGER_ID: Name manager was not part of this export. Assigning a new name manager.");
                // Still todo.
            }

            // 2. Sync qti_prop_NAME:
            SharedProperty object_name = ObjectManager::getSharedProperty(objects.at(i),qti_prop_NAME);
            if (object_name.isValid()) {
                objects.at(i)->setObjectName(object_name.value().toString());
                LOG_TRACE(">> qti_prop_NAME_MANAGER_ID : Sync'ed object name with qti_prop_NAME property.");
            }
        } else {
            LOG_TRACE("> qti_prop_NAME_MANAGER_ID : Property not found, nothing to restore.");
        }
    }

    // Enable subject event filtering on all observers in the objects list,
    for (int i = 0; i < observer_list.count(); i++) {
        if (i == 0) {
            // Only one observer has to indicate that it's layout changed:
            observer_list.at(0)->refreshViewsLayout();
        }
        //observer_list.at(i)->toggleSubjectEventFiltering(true);
    }

    return success;
}

QList<IExportable*> Qtilities::Core::ObserverData::getLimitedExportsList(QList<QObject* > objects, IExportable::ExportMode export_mode, bool * incomplete) const {
    QList<IExportable*> exportable_list;
    qint32 iface_count = 0;
    if (incomplete)
        *incomplete = true;
    for (int i = 0; i < objects.count(); i++) {
        QObject* obj = objects.at(i);
        IExportable* iface = qobject_cast<IExportable*> (obj);
        // When doing containment, the containment class should reconstruct the contained observer: Otherwise
        // reconstruction will just skip the containment class and attach the contained observer directly
        // to the parent of the containment class.
        //
        // if (!iface) {
        //     foreach (QObject* child, obj->children()) {
        //     Observer* obs = qobject_cast<Observer*> (child);
        //     if (obs) {
        //         iface = obs;
        //         break;
        //     }
        // }

        if (iface) {
            if (!(iface->supportedFormats() & export_mode)) {
                LOG_WARNING(IExportable::exportModeToString(export_mode) + QObject::tr(" export found an interface on object (") + observer->subjectNameInContext(obj) + QObject::tr(" in context ") + observer->observerName() + QObject::tr(") which does not support this export type. Export will be incomplete."));
                if (incomplete)
                    *incomplete = false;
            } else if (!iface->instanceFactoryInfo().isValid()) {
                LOG_WARNING(IExportable::exportModeToString(export_mode) + QObject::tr(" export found an interface on object (") + observer->subjectNameInContext(obj) + QObject::tr(" in context ") + observer->observerName() + QObject::tr(") with invalid instanceFactoryInfo(). Export will be incomplete."));
                if (incomplete)
                    *incomplete = false;
            } else {
                // Handle limited export object, thus they should only be exported once.
                int count = ObjectManager::getSharedProperty(obj,qti_prop_LIMITED_EXPORTS).value().toInt();
                if (count == 0) {
                    SharedProperty limited_exports_prop(qti_prop_LIMITED_EXPORTS,count+1);
                    ObjectManager::setSharedProperty(obj,limited_exports_prop);
                    exportable_list << iface;
                    ++iface_count;
                } else {
                    LOG_TRACE(QString("%1/%2: Limited export object \"%3\" excluded in this context...").arg(i).arg(iface_count).arg(observer->subjectNameInContext(obj)));
                }
            }
        } else {
            LOG_WARNING(IExportable::exportModeToString(export_mode) + QObject::tr(" export found an interface on object (") + observer->subjectNameInContext(obj) + QObject::tr(" in context ") + observer->observerName() + QObject::tr(") which does not implement an exportable interface. Export will be incomplete."));
            if (incomplete)
                *incomplete = false;
        }
    }
    //qDebug() << "getLimitedExportsList() on " + observer->observerName() + ": input list count = " + QString::number(objects.count()) + ", exportable list count = " + QString::number(exportable_list.count());
    return exportable_list;
}
