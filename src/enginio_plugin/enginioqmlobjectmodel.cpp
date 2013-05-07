/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://qt.digia.com/contact-us
**
** This file is part of the Enginio Qt Client Library.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "enginioqmlobjectmodel.h"

/*!
 * \qmltype ObjectModel
 * \instantiates EnginioQmlObjectModel
 * \inqmlmodule enginio-plugin
 * \brief List model for Enginio objects.
 *
 * ObjectModel can be used as container for Enginio objects on client side.
 *
 * Objects can be read from backend to the model with normal QueryOperation by
 * specifying operation's model property. When query operation finishes results
 * are added to the model.
 *
 * ObjectOperation also has model property. Depending on object operation's type
 * new objects are added to the model, modified objects are updated and deleted
 * objects are removed from the model.
 *
 * Example of creating new object in backend and adding it to model:
 * \code
 * Enginio.ObjectModel {
 *     id: cityModel
 * }
 *
 * Enginio.ObjectOperation {
 *     id: cityCreator
 *     client: client
 *     model: cityModel
 * }
 *
 * Component.onCompleted: {
 *     cityCreator.create({ objectType: "objects.city", name: "Berlin" });
 *     cityCreator.execute();
 *     // When operation finishes, new object is added to cityModel
 * }
 * \endcode
 *
 * ObjectModel can be used as a data source for QML's view elements (such as
 * ListView and GridView). In view delegate object data is accessed through
 * model's \c jsondata role.
 *
 * \code
 * ListView {
 *     id: cityList
 *     model: cityModel
 *     delegate: Text {
 *         text: jsondata.name
 *     }
 * }
 * \endcode
 */

/*!
 * \qmlproperty int ObjectModel::rowCount
 * Number of rows in the model.
 */

/*!
 * \qmlsignal ObjectModel::rowCountChanged(int newCount)
 * This signal is emitted when new rows are inserted or old rows are removed
 * from the model. \a newCount is the row count after insertion or removal.
 */

/*!
 * \qmlmethod int ObjectModel::rowFromId(string id)
 * Find object from model with ID \a id. Returns the row of found object or
 * negative number if object was not found.
 */

/*!
 * \qmlmethod object ObjectModel::objectFromRow(int row)
 * Returns object from model on row \a row or \c undefined if \a row is invalid.
 */

/*!
 * \qmlmethod bool ObjectModel::clear()
 * Removes all rows from model.
 */

EnginioQmlObjectModel::EnginioQmlObjectModel(QObject *parent) :
    EnginioObjectModel(parent)
{
}

QHash<int, QByteArray> EnginioQmlObjectModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[Enginio::JsonRole] = "jsondata";
    return names;
}

int EnginioQmlObjectModel::rowFromId(const QString &id) const
{
    QModelIndex index = indexFromId(id);
    if (index.isValid())
        return index.row();

    return -1;
}

QVariant EnginioQmlObjectModel::objectFromRow(int row) const
{
    return data(row, Enginio::JsonRole);
}