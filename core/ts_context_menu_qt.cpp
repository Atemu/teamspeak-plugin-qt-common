#include "core/ts_context_menu_qt.h"

#include "core/plugin_base.h"
#include "core/ts_logging_qt.h"

#include "plugin_definitions.h"

#include <QtCore/QPointer>

#include <cassert>

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon)
{
    struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
    menuItem->type = type;
    menuItem->id = id;
    qstrncpy(menuItem->text, text, PLUGIN_MENU_BUFSZ);
    qstrncpy(menuItem->icon, icon, PLUGIN_MENU_BUFSZ);
    return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
//#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define CREATE_MENU_ITEM(item) (*menuItems)[n++] = item;
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

TSContextMenu::TSContextMenu(Plugin_Base* plugin)
	: QObject(plugin)
{
}

bool TSContextMenu::setMainIcon(QString icon)
{
    if (m_isInit)
        return false;

    m_MainIcon = icon;
    return true;
}

int TSContextMenu::Register(QObject* p, PluginMenuType type, QString text, QString icon)
{
    Q_UNUSED(p);

    if (m_isInit)
        return -1;

	auto plugin = qobject_cast<Plugin_Base*>(parent());
	auto& translator = plugin->translator();
    auto trans = translator.translate("context_menu", text.toLatin1().constData());

    auto id = m_Items.size();
    m_Items.append(createMenuItem(type, id, (trans.isEmpty()?text:trans).toLatin1().constData(), icon.toLatin1().constData()));

    return id;
}

void TSContextMenu::onInitMenus(PluginMenuItem*** menuItems, char **menuIcon)
{
    m_isInit=true;
    /*
     * Create the menus
     * There are three types of menu items:
     * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
     * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
     * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
     *
     * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
     *
     * The menu text is required, max length is 128 characters
     *
     * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
     * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
     * plugin filename, without dll/so/dylib suffix
     * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
     */
//    TSLogging::Log(QString("(TSContextMenu::onInitMenu): %1").arg(m_Items.size()),LogLevel_DEBUG);
    auto amount = m_Items.size();
    BEGIN_CREATE_MENUS(amount);  /* IMPORTANT: Number of menu items must be correct! */
    for (int i = 0; i < amount; ++i)
        CREATE_MENU_ITEM(m_Items.takeFirst());

    END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */
    /*
     * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
     * If unused, set menuIcon to NULL
     */
    if (!m_MainIcon.isEmpty())
    {
        *menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
        qstrncpy(*menuIcon, m_MainIcon.toLatin1().constData(), PLUGIN_MENU_BUFSZ);
    }
    emit MenusInitialized();
}


void TSContextMenu::onMenuItemEvent(uint64 serverConnectionHandlerID, PluginMenuType type, int menuItemID, uint64 selectedItemID)
{
    emit FireContextMenuEvent(serverConnectionHandlerID, type, menuItemID, selectedItemID);
}
