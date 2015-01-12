//
//
//	sendcom.c
//
//
//
//	This program send a command to the itplanter via USB, and recevie response from the itplanter.
//	You can change this code, freely.
//
//	copyright itplants,ltd. since 2011.
//
//	Add -e option for command line mode. 2011.9.13 Y.S
//
//

#ifndef WIN32
#define MACOSX   // if you use MacOSX then define MACOSX simbole.
#endif

#ifdef WIN32
#include "usb.h"		// you need libusb library for compile this code.
#endif
#ifdef MACOSX
#include <libusb/libusb.h>  // User port for getting libusb binary library.
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#ifdef MACOSX
#include <unistd.h>
#endif

// itplanter
#define VENDOR_ID 0x250F // itplants,ltd.
//#define PRODUCT_ID 0x000A // itplanter-01
#define PRODUCT_ID 0x000C // itplanter-02

// Caltivation program ID
#define VERSION	1
#define REVISION	5

#ifdef  MACOSX
#define usb_init					libusb_init;
#define usb_find_busses		libusb_find_busses
#define usb_find_devices		usb_find_devices
#define usb_set_configuration	libusb_set_configuration
#define usb_claim_interface		linbusb_claim_interface
#define usb_bulk_write			libusb_bulk_write
#define usb_bulk_read			libusb_bulk_read
#define usb_release_interface	libusb_release_interface
#define usb_reset				libusb_reset
#define usb_close				libusb_close
#define usb_dev_handle			libusb_device_handle

#define EP_INTR			(1 | LIBUSB_ENDPOINT_IN)
#define EP_DATA			(2 | LIBUSB_ENDPOINT_IN)
#define CTRL_IN			(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CTRL_OUT		(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)
#define USB_RQ				0x04
#define INTR_LENGTH		64	//
#endif

#define EP_IN 0x81			// EP Adress	129
#define EP_OUT 0x01		// EP Adress	1
#define BUF_SIZE  64		//	

static char currentdir[1024]={'.','/','\0'};

void print_help(void);

int n_dev(void);
void command_list(void);

//
// count number of  itplanters
//
int n_dev()
{
	int i=0;	// number of itplanter 
#ifdef WIN32
	struct usb_bus *bus;
	struct usb_device *dev;
	
	usb_init(); 	/* initialize the library */
	usb_find_busses(); /* find all busses */
	usb_find_devices(); /* find all connected devices */
	
	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			
			if (dev->descriptor.idVendor == VENDOR_ID)
//				(dev->descriptor.idProduct == PRODUCT_ID )
			{
				i++; // found itplanters
			}
		}
	}
	//printf("%d itplanters found\n",i);
	return i;
#endif
    
#ifdef MACOSX
	libusb_device *dev;
	libusb_device **devs;
	int r;
	int j=0; 	// number of itplanter 
	ssize_t cnt;
	
	r = libusb_init(NULL);
	if (r < 0)	return  0;
	
	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0) return 0;
	
	while ( (dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			printf( "failed to get device descriptor");
			return 0;
		}
		if (desc.idVendor == VENDOR_ID){
			j++; // found itplanters
		}
    }
	return j;
#endif
}

#ifdef MACOSX
struct libusb_endpoint_descriptor *endpoint_;
void setEndPoint(struct libusb_endpoint_descriptor *endpoint) { endpoint_ = endpoint; }
struct libusb_endpoint_descriptor *getEndPoint() { return endpoint_; }

