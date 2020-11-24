#include "core/ts_settings_qt.h"
#include "core/ts_logging_qt.h"

#include "plugin.h"

TSSettings *TSSettings::m_Instance = nullptr;

void TSSettings::Init(const QString &tsConfigPath)
{
    const auto name = QString(ts3plugin_name()).simplified().append("_SetDbConn");
    m_SettingsDb = QSqlDatabase::addDatabase("QSQLITE", name);
    m_SettingsDb.setDatabaseName(tsConfigPath + "settings.db");
    if (!QFile::exists(m_SettingsDb.database(name).databaseName()))
        TSLogging::Log(
        QString("Couldn't open settings.db: %1 does not exist.").arg(m_SettingsDb.database().databaseName()));

    if (!m_SettingsDb.isValid())
        TSLogging::Log("Database is not valid.");

    if (!m_SettingsDb.open())
        TSLogging::Error("Error loading settings.db; aborting init");
}

//! Find out which Sound Pack the user is currently using
/*!
 * \brief TSSettings::GetSoundPack
 * \param result the result will be put in here
 * \return true on success, false when an error has occurred
 */
auto TSSettings::GetSoundPack(QString &result) -> bool
{
    if (!(GetValueFromQuery("SELECT value FROM Notifications WHERE key='SoundPack'", result, false)))
    {
        error_qsql.setDriverText(error_qsql.driverText().prepend("(GetSoundPack) "));
        return false;
    }
    return true;
}

//! Find out which Icon Pack the user is currently using
/*!
 * \brief TSSettings::GetIconPack
 * \param result the result will be put in here
 * \return true on success, false when an error has occurred
 */
auto TSSettings::GetIconPack(QString &result) -> bool
{
    if (!(GetValueFromQuery("SELECT value FROM Application WHERE key='IconPack'", result, false)))
    {
        error_qsql.setDriverText(error_qsql.driverText().prepend("(GetIconPack) "));
        return false;
    }
    return true;
}

//! Get the default capture profile
/*!
 * \brief TSSettings::GetDefaultCaptureProfile
 * \param result the result will be put in here
 * \return true on success, false when an error has occurred
 */
auto TSSettings::GetDefaultCaptureProfile(QString &result) -> bool
{
    if (!(GetValueFromQuery("SELECT value FROM Profiles WHERE key='DefaultCaptureProfile'", result, false)))
    {
        error_qsql.setDriverText(error_qsql.driverText().prepend("(GetDefaultCaptureProfile) "));
        return false;
    }
    return true;
}

//! Get preprocessordata
/*!
 * \brief TSSettings::GetPreProcessorData
 * \param profile
 * \param result the result will be put in here
 * \return true on success, false when an error has occurred
 */
auto TSSettings::GetPreProcessorData(const QString &profile, QString &result) -> bool
{
    QString query("SELECT value FROM Profiles WHERE key='Capture/" + profile + "/PreProcessing'");
    if (!(GetValueFromQuery(query, result, false)))
    {
        error_qsql.setDriverText(error_qsql.driverText().prepend("(GetPreProcessorData) "));
        return false;
    }
    return true;
}

//! Get all bookmarks
/*!
 * \brief TSSettings::GetBookmarks
 * \param result the result will be put in here
 * \return true on success, false when an error has occurred
 */
auto TSSettings::GetBookmarks(QStringList &result) -> bool
{
    QString query("SELECT value FROM Bookmarks");
    if (!(GetValuesFromQuery(query, result)))
    {
        error_qsql.setDriverText(error_qsql.driverText().prepend("(GetBookmarks) "));
        return false;
    }
    return true;
}

//! Find a bookmark by server uid
/*!
 * \brief TSSettings::GetBookmarkByServerUID Find a bookmark by server uid
 * \param sUID the server uid
 * \param result a QMap
 * \return true on success, false when an error has occurred; note that not found is a non-error -> success;
 * just check if the QMap is empty for that
 */
