#ifndef ROLES_H
#define ROLES_H

#include <qnamespace.h>

enum SequelJoeCellEditors {
    SJCellEditDefault = 0,
    SJCellEditLongText
};

enum SequelJoeCustomRoles {
    DatabaseIsFileRole = Qt::UserRole,
    DatabaseHasCipherRole,
    FilterColumnRole,
    FilterOperationRole,
    FilterValueRole,
    ForeignKeyRole,
    TableNameRole,
    ExpandedColumnIndexRole,
    HeightMultiple,
    EditorTypeRole
};


#endif // ROLES_H