void printdev(libusb_device *dev) {
	int i, j, k;
	struct libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		printf("failed to get device descriptor\n");
		return;
	}
	
	printf("\n\nprintdev\n");
	
	printf("Number of possible configurations: %d\n",(int)desc.bNumConfigurations);
	printf("Device Class: %d\n",(int)desc.bDeviceClass);
	printf("VendorID: %d\n",desc.idVendor);
	printf("ProductID: %d\n",desc.idProduct);
	
	struct libusb_config_descriptor *config;
	libusb_get_config_descriptor(dev, 0, &config);
	printf("Interfaces: %d\n", (int)config->bNumInterfaces);
	struct libusb_interface *inter;
	struct libusb_interface_descriptor *interdesc;
	struct libusb_endpoint_descriptor *epdesc;
	for(i=0; i<(int)config->bNumInterfaces; i++) {
		inter = (struct libusb_interface *)&config->interface[i];
		printf("Number of alternate settings: %d\n",inter->num_altsetting);
		for(j=0; j<inter->num_altsetting; j++) {
			interdesc = (struct libusb_interface_descriptor *)&inter->altsetting[j];
			printf("Interface Number: %d\n",(int)interdesc->bInterfaceNumber);
			printf("Number of endpoints: %d\n",(int)interdesc->bNumEndpoints);
			for(k=0; k<(int)interdesc->bNumEndpoints; k++) {
				epdesc = (struct libusb_endpoint_descriptor *)&interdesc->endpoint[k];
				printf("Descriptor Type: %d\n",(int)epdesc->bDescriptorType);
				printf("EP Address: %d\n",(int)epdesc->bEndpointAddress);
				setEndPoint(epdesc);
			}
		}
	}
	printf("\n\n");
	libusb_free_config_descriptor(config);
}
#endif

//
// Open indent itplanter
//
#ifdef WIN32
usb_dev_handle*  open_dev(int n)
#endif
#ifdef MACOSX
libusb_device_handle*  open_dev(int n)
#endif
{
	int i=0;
#ifdef WIN32
	struct usb_bus *bus;
	struct usb_device *dev;
	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			
			if (dev->descriptor.idVendor == VENDOR_ID)
			//	&& dev->descriptor.idProduct == PRODUCT_ID)
			{
				i++;	// number of itplanter 
			}
		}
	}
	//printf("###%d itplanters found\n",i);
	i=0;
	
	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			
			if (dev->descriptor.idVendor == VENDOR_ID)
			//	&& dev->descriptor.idProduct == PRODUCT_ID)
			{
				//printf("Vend %04x Prod %04x\n",dev->descriptor.idVendor,dev->descriptor.idProduct);
				if(i==n-1)
					return usb_open(dev);
				else i++;
			}
		}
	}
	return NULL;
#endif
#ifdef MACOSX
	libusb_device *dev;
	libusb_device **devs;
	libusb_device_handle *devh = NULL;
	int r;
	static int j=0;
	ssize_t cnt;
	
	r = libusb_init(NULL);
	if (r < 0)
		return  NULL;
	
	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0) return  NULL;
	
	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			printf( "failed to get device descriptor");
			return NULL;
		}
		if (desc.idVendor == VENDOR_ID)
		//	&& desc.idProduct == PRODUCT_ID)	
			j++;
	}
	//printf("%d itplanters found\n",j);
	j=0; i=0;
	
	
	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			printf( "failed to get device descriptor");
			return NULL;
		}
		
		if (desc.idVendor == VENDOR_ID)
	//		&& desc.idProduct == PRODUCT_ID)
		{
			//printf("Vend %04x Prod %04x  n=%d i=%d\n",desc.idVendor, desc.idProduct,n,i);
			if(j == n-1){
				r = libusb_open(dev, &devh);
				
				// add for test
				//	printdev(dev);
				
				return devh;
			}  else j++;
		}
		//	if( desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID ){
		//printf("itplanter found : Vender ID %d Product ID %d port %d\n",
		//	   desc.idVendor, desc.idProduct, i);
		//}
	}
	//	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	
	return NULL;
#endif
}

