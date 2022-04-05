#include "nm_class.h"

// /* copied from libnm-core/nm-utils.c */
// char *
// nm_utils_uuid_generate(void)
// {
//     uuid_t uuid;
//     char * buf;

//     buf = g_malloc0(37);
//     uuid_generate_random(uuid);
//     uuid_unparse_lower(uuid, &buf[0]);
//     return buf;
// }

NewtworkManager::NewtworkManager() {
    num_device = 0;
    read_device_paths();
    read_device_types();
    read_wireless_accesspoints();
    read_saved_connections();
    read_active_connection();
}

void NewtworkManager::read_device_paths() {
    GDBusProxy *props_proxy;
    GError *  error = NULL;
    GVariant *ret = NULL, *value = NULL;
    char ** paths = NULL;

    /* Create a D-Bus proxy to get the object properties from the NM Manager
    * object.  NM_DBUS_* defines are from nm-dbus-interface.h.
    */
    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL,
                                                NM_DBUS_SERVICE,
                                                NM_DBUS_PATH,
                                                "org.freedesktop.DBus.Properties",
                                                NULL,
                                                NULL);
    g_assert(props_proxy);

    /* Get the ActiveConnections property from the NM Manager object */
    ret = g_dbus_proxy_call_sync(props_proxy,
                                "Get",
                                g_variant_new("(ss)", NM_DBUS_INTERFACE, "AllDevices"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);

    if (!ret) {
        g_dbus_error_strip_remote_error(error);
        g_warning("Failed to get AllDevices property: %s\n", error->message);
        g_error_free(error);
        return;
    }

    g_variant_get(ret, "(v)", &value);

    /* Make sure the ActiveConnections property is the type we expect it to be */
    if (!g_variant_is_of_type(value, G_VARIANT_TYPE("ao"))) {
        g_warning("Unexpected type returned getting AllDevices: %s",
                g_variant_get_type_string(value));
        if (value)
            g_variant_unref(value);
        if (ret)
            g_variant_unref(ret);
    }

    /* Extract the active connections array from the GValue */
    paths = g_variant_dup_objv(value, NULL);

    if (!paths) {
        g_warning("Could not retrieve active connections property");
        if (value)
            g_variant_unref(value);
        if (ret)
            g_variant_unref(ret);
    }

    for (int i = 0; paths[i]; i++) {
        device_paths.push_back(paths[i]);
        num_device++;
    }

    g_strfreev(paths);
}

void NewtworkManager::read_device_types() {
    GDBusProxy *props_proxy;
    GVariant *  ret = NULL, * path_value = NULL;
    GError *    error = NULL;
    int dev_type_int;

    for (int i = 0; i < num_device; i++) {
        int n = device_paths[i].length();
        char char_array[n + 1];
        strcpy(char_array, device_paths[i].c_str());

        props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                    G_DBUS_PROXY_FLAGS_NONE,
                                    NULL,
                                    NM_DBUS_SERVICE,
                                    char_array,
                                    "org.freedesktop.DBus.Properties",
                                    NULL,
                                    NULL);

        g_assert(props_proxy);
        
        ret = g_dbus_proxy_call_sync(props_proxy,
                                    "Get",
                                    g_variant_new("(ss)", NM_DBUS_INTERFACE_DEVICE, "DeviceType"),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

        g_variant_get(ret, "(v)", &path_value);
        dev_type_int = g_variant_get_uint32(path_value);
        device_types.push_back(DEV_TYPE_NAME[dev_type_int]);

        g_variant_unref(ret);
    }
}

