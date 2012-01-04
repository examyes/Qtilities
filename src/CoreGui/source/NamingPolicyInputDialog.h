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

#ifndef NAMINGPOLICYINPUTDIALOG_H
#define NAMINGPOLICYINPUTDIALOG_H

#include "NamingPolicyFilter.h"
#include "INamingPolicyDialog.h"

#include <QDialog>
#include <QIcon>
#include <QString>

namespace Ui {
    class NamingPolicyInputDialog;
}

namespace Qtilities {
    namespace CoreGui {
        using namespace Interfaces;

        /*!
          \class NamingPolicyInputDialog
          \brief The NamingPolicyInputDialog class provides a dialog which allows the user to select how to resolve a naming conflict.

          The NamingPolicyInputDialog is used when a naming policy filter detects a conflict and its resolution policy
          is set to PromptUser. The dialog presents the user with possible options on how to resolve the conflict.

          Below is an example of the dialog in action.

          \image html naming_policy_input_dialog.jpg "Naming Policy Input Dialog"
          \image latex naming_policy_input_dialog.eps "Naming Policy Input Dialog" width=5in
          \sa NamingPolicyFilter
         */
        class NamingPolicyInputDialog : public INamingPolicyDialog {
            Q_OBJECT

            friend class NamingPolicyFilter;

        public:
            NamingPolicyInputDialog(QWidget *parent = 0);
            ~NamingPolicyInputDialog();

            void accept();
            void reject();

            // --------------------------------
            // IObjectBase Implementation
            // --------------------------------
            QObject* objectBase() { return this; }
            const QObject* objectBase() const { return this; }

            // --------------------------------
            // INamingPolicyDialog Implementation
            // --------------------------------
            bool useCycleResolution() const;
            void endValidationCycle();
            QString autoGeneratedName() const;
            NamingPolicyFilter::ResolutionPolicy selectedResolution() const;
            void setNamingPolicyFilter(NamingPolicyFilter* naming_policy_subject_filter) { subject_filter = naming_policy_subject_filter; }
            void setObject(QObject* conflicting_obj);
            void setContext(int context_id, const QString& context_name, const QIcon& window_icon = QIcon());
            bool initialize(NamingPolicyFilter::NameValidity validity_result);

        public slots:
            void handleGeneratedNewNameButton();
            void handleDifferentNameChange(QString new_text);
            void updateStatusMessage();

        protected:
            void changeEvent(QEvent *e);

        private:
            QString getName();

            NamingPolicyFilter* subject_filter;
            QString observer_context;
            int observer_id;
            QObject* object;
            QObject* conflicting_object;
            Ui::NamingPolicyInputDialog *ui;
        };
    }
}

#endif // NAMINGPOLICYINPUTDIALOG_H