int resetcom=0;
// 
//	send a command to specific itplanter
//
char* USB_Command(char* command, int n )
{
	usb_dev_handle *handle=NULL; /* the device handle */
	static char rcommand[1024];
	
	sprintf(rcommand,"");// Reset rcommand
	
#ifdef WIN32
	int result=0;
	int iretry=0;
 	void *request;
	int i;
	
retry:;
	usb_init();
	usb_find_busses();
	usb_find_devices();
	
	
	if ((handle = open_dev(n))==NULL)
	{
		// do retry.
		if(iretry<10){
			iretry++;
			goto retry;
		}
		fprintf (stderr,"error: open_dev failed\n", usb_strerror());
		exit(1);
	}
	
	if (usb_set_configuration(handle, 1) < 0)
	{
		if(usb_release_interface(handle, 0)<0){
			fprintf (stderr,"usb_release_interface error. (%s)\n", usb_strerror());
		}
		if(usb_close(handle)<0){
			fprintf (stderr,"usb_close error. (%s)\n", usb_strerror());
		}
		if(iretry<10){
			iretry++;
			goto retry;
		}
		fprintf (stderr,"error: setting config failed\n", usb_strerror());
		exit(1);
		
	}
	
	//	usb_resetep() ;
	/////
	if (usb_claim_interface(handle, 0) < 0)///<<<<<<
	{
		fprintf (stderr,"error: claiming interface 0 failed  %s\n", usb_strerror());
	
		if( usb_close(handle)<0){
			fprintf (stderr,"usb_close error. (%s)\n", usb_strerror());
		}
		if(iretry<10){
			iretry++;
			goto retry;
		}
		fprintf (stderr, "error: claiming interface 0-2 failed\n", usb_strerror());
		exit(1);
	}
	//
	//if(usb_set_altinterface(handle, 0)<0){
		//fprintf (stderr, "error: usb_set_altinterface failed\n", usb_strerror());
	//}
	
	////////
	//fprintf (stderr,"usb_bulk_write \n");
	result = usb_bulk_write(handle,EP_OUT, command, BUF_SIZE, 1000);
	//fprintf (stderr,"usb_bulk_write %d\n",result);
		// Reset command
	if(strcmp(command,"O")==0) exit(1);

	if(result<0){
	//	fprintf (stderr,"usb_bulk_write : return=%d  com=%s\n",result,command);

		if(usb_clear_halt(handle,  EP_OUT)<0){
			fprintf (stderr,"usb_clear_halt error. (%s)\n", usb_strerror());
		}
		if(usb_release_interface(handle, 0)<0){
			fprintf (stderr,"usb_release_interface error. (%s)\n", usb_strerror());
		}
		if(usb_close(handle)<0){
			fprintf (stderr,"usb_close error. (%s)\n", usb_strerror());
		}
		if(iretry<10){
			iretry++;
			goto retry;
		}  else {
			fprintf (stderr,"error. (%s)\n", usb_strerror());
			exit(1);
		}
	}
	

	
	//
	//
	//fprintf (stderr,"usb_bulk_read : \n");
	result = usb_bulk_read(handle, EP_IN , rcommand, BUF_SIZE, 1000);
	//fprintf (stderr,"usb_bulk_read : %d\n",result);
	if(result<0){
		fprintf (stderr,"usb_bulk_read : return=%d  com=%s\n",result,command);

		if(usb_clear_halt(handle,  EP_OUT)<0){
			fprintf (stderr,"usb_clear_halt EP_OUT error. (%s)\n", usb_strerror());
		}

		if(usb_clear_halt(handle,  EP_IN)<0){
			fprintf (stderr,"usb_clear_halt EP_IN error. (%s)\n", usb_strerror());
		}

		if(usb_resetep(handle,  EP_OUT)<0){
			fprintf (stderr,"usb_resetep EP_OUT error. (%s)\n", usb_strerror());
		}

		if(usb_resetep(handle,  EP_IN)<0){
			fprintf (stderr,"usb_resetep EP_IN error. (%s)\n", usb_strerror());
		}

		if(	usb_release_interface(handle, 0)<0){
			fprintf (stderr,"usb_release_interface error. (%s)\n", usb_strerror());
		}
		if(	usb_close(handle)<0 ){
			fprintf (stderr,"usb_close error. (%s)\n", usb_strerror());
		}
		if(iretry<10){
			iretry++;
			goto retry;
		}  else {
			fprintf (stderr,"error. (%s)\n", usb_strerror());
			printf ("error. (%s)\n", usb_strerror());
			exit(1);
		}
	}
	
	
	/*
	 if( usb_bulk_setup_async(handle, &request, EP_OUT) < 0) {
	 // error handling
	 usb_clear_halt(handle,  EP_OUT);
	 usb_release_interface(handle, 0);
	 usb_close(handle);
	 if(iretry<10){
	 iretry++;
	 goto retry;
	 }  else {
	 printf ("error. (%s)\n", usb_strerror());
	 exit(1);
	 }	
	 }
	 
	 
	 if(usb_submit_async(request, command, sizeof(command)) < 0) {
	 // error handling
	 usb_clear_halt(handle,  EP_OUT);
	 usb_release_interface(handle, 0);
	 usb_close(handle);
	 if(iretry<10){
	 iretry++;
	 goto retry;
	 }  else {
	 printf ("error. (%s)\n", usb_strerror());
	 exit(1);
	 }
	 }
	 
	 result =  usb_reap_async(request, 1000);
	 if(result  >= 0){
	 
	 printf("command %s\n", command);
	 
	 printf("read %d bytes\n", result );
	 printf("read %s\n", request );
	 
	 printf("command %s\n", rcommand);
	 printf("command %s\n", command);
	 }
	 else {
	 // error handling
	 usb_free_async(&request);
	 usb_clear_halt(handle,  EP_OUT);
	 usb_release_interface(handle, 0);
	 usb_close(handle);
	 if(iretry<10){
	 iretry++;
	 goto retry;
	 }  else {
	 printf ("error. (%s)\n", usb_strerror());
	 exit(1);
	 }
	 }
	 */

	if(usb_release_interface(handle, 0)<0){
		printf ("usb_release_interface error. (%s)\n", usb_strerror());
	}
	if( usb_close(handle)<0){
		printf ("usb_close error. (%s)\n", usb_strerror());
	}


	if(iretry>0) resetcom=1;
	
	return(rcommand);
#endif
	
#ifdef MACOSX
	int r;
	int transferred;
	int iretry=0;
	//static struct libusb_transfer *img_transfer = NULL;
	
retry:
	//img_transfer = libusb_alloc_transfer(0);	
	if (!(handle = open_dev(n)))
	{
		printf( "error :  device not found!\n");
		libusb_exit(NULL);	
		exit(1); 
	}
	
	r = libusb_claim_interface(handle, 0);
	if (r < 0) {
		//printf("usb_claim_interface error %d\n", r);
		libusb_clear_halt(handle,  EP_OUT);
		usb_release_interface(handle, 0);
		usb_close(handle);
		if(iretry<10){
			iretry++;
			goto retry;
		}  else {
			//printf ("error. (%s)\n", libusb_strerror());
			exit(1);
		}
	} //else 	printf("claimed interface\n");
	// write command
	
	if( libusb_bulk_transfer(handle, EP_OUT, command, INTR_LENGTH, &transferred, 1000)!= 0){
		libusb_clear_halt(handle,  EP_OUT);
		usb_release_interface(handle, 0);
		usb_close(handle);
		if(iretry<10){
			iretry++;
			goto retry;
		}  else {
			//printf ("error. (%s)\n", libusb_strerror());
			exit(1);
		}
	}
	
	// read data
	if( libusb_bulk_transfer(handle, EP_IN, rcommand, INTR_LENGTH, &transferred, 1000) !=0 ){
		libusb_clear_halt(handle,  EP_OUT);
		usb_release_interface(handle, 0);
		usb_close(handle);
		if(iretry<10){
			iretry++;
			goto retry;
		}  else {
			//printf ("error. (%s)\n", usb_strerror());
			exit(1);
		}
	}
	
	//	printf("r=%d\n",r);
	usb_release_interface(handle, 0);
	usb_close(handle);
	//libusb_exit(NULL);	
	
	if(iretry>0) resetcom=1;
	
	return( rcommand );
#endif
}

