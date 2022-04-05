#include <iostream>
#include "nm_class.h" 

using namespace std;

int main(int argc, const char* argv[]) {

    int num, num2;
    int en_wifi;
    string password;

    NewtworkManager nm{};

    system("clear");
    printf("\n\n\n");
    printf("== Select the proccess ==\n");
    printf("\t 1) Display Active Connections\n");
    printf("\t 2) Display Device Type\n");
    printf("\t 3) Display Avaliable Wifi\n");
    printf("\t 4) Enable/Disable WiFi\n");
    printf("\t 5) Add new WiFi Connections\n");
    printf("\t 6) Quit\n");

    printf("\n\n\n\n\n");
    printf("Key in the number for the task you want\t");
    cin >> num;

    vector<string> ap;
    vector<string> devp;
    vector<string> devt;
    vector<wireless_struct> ws;

    switch (num)
    {
    case 1:
        ap = nm.get_activeCPaths();
        for (int i = 0; i < ap.size(); i++) 
        {
            printf("\n\n");
            printf("Active connection %d <=> ", i + 1);
            cout << ap[i] << endl;
            nm.print_active_connection_details(ap[i]);
        }
        break;
    
    case 2:
        devp = nm.get_devicePaths();
        devt = nm.get_deviceTypes();

        printf("\n\n");
        for (int i = 0; i < devp.size(); i++) {
            cout << "Device" << i + 1 << ":\t" << devp[i] << "\t " << devt[i] << endl;
        }
        printf("\n\n");
        break;
    
    case 3:
        ws = nm.get_accessPoints();

        printf("\n\n");
        for (int i = 0; i < ws.size(); i++) {
            cout << "WiFi " << i + 1 << ":\t" << ws[i].obj_dir << "\t " << ws[i].ssid << endl;
        }
        printf("\n\n");
        break;
    
    case 4:
        printf("\n\n");
        printf("\t1) Enable (!!Need sudo access)\n");
        printf("\t2) Disable\n");

        cin >> en_wifi;
        en_wifi--;
        if (en_wifi) {
            nm.disconnect_wireless();
            printf("Disable\n");
        }
        else {
            nm.activate_wireless();
            
            printf("Enable\n");
        }
        break;

    case 5:
        ws = nm.get_accessPoints();

        printf("\n\n");
        for (int i = 0; i < ws.size(); i++) {
            cout << "\t" << i + 1 << ")" << "\t" << ws[i].obj_dir << "\t " << ws[i].ssid << endl;
        }
        
        printf("\n Pick the wifi to connect\t");
        cin >> num2;
        printf("\n Type in password\t");
        cin >> password;
        nm.saved_wireless(--num2, password);

        break;

    default:
        printf("ERROR please select 1 - 6\n");
        return 0;
    }
}   