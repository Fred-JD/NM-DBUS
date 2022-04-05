#include <stdlib.h>  // for EXIT_SUCCESS
#include <memory>    // for allocator, __shared_ptr_access
#include <string>  // for string, operator+, basic_string, to_string, char_traits
#include <vector>  // for vector, __alloc_traits<>::value_type
#include <cstring>

#include <gio/gio.h>
#include <uuid/uuid.h>
#include <nm-dbus-interface.h>
#include <NetworkManager.h>

const std::string DEV_TYPE_NAME[21] {
            "",
            "Ethernet",
            "Wi-Fi",
            "Bluetooth",
            "OLPC",
            "WiMAX",
            "Modem",
            "InfiniBand",
            "Bond",
            "VLAN",
            "ADSL",
            "Bridge",
            "Generic",
            "Team",
            "TUN",
            "IPTunnel",
            "MACVLAN",
            "VXLAN",
            "Veth",
            "Unknown"
};

struct network_device_struct {
    std::string obj_path;
    std::string dev_type;
};

struct wireless_struct {
    std::string obj_dir;
    std::string ssid;
    int strength;
};

struct saved_connections_struct {
    std::string obj_dir;
};

class NewtworkManager{
private:
    int num_device;
    int device_path_num;
    std::vector<std::string> device_paths;
    std::vector<std::string> device_types;
    std::vector<std::string> activeC_paths;
    std::vector<wireless_struct> access_points;
    std::vector<saved_connections_struct> saved_connections;

    void read_device_paths();
    void read_device_types();
    void read_access_point_detail(const char *path, char *ssid_name);
    void read_wireless_accesspoints();
    void read_saved_connections();
    void read_active_connection();
    
    void wifi_scan();
    void print_setting(const char *setting_name, GVariant *setting);

public:
    NewtworkManager();
    ~NewtworkManager() {}

    void print_connection(const char * path);
    void print_active_connection_details(const std::string obj_path);
    void connect_wireless(const int num_ap, const std::string password);
    void saved_wireless(const int num_ap, const std::string password);
    void disconnect_wireless();
    void activate_wireless();
    void activate_wireless2();
    const std::vector<std::string> get_devicePaths();
    const std::vector<std::string> get_deviceTypes();
    const std::vector<std::string> get_activeCPaths();
    const std::vector<wireless_struct> get_accessPoints();
    const std::vector<saved_connections_struct> get_savedConnections();
};