//
// send a command and translate response.
//
char* Process(char c,int n)
{
	char idata[64];
	char* odata;
	FILE *fd;
	char filename[128];
	char fullfilename[1152];
	unsigned char romdata[64];
	int i;
	
	sprintf(filename,"ROM.data");
	
#ifdef MSVC
	struct tm *newtime;
    __time64_t long_time;
    _time64( &long_time );           // Get time as 64-bit integer.
#endif
#ifndef MSVC
    time_t tim;
	struct tm *stm;
#endif
	unsigned char sec,min,hour,year,month,day;
	//	unsigned char data1[64],data2[64];
	int wlevel, wtemp, rtemp;
	int iamp, iphoto, itouch;
	//unsigned int i,j;
	
	switch(c){
		case 'A':
			idata[0]='A';idata[1]='\0';
			odata = USB_Command( idata, n);
			//	printf("Room Temprature :  %s", odata );
			//４００ｍV時が０度で１９．５ｍV／℃　になります。
			//	printf("Room Temprature :  %f [mV]\n", atof(odata)/1024.0*5.0*1000.0);
			//			rtemp = atoi(odata);
			sscanf(odata,"%s  %d",idata,&rtemp);
			if(rtemp < 100 ){
				printf("Room Temprature Sensor not found.  return value = %s\n",odata);
			} else  {
				printf("Room Temprature :  %3.1f [deg]\n", ((float)rtemp/1024.0*4.9*1000.0-
															400.0) /19.5-2.9);
			}
			break;
			
		case 'B':
			// Water level
			idata[0]='B';idata[1]='\0';
			odata = USB_Command( idata, n);
			sscanf(odata,"%s %d",idata,&rtemp);
			wlevel = rtemp - 196; // 196 is empty level, 485 is full level
			if(wlevel < 0 ) wlevel = 0;
			if( rtemp < -100 ){
				printf("Water level sensor not found. return value = %d\n",rtemp);
			} else {
				printf("Water level :  %3.1f [percent]\n", ((float)wlevel/(float)(500-200))
					   *100.0);
			}
			break;
#if 0
		case 'C':
			//pH
			idata[0]='C';idata[1]='\0';
			odata = USB_Command( idata,n);
			wtemp = atoi(odata);
			wtemp = (int)((float)wtemp*0.11-23);
			if( wtemp < 10 ){
				printf("pH  sensor not found. return value = %s\n",odata);
			} else {
				printf("pH :  %d \n", wtemp);
			}
			break;
			
		case 'D':
			// Absorp sensor
			idata[0]='D';idata[1]='\0';
			odata = USB_Command( idata,n);
			itouch = atoi(odata);
			if( itouch < 50 ){
				printf("Absorp sensor not found. return value = %s\n",odata);
			} else {
				printf("Absorp Sensor :  %d\n", itouch);
			}
			break;
			
		case 'E':
			// EC of Pump
			idata[0]='E';idata[1]='\0';
			odata = USB_Command( idata,n);
			iamp = atoi(odata);
			if( iamp < 20 ){
				printf("EC sensor not found. return value = %s\n",odata);
			} else {
				printf("EC Sensor :  %d\n", iamp);
			}
			break;
#endif
		case 'F':
			// photo sensor
			idata[0]='F';idata[1]='\0';
			odata = USB_Command( idata,n);
			//iphoto = atoi(odata);
			sscanf(odata,"%s %d",idata,&iphoto);
			if( iphoto < 10 ){
				printf("Photo sensor not found . return value = %s\n",odata);
			} else {
				printf("Photo Sensor :  %d [Lux]\n", iphoto);
			}
			break;
			
		case 'G':
			// Time Date setting
			//printf("Process G\n");
#ifdef MSVC
			newtime = _localtime64( &long_time ); // C4996
			hour = newtime->tm_hour;
			min = newtime->tm_min;
			sec = newtime->tm_sec;
			month = newtime->tm_mon;
			day = newtime->tm_mday;
			year = newtime->tm_year;
#endif
#ifndef MSVC
			time( &tim );
			stm = localtime( &tim );
			sec = stm->tm_sec;
			min = stm->tm_min;
			hour = stm->tm_hour;
			month = stm->tm_mon;
			day = stm->tm_yday;
			year = 12; // 2012
#endif
			sprintf(idata,"G%02d%02d%02d%02d%02d%02d",hour,min,sec,year,month,day);
			printf("->G %dh %dm %ds %dy %dm %dd\n",hour,min,sec,year,month,day );
			odata = USB_Command( idata,n);
			printf("%s", odata);
			break;
			
		case 'H':
			// Duty LED blink
			idata[0]='H';idata[1]='5';idata[2]='0';idata[3]='0';idata[4]='\0';
			odata = USB_Command( idata,n);
			printf("%s", odata);
			break;
			
		case 'Y':
#ifdef MSVC
			newtime = _localtime64( &long_time ); // C4996
			hour = newtime->tm_hour;
			min = newtime->tm_min;
			sec = newtime->tm_sec;
			month = newtime->tm_mon;
			day = newtime->tm_mday;
			year = newtime->tm_year;
#endif
#ifdef MACOSX
			time( &tim );
			stm = localtime( &tim );
			sec = stm->tm_sec;
			min = stm->tm_min;
			hour = stm->tm_hour;
			month = stm->tm_mon;
			day = stm->tm_yday;
			year = 13; // 2012
#endif
			sprintf(idata,"Y%c%c%c%c%c%c",hour,min,sec,year,month,day );
			printf("->Y %d %d %d %d %d\n",min,sec,year,month,day );
			odata = USB_Command( idata,n);
			printf("->TimeDate :  %s", odata);
			break;
#if 0
		case 'd':
			//ROM data write	2011.8.16 Y.S
			idata[0]='d';
			// read rom data from file
			sprintf( fullfilename, "%s%s",currentdir,filename);
			if((fd = fopen(fullfilename,"r") ) == NULL ){
				printf("Can not open ROM file : %s\n", fullfilename);
				break;
			}
			if( fscanf(fd,"%63c",romdata) != 63 ){
				printf("ROM.data file is wrong\n");
				break;
			}
			fclose(fd);
			// data copy
			for(i=1;i<64;i++) idata[i]=romdata[i-1];
			// 
			odata = USB_Command( idata,n);
			printf("%s", odata);
			
			// Verify
			idata[0]='T';
			odata = USB_Command( idata,n);
			
			// data check
			for(i=0;i<64;i++){
				if( odata[i] != romdata[i] ){
					printf("Missmatch ROM %d  Data %d\n",odata[i],romdata[i]);
				}
			}
			break;
			////
#endif
		default:
			idata[0]=c;idata[1]='\0';
			odata = USB_Command( idata,n);
			printf("%s", odata);
			break;
	}
	
	
	return(odata);
}	