void NewtworkManager::read_access_point_detail(const char *path, char *ssid_name)
{
    GDBusProxy * proxy;
    GError *     error = NULL;
    GVariant *   ret;
    GVariant * path_value;
    // const char * id, *type;
    // gboolean     found;
    // GVariantIter iter;
    // const char * setting_name;
    GVariantIter iter;
    char paths;
    /* This function asks NetworkManager for the details of the connection */

    /* Create the D-Bus proxy so we can ask it for the connection configuration details. */
    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        NULL,
                                        NM_DBUS_SERVICE,
                                        path,
                                        "org.freedesktop.DBus.Properties",
                                        NULL,
                                        NULL);
    g_assert(proxy);

    /* Request the all the configuration of the Connection */
    ret = g_dbus_proxy_call_sync(proxy,
                                "Get",
                                g_variant_new("(ss)", NM_DBUS_INTERFACE_ACCESS_POINT, "Ssid"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (!ret) {
        g_dbus_error_strip_remote_error(error);
        g_warning("Failed to get Ssid: %s\n", error->message);
        g_error_free(error);
        return;
    }

    g_variant_get(ret, "(v)", &path_value);

    g_variant_iter_init(&iter, path_value);

    int len = 0;
    while (g_variant_iter_next(&iter, "y", &paths)) {
        ssid_name[len++] = paths;
        g_print("%c", paths);
    }
    ssid_name[len] = '\0';
    g_print("\n");

    if (path_value)
        g_variant_unref(path_value);
    if (ret)
        g_variant_unref(ret);
}

void NewtworkManager::read_wireless_accesspoints() {
    GDBusProxy *props_proxy;
    GVariant *  ret = NULL;
    GError *    error = NULL;

    int       i;
    char **   paths;

    // Get wireless device path
    for (int i = 0; i < device_paths.size(); i++) {
        if (device_types[i].compare(DEV_TYPE_NAME[2]) == 0)
            device_path_num = i;
    }

    int n = device_paths[device_path_num].length();
    char dev_path[n + 1];
    strcpy(dev_path, device_paths[device_path_num].c_str());

    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL,
                                            NM_DBUS_SERVICE,
                                            dev_path,
                                            NM_DBUS_INTERFACE_DEVICE_WIRELESS,
                                            NULL,
                                            NULL);
    g_assert(props_proxy);

    ret = g_dbus_proxy_call_sync(props_proxy,
                                "GetAllAccessPoints",
                                NULL,
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    
    if (!ret) {
        g_dbus_error_strip_remote_error(error);
        g_print("ListConnections failed: %s\n", error->message);
        g_error_free(error);
        return;
    }

    g_variant_get(ret, "(^ao)", &paths);
    g_variant_unref(ret);

    for (i = 0; paths[i]; i++) {
        wireless_struct w_obj;
        char ssid[225];
        read_access_point_detail(paths[i], ssid);
        w_obj.obj_dir = paths[i];
        w_obj.ssid = ssid;
        access_points.push_back(w_obj);
    }
}

void NewtworkManager::saved_wireless(const int num_ap, const std::string password) {
    GVariantBuilder connection_builder;
    GVariantBuilder setting_builder;
    char *          uuid;
    const char *    new_con_path;
    GVariant *      ret, *ret2, *value;
    GError *        error = NULL;
    GDBusProxy *proxy, *proxy2;

    int n = (access_points[num_ap].obj_dir).length();
    char dev_path[n + 1];
    strcpy(dev_path, (access_points[num_ap].obj_dir).c_str());

    n = (access_points[num_ap].ssid).length();
    char ssid_char[n + 1];
    strcpy(ssid_char, (access_points[num_ap].ssid).c_str());

    n = password.length();
    char password_char[n + 1];
    strcpy(password_char, password.c_str());

    proxy2 = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        NULL,
                                        NM_DBUS_SERVICE,
                                        dev_path,
                                        "org.freedesktop.DBus.Properties",
                                        NULL,
                                        NULL);

    g_assert(proxy2);

    /* Request the all the configuration of the Connection */
    ret2 = g_dbus_proxy_call_sync(proxy2,
                                "Get",
                                g_variant_new("(ss)", NM_DBUS_INTERFACE_ACCESS_POINT, "Ssid"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (!ret2) {
        g_dbus_error_strip_remote_error(error);
        g_warning("Failed to get Ssid: %s\n", error->message);
        g_error_free(error);
        return;
    }

    g_variant_get(ret2, "(v)", &value);

    /* Create a D-Bus proxy; NM_DBUS_* defined in nm-dbus-interface.h */
    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        NULL,
                                        NM_DBUS_SERVICE,
                                        NM_DBUS_PATH_SETTINGS,
                                        NM_DBUS_INTERFACE_SETTINGS,
                                        NULL,
                                        &error);
    if (!proxy) {
        g_dbus_error_strip_remote_error(error);
        g_print("Could not create NewtworkManager D-Bus proxy: %s\n", error->message);
        g_error_free(error);
        return;
    }

    /* Initialize connection GVariantBuilder */
    g_variant_builder_init(&connection_builder, G_VARIANT_TYPE("a{sa{sv}}"));

    /* Build up the 'connection' Setting */
    g_variant_builder_init(&setting_builder, G_VARIANT_TYPE("a{sv}"));

    uuid = nm_utils_uuid_generate();
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_CONNECTION_UUID,
                        g_variant_new_string(uuid));
    g_free(uuid);

    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_CONNECTION_ID,
                        g_variant_new_string(ssid_char));

    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_CONNECTION_TYPE,
                        g_variant_new_string(NM_SETTING_WIRELESS_SETTING_NAME));

    g_variant_builder_add(&connection_builder,
                        "{sa{sv}}",
                        NM_SETTING_CONNECTION_SETTING_NAME,
                        &setting_builder);

    /* Add the (empty) 'wired' Setting */
    g_variant_builder_init(&setting_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_WIRELESS_SSID,
                        value);

    g_variant_builder_add(&connection_builder,
                        "{sa{sv}}",
                        NM_SETTING_WIRELESS_SETTING_NAME,
                        &setting_builder);

    /* Build up the 'Password' Setting */
    g_variant_builder_init(&setting_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_WIRELESS_SECURITY_KEY_MGMT,
                        g_variant_new_string("wpa-psk"));
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_WIRELESS_SECURITY_PSK,
                        g_variant_new_string(password_char));

    g_variant_builder_add(&connection_builder,
                        "{sa{sv}}",
                        NM_SETTING_WIRELESS_SECURITY_SETTING_NAME,
                        &setting_builder);

    /* Build up the 'ipv4' Setting */
    g_variant_builder_init(&setting_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_IP_CONFIG_METHOD,
                        g_variant_new_string(NM_SETTING_IP4_CONFIG_METHOD_AUTO));
    g_variant_builder_add(&connection_builder,
                        "{sa{sv}}",
                        NM_SETTING_IP4_CONFIG_SETTING_NAME,
                        &setting_builder);

    /* Call AddConnection with the connection dictionary as argument.
    * (g_variant_new() will consume the floating GVariant returned from
    * &connection_builder, and g_dbus_proxy_call_sync() will consume the
    * floating variant returned from g_variant_new(), so no cleanup is needed.
    */
    ret = g_dbus_proxy_call_sync(proxy,
                                "AddConnection",
                                g_variant_new("(a{sa{sv}})", &connection_builder),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (ret) {
        g_variant_get(ret, "(&o)", &new_con_path);
        g_print("Added: %s\n", new_con_path);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    if (proxy) {
        g_object_unref(proxy);
    }
    if (proxy2) {
        g_object_unref(proxy2);
    }
    if (value) {
        g_variant_unref(value);
    }
}

void NewtworkManager::connect_wireless(const int num_ap, const std::string password) {
    GVariantBuilder connection_builder;
    GVariantBuilder setting_builder;
    char *          uuid;
    const char *    new_con_path;
    GVariant *      ret, *ret2, *value;
    GError *        error = NULL;
    GDBusProxy *proxy, *proxy2;

    int n = (access_points[num_ap].obj_dir).length();
    char dev_path[n + 1];
    strcpy(dev_path, (access_points[num_ap].obj_dir).c_str());
    g_print("%s\n", dev_path);

    n = (access_points[num_ap].ssid).length();
    char ssid_char[n + 1];
    strcpy(ssid_char, (access_points[num_ap].ssid).c_str());
    g_print("%s\n", ssid_char);

    n = device_paths[device_path_num].length();
    char dev_obj_path[n + 1];
    strcpy(dev_obj_path, (device_paths[device_path_num].c_str()));
    g_print("%s\n", dev_obj_path);

    n = password.length();
    char password_char[n + 1];
    strcpy(password_char, password.c_str());
    g_print("%s\n", password_char);

    proxy2 = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        NULL,
                                        NM_DBUS_SERVICE,
                                        dev_path,
                                        "org.freedesktop.DBus.Properties",
                                        NULL,
                                        NULL);

    g_assert(proxy2);

    /* Request the all the configuration of the Connection */
    ret2 = g_dbus_proxy_call_sync(proxy2,
                                "Get",
                                g_variant_new("(ss)", NM_DBUS_INTERFACE_ACCESS_POINT, "Ssid"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (!ret2) {
        g_dbus_error_strip_remote_error(error);
        g_warning("Failed to get Ssid: %s\n", error->message);
        g_error_free(error);
        return;
    }

    g_variant_get(ret2, "(v)", &value);

    /* Create a D-Bus proxy; NM_DBUS_* defined in nm-dbus-interface.h */
    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        NULL,
                                        NM_DBUS_SERVICE,
                                        NM_DBUS_PATH,
                                        NM_DBUS_INTERFACE,
                                        NULL,
                                        &error);
    if (!proxy) {
        g_dbus_error_strip_remote_error(error);
        g_print("Could not create NewtworkManager D-Bus proxy: %s\n", error->message);
        g_error_free(error);
        return;
    }

    /* Initialize connection GVariantBuilder */
    g_variant_builder_init(&connection_builder, G_VARIANT_TYPE("a{sa{sv}}"));

    /* Build up the 'connection' Setting */
    g_variant_builder_init(&setting_builder, G_VARIANT_TYPE("a{sv}"));

    uuid = nm_utils_uuid_generate();
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_CONNECTION_UUID,
                        g_variant_new_string(uuid));
    g_free(uuid);

    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_CONNECTION_ID,
                        g_variant_new_string(ssid_char));

    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_CONNECTION_TYPE,
                        g_variant_new_string(NM_SETTING_WIRELESS_SETTING_NAME));

    g_variant_builder_add(&connection_builder,
                        "{sa{sv}}",
                        NM_SETTING_CONNECTION_SETTING_NAME,
                        &setting_builder);

    /* Add the (empty) 'wired' Setting */
    g_variant_builder_init(&setting_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_WIRELESS_SSID,
                        value);

    g_variant_builder_add(&connection_builder,
                        "{sa{sv}}",
                        NM_SETTING_WIRELESS_SETTING_NAME,
                        &setting_builder);

    /* Build up the 'Password' Setting */
    g_variant_builder_init(&setting_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_WIRELESS_SECURITY_KEY_MGMT,
                        g_variant_new_string("wpa-psk"));
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_WIRELESS_SECURITY_PSK,
                        g_variant_new_string(password_char));

    g_variant_builder_add(&connection_builder,
                        "{sa{sv}}",
                        NM_SETTING_WIRELESS_SECURITY_SETTING_NAME,
                        &setting_builder);

    /* Build up the 'ipv4' Setting */
    g_variant_builder_init(&setting_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&setting_builder,
                        "{sv}",
                        NM_SETTING_IP_CONFIG_METHOD,
                        g_variant_new_string(NM_SETTING_IP4_CONFIG_METHOD_AUTO));
    g_variant_builder_add(&connection_builder,
                        "{sa{sv}}",
                        NM_SETTING_IP4_CONFIG_SETTING_NAME,
                        &setting_builder);

    /* Call AddConnection with the connection dictionary as argument.
    * (g_variant_new() will consume the floating GVariant returned from
    * &connection_builder, and g_dbus_proxy_call_sync() will consume the
    * floating variant returned from g_variant_new(), so no cleanup is needed.
    */
    ret = g_dbus_proxy_call_sync(proxy,
                                "AddAndActivateConnection",
                                g_variant_new("(a{sa{sv}}oo*)", &connection_builder, 
                                            g_variant_new_object_path(dev_obj_path),
                                            g_variant_new_object_path("/")
                                ),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (ret) {
        g_variant_get(ret, "(&o)", &new_con_path);
        g_print("Added: %s\n", new_con_path);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    if (proxy) {
        g_object_unref(proxy);
    }
    if (proxy2) {
        g_object_unref(proxy2);
    }
    if (value) {
        g_variant_unref(value);
    }
}

void NewtworkManager::disconnect_wireless() {
    GDBusProxy *props_proxy;
    GError *     error = NULL;
    GVariant *   ret;

    int n = device_paths[device_path_num].length();
    char dev_path[n + 1];
    strcpy(dev_path, device_paths[device_path_num].c_str());

    /* Create a D-Bus proxy to get the object properties from the NM Manager
    * object.  NM_DBUS_* defines are from nm-dbus-interface.h.
    */
    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL,
                                                NM_DBUS_SERVICE,
                                                dev_path,
                                                "org.freedesktop.NetworkManager.Device",
                                                NULL,
                                                NULL);
    g_assert(props_proxy);

    g_print("%s\n", dev_path);
    ret = g_dbus_proxy_call_sync(props_proxy,
                                "Disconnect",
                                NULL,
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);

    if (ret)
        g_variant_unref(ret);
    else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error disable connection: %s\n", error->message);
        g_clear_error(&error);
    }

    g_object_unref(props_proxy);
}