auto TSSettings::GetBookmarkByServerUID(QString sUID, QMap<QString, QString> &result) -> bool
{
    QStringList bookmarks;
    if (!GetBookmarks(bookmarks))
        return false;

    sUID.prepend("ServerUID=");
    for (int i = 0; i < bookmarks.size(); ++i)
    {
        const auto &bookmark = bookmarks.at(i);
        if (bookmark.contains(sUID))
        {
            result = GetMapFromValue(bookmark);
            break;
        }
    }
    return true;
}

//! Get all contacts
/*!
 * \brief TSSettings::GetContacts
 * \param result the result will be put in here
 * \return true on success, false when an error has occurred
 */
auto TSSettings::GetContacts(QStringList &result) -> bool
{
    QString query("SELECT value FROM Contacts");
    if (!(GetValuesFromQuery(query, result)))
    {
        error_qsql.setDriverText(error_qsql.driverText().prepend("(GetContacts) "));
        return false;
    }
    return true;
}

/*auto TSSettings::GetContactFromUniqueId(const char* clientUniqueID, QMap<QString,QString> result) -> bool
{
    QStringList contacts;
    if (!(GetContacts(contacts)))
    {
        return false;
    }
    //TODO
    return true;
}*/

//! Returns the language the client will be trying to use
/*!
 * \brief TSSettings::GetLanguage Query the TS database for the user language selection; on system default,
 * fallback to OS language \param result the result will be put in here \return true on success, false when an
 * error has occurred
 */
auto TSSettings::GetLanguage(QString &result) -> bool
{
    QString query("SELECT value FROM Application WHERE key='Language'");  //"","enUS","deDE"...
    if (!(GetValueFromQuery(query, result, true)))
    {
        error_qsql.setDriverText(error_qsql.driverText().prepend("(GetLanguage) "));
        result = QLocale::system().name();
        return false;
    }
    if (result.isEmpty())
        result = QLocale::system().name();

    return true;
}

auto TSSettings::Is3DSoundEnabled(bool &result) -> bool
{
    QString qstr_result;
    QString query("SELECT value FROM Application WHERE key='3DSoundEnabled'");
    if (!(GetValueFromQuery(query, qstr_result, false)))
    {
        error_qsql.setDriverText(error_qsql.driverText().prepend("(Is3DSoundEnabled) "));
        return false;
    }
    result = (qstr_result == "1" || qstr_result == "true");
    return true;
}

auto TSSettings::Set3DSoundEnabled(bool val) -> bool
{
    QSqlQuery q_query(
    QString("UPDATE Application SET value='%1' WHERE key='3DSoundEnabled'").arg((val) ? "1" : "0"),
    m_SettingsDb);
    if (!q_query.exec())
    {
        auto sql_error = q_query.lastError();
        if (sql_error.isValid())
            error_qsql = sql_error;
        else
            SetError("Unknown error on query.exec.");

        return false;
    }
    return true;
}

//! Returns the last SQL Error; optional usage when a TSSettings function doesn't return true
/*!
 * \brief TSSettings::GetLastError Returns the last SQL Error
 * \return a QSqlError
 */
auto TSSettings::GetLastError() -> QSqlError
{
    return error_qsql;
}

// Private

//! Convert the value QString to a QMap
/*!
 * \brief TSSettings::GetMapFromValue Convert the value QString to a QMap
 * \param value the value
 * \return a QMap
 */
auto TSSettings::GetMapFromValue(const QString &value) -> QMap<QString, QString>
{
    QMap<QString, QString> result;
    auto qstrl_value = value.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
    for (int i = 0; i < qstrl_value.count(); ++i)
    {
        const auto &qs_val = qstrl_value.at(i);
        result.insert(qs_val.section("=", 0, 0), qs_val.section("=", 1));
    }
    return result;
}

//! Get a single value from the TS Database
/*!
 * \brief TSSettings::GetValueFromQuery Get a single value from the TS Database
 * \param query an sql query string
 * \param result the result will be put in here
 * \param isEmptyValid determines, if an empty result is considered an error
 * \return true on success, false when an error has occurred
 */