void print_help()
{
	printf("sendcom ver.%d.%d\n",VERSION,REVISION);
	printf("itplant ltd. 2011 All reservid copyright\n");
	printf("args\n");
	printf("1 : or other number for itplanter select.\n");
	printf("-e [command] : command line mode.\n");
	printf("-h print help message.\n");
}

void command_list()
{
	printf("\n");
	printf("-------Please Input your command------\n");
	printf("Percent  Get/change planter No.\n");
	printf("*    Send command to all planters.\n");
	printf("G^   Set current time to real time clock.\n");
	printf(".   Change current directory.\n");
	printf("A   Temprature report.\n");
	printf("B   Water level report.\n");
	printf("C   Integrated Temp&Illum  report.\n");
	printf("D   Wong Pump Working count report.\n");
    printf("E   Electlical Water sensor  report.\n");
	printf("F   Photo sensor report.\n");
	printf("G   Set/get Time of RTC.\n");
	printf("H   Set/get duty ratio.\n");
	printf("I  set full water level .\n");
	printf("J  Set current water level as empty water level.\n");
	printf("L Light ON/OFF toggle.\n");
	printf("M Set/get return time from Manual mode to AUTO mode.\n");
	printf("N Set/get return time from PC mode to AUTO mode.\n");
	printf("U Set/get max pump working time.\n");
	printf("O Reset system.\n");
	printf("P  Pump ON/OFF toggle .\n");
	printf("T Read EEPROM data by binary.\n");
	printf("W Set/get illumination worning level.\n");
	printf("Y Set/get temprature worning level.\n");
	printf("a report pump/lamp working number.\n");
	printf("b Blue LED ON/OFF toggle.\n");
	printf("c Set/get white LED winker frequency.\n");
	printf("d write 32 byte data.\n");
	printf("e[address][n][data] write data to specific address.\n");
	printf("f Set/get Lamp schedule number.\n");
	printf("g Set/get Pump schedule number.\n");
//	printf("k Check Lamp schedule.\n");
//	printf("l Check Pump schedule.\n");
	printf("m report empty level, current level, full level.\n");
	printf("o Change to PC mode.\n");
	printf("p Change to AUTO mode.\n");
	printf("q Change to MANUAL  mode.\n");
	printf("r Red LED ON/OFF toggle.\n");
	printf("s set EEPROM to Factory settings.\n");
	printf("t Report current mode.\n");
	printf("u Set/get Pump Working Time[sec] max 65536.\n");
	printf("v report version.\n");
	printf("x Set/get water warnning level.\n");
	printf("y Night cultivation mode.\n");
	printf("z status report.\n");
	printf("Percent  Report/change itplanter number.\n");
	printf("*[command] send command to all itplanters.\n");
	printf("? Show this command list.\n");
	printf("-------type quit for exit program or Ctl-C------\n\n");
}

