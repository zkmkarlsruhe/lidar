#include "ord_lidar_driver.h"

using namespace std;
using namespace ordlidar;

int running = 1;

static void sig_handle(int signo)
{
    printf("program exit, [%s,%s] Receive SIGNAL %d ====== \r\n", __FILE__, __func__, signo);
    running = 0;
    delay(1000);
    exit(1);
}


int main(int argc, char *argv[])
{
    (void)argc;(void)argv;

    signal(SIGINT, sig_handle);
    
    uint8_t type = ORADAR_TYPE_SERIAL;
    int model = ORADAR_MS200;

    OrdlidarDriver device(type, model);
    full_scan_data_st scan_data;

    #if defined(_WIN32)
    std::string port_name("com18");
    #else
//    std::string port_name("/dev/ttyACM0");
    std::string port_name("/dev/ttyUSB0");
    #endif
    int serialBaudrate = 230400;
    bool is_logging = false;
    bool ret = false;
    long long count = 0;
    device.SetSerialPort(port_name, serialBaudrate);

    while (running)
    {
        if (device.Connect())
        {
            printf("scan_frame_data lidar device connect succuss..\n");

            break;
        }
        else
        {
            printf("lidar device connect fail..\n");
            delay(1000);
        }
    }

    device.Activate();

//    device.SetRotationSpeed( 7.0 );
    
    int scanCount = 21;

    while (running)
    {
        ret = device.GrabFullScan(scan_data);
        if (ret)
        {
            printf("count = %lld, point_num: %d\n",++count, scan_data.vailtidy_point_num);
            if (is_logging)
            {
                for (int i = 0; i < scan_data.vailtidy_point_num; i++)
                {
                    printf("[%d: %f, %f] \n", i, (scan_data.data[i].distance * 0.001), scan_data.data[i].angle);
                }
            }
	    
	    printf( "speed: %g\n", device.GetRotationSpeed() );
	    
	    std::string top_fw_version, bot_fw_version, device_sn;
	    if ( device.GetFirmwareVersion(top_fw_version, bot_fw_version) )
	      printf("firmware: %s.%s\n", top_fw_version.c_str(), bot_fw_version.c_str() );
		 
	    if ( device.GetDeviceSN(device_sn) )
	      printf("SN: %s\n", device_sn.c_str() );


        }

	if ( scanCount > 0 && --scanCount == 0 )
	  device.Deactive();

        delay(100);
    }

    device.Disconnect();
exit:
    return 0;
}