void NewtworkManager::activate_wireless() {
    GDBusProxy *props_proxy;
    GError *     error = NULL;
    GVariant *   ret;
    char * new_con_path;

    int n = device_paths[device_path_num].length();
    char dev_path[n + 1];
    strcpy(dev_path, device_paths[device_path_num].c_str());
    g_print("%s\n", dev_path);

    n = saved_connections[0].obj_dir.length();
    char sav_con[n + 1];
    strcpy(sav_con, saved_connections[0].obj_dir.c_str());
    g_print("%s\n", sav_con);

    /* Create a D-Bus proxy to get the object properties from the NM Manager
    * object.  NM_DBUS_* defines are from nm-dbus-interface.h.
    */
    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL,
                                                NM_DBUS_SERVICE,
                                                NM_DBUS_PATH,
                                                NM_DBUS_SERVICE,
                                                NULL,
                                                NULL);
    
    g_assert(props_proxy);

    ret = g_dbus_proxy_call_sync(props_proxy,
                                "ActivateConnection",
                                g_variant_new("(ooo)", "/", dev_path, "/"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (ret) {
        g_variant_get(ret, "(&o)", &new_con_path);
        g_print("Added: %s\n", new_con_path);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    g_object_unref(props_proxy);
}

void NewtworkManager::activate_wireless2() {
    GDBusProxy *props_proxy;
    GError *     error = NULL;
    GVariant *   ret;
    char * new_con_path;

    int n = device_paths[device_path_num].length();
    char dev_path[n + 1];
    strcpy(dev_path, device_paths[device_path_num].c_str());
    g_print("%s\n", dev_path);

    /* Create a D-Bus proxy to get the object properties from the NM Manager
    * object.  NM_DBUS_* defines are from nm-dbus-interface.h.
    */
    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL,
                                                NM_DBUS_SERVICE,
                                                dev_path,
                                                "org.freedesktop.DBus.Properties",
                                                NULL,
                                                NULL);
    
    g_assert(props_proxy);

    ret = g_dbus_proxy_call_sync(props_proxy,
                                "Set",
                                g_variant_new(
                                    "(ssv)", 
                                    NM_DBUS_INTERFACE_DEVICE, 
                                    "Autoconnect",
                                    g_variant_new_boolean(TRUE)
                                ),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (ret) {
        // g_variant_get(ret, "(&o)", &new_con_path);
        // g_print("Added: %s\n", new_con_path);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    g_object_unref(props_proxy);
}

void NewtworkManager::deactivate_wireless2() {
    GDBusProxy *props_proxy;
    GError *     error = NULL;
    GVariant *   ret;
    char * new_con_path;

    int n = device_paths[device_path_num].length();
    char dev_path[n + 1];
    strcpy(dev_path, device_paths[device_path_num].c_str());
    g_print("%s\n", dev_path);

    /* Create a D-Bus proxy to get the object properties from the NM Manager
    * object.  NM_DBUS_* defines are from nm-dbus-interface.h.
    */
    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL,
                                                NM_DBUS_SERVICE,
                                                dev_path,
                                                "org.freedesktop.DBus.Properties",
                                                NULL,
                                                NULL);
    
    g_assert(props_proxy);

    ret = g_dbus_proxy_call_sync(props_proxy,
                                "Set",
                                g_variant_new(
                                    "(ssv)", 
                                    NM_DBUS_INTERFACE_DEVICE, 
                                    "Autoconnect",
                                    g_variant_new_boolean(FALSE)
                                ),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (ret) {
        // g_variant_get(ret, "(&o)", &new_con_path);
        // g_print("Added: %s\n", new_con_path);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    g_object_unref(props_proxy);
}

///* Find Wi-Fi device to scan on. When no ifname is provided, the first Wi-Fi is used. */
void NewtworkManager::wifi_scan(){
    GDBusProxy *props_proxy;
    GError *     error = NULL;
    GVariant *   ret, *value;
    int64_t time_before, time_after;

    int n = device_paths[device_path_num].length();
    char dev_path[n + 1];
    strcpy(dev_path, device_paths[device_path_num].c_str());

    g_print("%s\n", dev_path);
    /* Create a D-Bus proxy to get the object properties from the NM Manager
    * object.  NM_DBUS_* defines are from nm-dbus-interface.h.
    */
    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL,
                                                NM_DBUS_SERVICE,
                                                dev_path,
                                                "org.freedesktop.DBus.Properties",
                                                NULL,
                                                NULL);
    
    g_assert(props_proxy);

    ret = g_dbus_proxy_call_sync(props_proxy,
                                "Get",
                                g_variant_new("(ss)", "org.freedesktop.NetworkManager.Device.Wireless", "LastScan"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (ret) {
        g_variant_get(ret, "(v)", &value);
        time_before = g_variant_get_int64(value);
        g_print("Added: %ld\n", time_before);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    
    system("nmcli device wifi rescan");

    sleep(100);

    ret = g_dbus_proxy_call_sync(props_proxy,
                                "Get",
                                g_variant_new("(ss)", "org.freedesktop.NetworkManager.Device.Wireless", "LastScan"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);
    if (ret) {
        g_variant_get(ret, "(v)", &value);
        time_after = g_variant_get_int64(value);
        g_print("Added: %ld\n", time_after);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    if (time_after != time_before) {
        g_print("Changed\n");
    }
    g_object_unref(props_proxy);
}

void NewtworkManager::read_saved_connections() {
    GDBusProxy *props_proxy;
    GError *     error = NULL;
    GVariant *   ret, *value;
    char ** paths;

    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        NULL,
                                        NM_DBUS_SERVICE,
                                        NM_DBUS_PATH_SETTINGS,
                                        "org.freedesktop.DBus.Properties",
                                        NULL,
                                        &error);
    
    g_assert(props_proxy);

    ret = g_dbus_proxy_call_sync(props_proxy,
                                "Get",
                                g_variant_new("(ss)", NM_DBUS_INTERFACE_SETTINGS, "Connections"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);

    if (ret) {
        g_variant_get(ret, "(v)", &value);
        paths = g_variant_dup_objv(value, NULL);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    for (int i = 0; paths[i]; i++) {
        saved_connections_struct temp;
        temp.obj_dir = paths[i];
        saved_connections.push_back(temp);
    }
    g_strfreev(paths);
    g_object_unref(props_proxy);
}

void NewtworkManager::print_setting(const char *setting_name, GVariant *setting)
{
    GVariantIter iter;
    const char * property_name;
    GVariant *   value;
    char *       printed_value;

    g_print("  %s:\n", setting_name);
    g_variant_iter_init(&iter, setting);
    while (g_variant_iter_next(&iter, "{&sv}", &property_name, &value)) {
        printed_value = g_variant_print(value, FALSE);
        if (strcmp(printed_value, "[]") != 0)
            g_print("    %s: %s\n", property_name, printed_value);
        g_free(printed_value);
        g_variant_unref(value);
    }
}

void NewtworkManager::print_connection(const char * path)
{
    GDBusProxy * proxy;
    GError *     error = NULL;
    GVariant *   ret, *connection = NULL, *s_con = NULL;
    const char * id, *type;
    gboolean     found;
    GVariantIter iter;
    const char * setting_name;
    GVariant *   setting;

    /* This function asks NetworkManager for the details of the connection */

    /* Create the D-Bus proxy so we can ask it for the connection configuration details. */
    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                          G_DBUS_PROXY_FLAGS_NONE,
                                          NULL,
                                          NM_DBUS_SERVICE,
                                          path,
                                          NM_DBUS_INTERFACE_SETTINGS_CONNECTION,
                                          NULL,
                                          NULL);
    g_assert(proxy);

    /* Request the all the configuration of the Connection */
    ret = g_dbus_proxy_call_sync(proxy,
                                 "GetSettings",
                                 NULL,
                                 G_DBUS_CALL_FLAGS_NONE,
                                 -1,
                                 NULL,
                                 &error);
    if (!ret) {
        g_dbus_error_strip_remote_error(error);
        g_warning("Failed to get connection settings: %s\n", error->message);
        g_error_free(error);
        goto out;
    }

    g_variant_get(ret, "(@a{sa{sv}})", &connection);

    s_con = g_variant_lookup_value(connection, NM_SETTING_CONNECTION_SETTING_NAME, NULL);
    g_assert(s_con != NULL);
    found = g_variant_lookup(s_con, NM_SETTING_CONNECTION_ID, "&s", &id);
    g_assert(found);
    found = g_variant_lookup(s_con, NM_SETTING_CONNECTION_TYPE, "&s", &type);
    g_assert(found);

    /* Dump the configuration to stdout */
    g_print("%s <=> %s\n", id, path);

    /* Connection setting first */
    print_setting(NM_SETTING_CONNECTION_SETTING_NAME, s_con);

    /* Then the type-specific setting */
    setting = g_variant_lookup_value(connection, type, NULL);
    if (setting) {
        print_setting(type, setting);
        g_variant_unref(setting);
    }

    g_variant_iter_init(&iter, connection);
    while (g_variant_iter_next(&iter, "{&s@a{sv}}", &setting_name, &setting)) {
        if (strcmp(setting_name, NM_SETTING_CONNECTION_SETTING_NAME) != 0
            && strcmp(setting_name, type) != 0)
            print_setting(setting_name, setting);
        g_variant_unref(setting);
    }
    g_print("\n");

out:
    if (s_con)
        g_variant_unref(s_con);
    if (connection)
        g_variant_unref(connection);
    if (ret)
        g_variant_unref(ret);
    g_object_unref(proxy);
}

void NewtworkManager::print_active_connection_details(const std::string str_path)
{
    GDBusProxy *props_proxy;
    GVariant *  ret = NULL, *path_value = NULL;
    const char *path  = NULL;
    GError *    error = NULL;

    int n = str_path.length();
    char obj_path[n + 1];
    strcpy(obj_path, str_path.c_str());
    /* This function gets the backing Connection object that describes the
     * network configuration that the ActiveConnection object is actually using.
     * The ActiveConnection object contains the mapping between the configuration
     * and the actual network interfaces that are using that configuration.
     */

    /* Create a D-Bus object proxy for the active connection object's properties */
    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                NULL,
                                                NM_DBUS_SERVICE,
                                                obj_path,
                                                "org.freedesktop.DBus.Properties",
                                                NULL,
                                                NULL);
    g_assert(props_proxy);

    /* Get the object path of the Connection details */
    ret = g_dbus_proxy_call_sync(
        props_proxy,
        "Get",
        g_variant_new("(ss)", NM_DBUS_INTERFACE_ACTIVE_CONNECTION, "Connection"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    if (!ret) {
        g_dbus_error_strip_remote_error(error);
        g_warning("Failed to get active connection Connection property: %s\n", error->message);
        g_error_free(error);
        goto out;
    }

    g_variant_get(ret, "(v)", &path_value);
    if (!g_variant_is_of_type(path_value, G_VARIANT_TYPE_OBJECT_PATH)) {
        g_warning("Unexpected type returned getting Connection property: %s",
                  g_variant_get_type_string(path_value));
        goto out;
    }

    path = g_variant_get_string(path_value, NULL);

    /* Print out the actual connection details */
    print_connection(path);

out:
    if (path_value)
        g_variant_unref(path_value);
    if (ret)
        g_variant_unref(ret);
    g_object_unref(props_proxy);
}

void NewtworkManager::read_active_connection() {
    GDBusProxy *props_proxy;
    GError *     error = NULL;
    GVariant *   ret, *value;
    char ** paths;

    props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                        G_DBUS_PROXY_FLAGS_NONE,
                                        NULL,
                                        NM_DBUS_SERVICE,
                                        NM_DBUS_PATH,
                                        "org.freedesktop.DBus.Properties",
                                        NULL,
                                        &error);
    
    g_assert(props_proxy);

    ret = g_dbus_proxy_call_sync(props_proxy,
                                "Get",
                                g_variant_new("(ss)", NM_DBUS_INTERFACE, "ActiveConnections"),
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error);

    if (ret) {
        g_variant_get(ret, "(v)", &value);
        paths = g_variant_dup_objv(value, NULL);
        g_variant_unref(ret);
    } else {
        g_dbus_error_strip_remote_error(error);
        g_print("Error adding connection: %s\n", error->message);
        g_clear_error(&error);
    }

    for (int i = 0; paths[i]; i++) {
        activeC_paths.push_back(paths[i]);
    }
    g_strfreev(paths);
    g_object_unref(props_proxy);
}

const std::vector<std::string> NewtworkManager::get_devicePaths() {
    return device_paths;
}

const std::vector<std::string> NewtworkManager::get_deviceTypes() {
    return device_types;
}

const std::vector<std::string> NewtworkManager::get_activeCPaths() {
    return activeC_paths;
}

const std::vector<wireless_struct> NewtworkManager::get_accessPoints() {
    return access_points;
}

const std::vector<saved_connections_struct> NewtworkManager::get_savedConnections() {
    return saved_connections;
}