//
// main routine
//
#ifdef MSVC
int _tmain(int argc, _TCHAR* argv[])
#endif
#ifndef MSVC
int main(int argc, char* argv[])
#endif
{
	unsigned char idata[64];
	char* odata;
	int  i, n=1, m=0, q=0;
	
	for(i=1;i<argc;i++){
		// Planter No.
		if( atoi(argv[i]) >0 && atoi(argv[i])<128){// max device of USB id 127.
			n = atoi(argv[i]);
			continue;
		}
		
		// -e option added by Y.S 2011.9.13
		if(strcmp(argv[i],"-e") == 0 ){
			// Command mode
			sprintf(idata,"%s",argv[i+1]);
			
			if(idata[0]=='.'){ // Change currenr dir for rom_make data reading 8.16 Y.S
				printf("input currenr directory\n");
				scanf("%s",currentdir);
			}
			// change control device
			// sendcom -e %
			// nDevices 1 : retuen number of itplanters
			//
			if(idata[0]=='%'){
				if( strcmp( idata,"%") ==0){		
					n=n_dev();
					printf("Command: %s\n",idata);
					printf("ndevices  %d\n",n);
				} else {
					if( sscanf(idata+1, "%d", &n)  == 0  )  n=1;
					printf("Command: %s\n",idata);
					printf("devices  %d\n",n);
				} 
			} else {
				// command at once  2011.9.4
				if( idata[0] == '*' ){
					for(i=0;i<strlen(idata)-1;i++) idata[i]=idata[i+1];
					idata[strlen(idata)-1]='\0';
					for(i=1;i<=n_dev();i++){
						//
						printf("Command: %s\n",idata);
#if 0
						if(idata[0]== 'e'){
							// ROM write
							idata[1]=(unsigned char)atoi(argv[i+2]);//  start  address
							idata[2]=(unsigned char)atoi(argv[i+3]); //n schedule
							for(q=3; q<3+idata[2]; q++){// schedule data : first byte last byte....
								idata[q]=(unsigned char)atoi(argv[i+q+1]);
							}
						}
#endif
						odata = USB_Command( idata, i);
						printf("%s\n",odata);
					}
				} else {
					// single command 
					printf("Command: %s\n",idata);
					if( strcmp(idata,"^")==0 ) Process( 'G',n);  // Clock set　　type '^'
					if( strcmp(idata,"G^")==0 ) Process( 'G',n);  // Clock set　　type '^'
					else	  if( strcmp(idata,"d")==0 ) Process( 'd',n);  // Write ROM data
					else {
#if 0
						if(idata[0]== 'e'){
							// -e staddr nschedule firstbyte lastbyte .....
							// convert ascii to bin
							
							idata[1]=(unsigned char)atoi(argv[i+2]);//  start address					
							idata[2]=(unsigned char)atoi(argv[i+3]); //n schedule
							//printf("[1] %d\n",idata[1]);
							//printf("[2] %d\n",idata[2]);
							
							for(q=3; q<3+idata[2]; q++){// schedule data : first byte last byte....
								//printf("((%d))%s \n",i+q,argv[i+q+1]);
								idata[q]=(unsigned char)atoi(argv[i+q+1]);
								//printf("((idata %d))%d\n",q,idata[q]);
							}
						}
#endif
						
						odata = USB_Command( idata, n);
						printf("%s\n",odata);
					}
				}
			}
			
			
			// reset planter
			// retryが起きていたら、リセットしておく。
			if(resetcom){
				odata = USB_Command( "O", n);
			}
			//
			exit(1);
		}
		// help
		if(strcmp(argv[i],"-h") == 0 ){
			print_help();
			exit(1);
		}
	}
	
	
#ifndef MSVC
	if( argc > 1 ){
		n = atoi(argv[1]);
	} else {
		if( (m=n_dev()) > 1 ){
			// Ask which device do you use?
			printf("There are %d itplanters. Which planter do you like to use ? First No is    [1]:\n",m);
			scanf("%d",&n);
			if( n < 1 || n > m ) n = 1;
		} else n=1;
	}
#endif
#ifdef MSVC
	// PC mode
	if( argc > 1 ){
		n = _wtoi( argv[1] );
	} else {
		n=1;
	}
#endif
	printf("\n\n");
	printf("-------------------------------------------\n");
	printf("SendCom version %d.%d\n",VERSION,REVISION);
	printf("itplant ltd. 2011 All reservid copyright\n");
	printf("-------------------------------------------\n");
	printf("\n\n");
	
	printf("There are %d itplanter in this PC\n",n_dev());
	sprintf(idata,"o");
	odata = USB_Command(idata,n);
	
	//Process( 'A',n); // Temp Sensor
	//Process( 'B',n); // Water Level Sensor
	/*
	 Process( 'C',n);
	 Process( 'D',n);
	 Process( 'E',n);
	 */
	//Process( 'F',n);//  Photo Sensor
	
	//Process( 'G',n);  // Clock set
	//Process( 'H',n);  // Duty set
	
	
	command_list();
	
	//repeat_point:
	while(1){
		printf("Input Command\n");
		scanf("%s",idata);
		if( strcmp(idata,"quit") == 0) break;
		if( idata[0] == 0x1b ) break; // ESC
        
		
		if(idata[0]=='.'){ // Change currenr dir for rom_make data reading 8.16 Y.S
			printf("input currenr directory\n");
			scanf("%s",currentdir);
		}
		if(idata[0]=='?'){ // print command list 2011.9.14 Y.S
			command_list();
		}
		// change control device
		if(idata[0]== 0x25){
			if( strcmp( idata,"%") ==0){				
				if( (n=n_dev()) > 1 ){
					// Ask which device do you use?
					printf("There are %d itplanters. Which planter do you like to use ?   First No is [1]:\n",n);
					scanf("%d",&n);
					if( n < 1 || n > m ) n = 1;
					printf("Command: %s\n",idata);
					printf("ndevices  %d\n",n);
				}
			} else {
				if( sscanf(idata+1, "%d", &n)  == 0  )  n=1;
				printf("Command: %s\n",idata);
				printf("ndevices  %d\n",n);
			} 
		} else {
				// command at once  2011.9.4
			if( idata[0] == '*' ){
				for(i=0;i<strlen(idata)-1;i++) idata[i]=idata[i+1];
				idata[strlen(idata)-1]='\0';
				for(i=1;i<=n_dev();i++){
					//
					printf("Command:[%d] %s\n",i,idata);
					odata = USB_Command( idata, i);
					printf("%s\n",odata);
				}
			} else {
				// single command 
			printf("Command: %s\n",idata);
			if( strcmp(idata,"^")==0 ) Process( 'G',n);  // Clock set　　type '^'
			if( strcmp(idata,"G^")==0 ) Process( 'G',n);  // Clock set　　type '^'
#if 0
			else	  if( strcmp(idata,"d")==0 ) Process( 'd',n);  // Write ROM data
			else  if(idata[0]== 'e'){
					// ROM write
					idata[1]=(unsigned char)atoi(argv[i+2]);//  start address		
			
					idata[2]=(unsigned char)atoi(argv[i+3]); //n schedule
					for(q=3; q<3+idata[2]; q++){// schedule data : first byte last byte....
						idata[q]=(unsigned char)atoi(argv[i+q+1]);
					}
				}
#endif
			else {
				odata = USB_Command( idata, n);
				printf("%s\n",odata);
				}
			}
		}
	}
	return(0);
}