auto TSSettings::GetValueFromQuery(const QString &query,
                                   QString &result,
                                   bool isEmptyValid) -> bool  // provides first valid
{
    QSqlQuery q_query(query, m_SettingsDb);
    if (!q_query.exec())
    {
        QSqlError sql_error = q_query.lastError();
        if (sql_error.isValid())
        {
            error_qsql = sql_error;
            error_qsql.setDriverText(error_qsql.driverText().prepend("(q_query.exec()) "));
        }
        else
        {
            SetError("Unknown error on query.exec.");
        }
        return false;
    }
    else
    {
        if (!(q_query.isSelect()))
        {
            auto sql_error = q_query.lastError();
            if (sql_error.isValid())
            {
                error_qsql = sql_error;
                error_qsql.setDriverText(error_qsql.driverText().prepend("(q_query.isSelect()) "));
            }
            else
            {
                SetError("Unknown error on query.isSelect().");
            }
            return false;
        }
        if (!(q_query.isActive()))
        {
            auto sql_error = q_query.lastError();
            if (sql_error.isValid())
            {
                error_qsql = sql_error;
                error_qsql.setDriverText(error_qsql.driverText().prepend("(q_query.isActive()) "));
            }
            else
            {
                SetError("Unknown error on query.isActive().");
            }
            return false;
        }

        auto found = false;
        while (q_query.next())
        {
            if (!(q_query.isValid()))
            {
                auto sql_error = q_query.lastError();
                if (sql_error.isValid())
                {
                    error_qsql = sql_error;
                    error_qsql.setDriverText(error_qsql.driverText().prepend("(q_query.isValid()) "));
                }
                else
                {
                    SetError("Unknown error on query.isValid().");
                }
                return false;
            }
            QString qstr(q_query.value(0).toString());
            if (!qstr.isEmpty())
            {
                result = qstr;
                found = true;
                break;
            }
        }
        if (!found)
        {
            if (isEmptyValid)
            {
                result = "";
                found = true;
            }
            else
                SetError("Unknown error.");
        }

        return found;
    }
}

//! Get multiple values from the TS Database
/*!
 * \brief TSSettings::GetValuesFromQuery
 * \param query a sql query string
 * \param result the result will be put in here
 * \return true on success, false when an error has occurred
 */
auto TSSettings::GetValuesFromQuery(const QString &query, QStringList &result) -> bool  // proper result list
{
    QSqlQuery q_query(query, m_SettingsDb);
    if (!q_query.exec())
    {
        auto sql_error = q_query.lastError();
        if (sql_error.isValid())
            error_qsql = sql_error;
        else
            SetError("Unknown error on query.exec.");

        return false;
    }
    else
    {
        if (!(q_query.isSelect()))
        {
            QSqlError sql_error = q_query.lastError();
            if (sql_error.isValid())
                error_qsql = sql_error;
            else
                SetError("Unknown error on query.isSelect().");

            return false;
        }
        if (!(q_query.isActive()))
        {
            QSqlError sql_error = q_query.lastError();
            if (sql_error.isValid())
                error_qsql = sql_error;
            else
                SetError("Unknown error on query.isActive().");

            return false;
        }

        while (q_query.next())
        {
            if (!(q_query.isValid()))
            {
                QSqlError sql_error = q_query.lastError();
                if (sql_error.isValid())
                    error_qsql = sql_error;
                else
                    SetError("Unknown error on query.isValid().");

                return false;
            }
            QString qstr(q_query.value(0).toString());
            if (!qstr.isEmpty())
                result.append(qstr);
        }
        return true;
    }
}

//! Manually creates a custom SQL Error
/*!
 * \brief TSSettings::SetError encapsulates an error string in a sql error
 * \param in the QString to put into the errors DriverText
 */
void TSSettings::SetError(const QString &in)
{
    QSqlError sql_error;
    // sql_error.setType(-1); //Cannot manually set to "Cannot be determined"(-1) since out of ErrorType enum,
    // hopefully it's the default QSqlError
    sql_error.setNumber(-1);
    sql_error.setDatabaseText("Unknown Custom Error");
    sql_error.setDriverText(in);
    error_qsql = sql_error;
}
