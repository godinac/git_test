#include "extcode.h"
#include <Windows.h>
#include <fstream>
#include <ctime>

using namespace std;

#include "globals.h"

#ifdef LOG_ENABLE
ofstream log_out;
#define LOG(T) log_out<<T
#else
#define LOG(T)
#endif

SOCKET sock_client = INVALID_SOCKET;
bool status_conexiune = false;
bool status_init_sock = false;

static float X;
static float Y;
static float Z;
static float RX;
static float RY;
static float RZ;

static float X2;
static float Y2;
static float Z2;
static float RX2;
static float RY2;
static float RZ2;

static float a;
static float v;

clock_t tt;

void init_vals()
{
	X = 0.0f;
	Y = 0.0f;
	Z = 0.0f;
	RX = 0.0f;
	RY = 0.0f;
	RZ = 0.0f;

	X2 = 0.0f;
	Y2 = 0.0f;
	Z2 = 0.0f;
	RX2 = 0.0f;
	RY2 = 0.0f;
	RZ2 = 0.0f;

	a = 0.0f;
	v = 0.0f;

	tt = 0;
}

bool WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID lpvres)
{
	switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			{
#ifdef LOG_ENABLE
				log_out.open(PATH_LOG, ofstream::out);
#endif
				status_init_sock = true;
				WSADATA wsData;
				if(WSAStartup(MAKEWORD(2, 2), &wsData)!=0)
				{
					status_init_sock = false;
					WSACleanup();
					LOG("Eroare initializare socket\n");
					return false;
				}

				init_vals();
			}break;
		case DLL_PROCESS_DETACH:
			{

				if(status_conexiune)
				{
					shutdown(sock_client, 0x02);
					closesocket(sock_client);
					sock_client = INVALID_SOCKET;
					status_conexiune = false;
				}

				WSACleanup();

#ifdef LOG_ENABLE
				if(log_out.good())
				{
					log_out.close();
				}
#endif
			}break;
	}
	return true;
}

extern "C" {

_declspec(dllexport) int32_t connect_robot(int16_t connect_but, CStr host, uint16_t port)
{
	if(connect_but==1)
	{

		if(status_conexiune == true)
		{
			if(sock_client != INVALID_SOCKET)
			{
				shutdown(sock_client, 0x02);
				closesocket(sock_client);
				sock_client = INVALID_SOCKET;
			}
			status_conexiune = false;
		}
		else
		{
			init_vals();
			status_conexiune = false;
			LOG("Conectare: ");
			LOG(host);
			LOG(" : ");
			LOG(port);
			LOG("\n");
			if(sock_client != INVALID_SOCKET)
			{
				shutdown(sock_client, 0x02);
				closesocket(sock_client);
				sock_client = INVALID_SOCKET;
			}

			if(!status_init_sock)
			{
				LOG("Initializare socket\n");
				status_init_sock = true;
				WSADATA wsData;
				if(WSAStartup(MAKEWORD(2, 2), &wsData)!=0)
				{
					status_init_sock = false;
					WSACleanup();
					LOG("Eroare initializare socket\n");
					return 0;
				}
			}

			sock_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if(sock_client == INVALID_SOCKET)
			{
				LOG("Eroare creare socket\n");
				return 0;
			}

			HOSTENT *host=NULL;
			if((host=gethostbyname((const char*)host))==NULL)
			{
				LOG("Host invalid!!!\n");	
				return 0;
			}

			SOCKADDR_IN serverInfo;
			memset(&serverInfo, 0, sizeof(serverInfo));
			serverInfo.sin_family = AF_INET;
			serverInfo.sin_port = htons(port);
			serverInfo.sin_addr.s_addr = *((unsigned long*)host->h_addr);

			if(connect(sock_client, (SOCKADDR*)&serverInfo, sizeof(serverInfo))!=0)
			{
				LOG("Problema conectare robot!\n");
				return 0;
			}

			u_long mod=1;
			ioctlsocket(sock_client,FIONBIO,&mod);

			status_conexiune = true;
		}
		return 1;
	}

	return 0;
}



_declspec(dllexport) int32_t getConnectionStatus(void)
{
	if(status_conexiune)
	{
		int eroare_sock = WSAGetLastError();
		if(eroare_sock!=WSAEWOULDBLOCK && eroare_sock!=0)
		{
			status_conexiune = false;
			LOG("Socket error: "<<eroare_sock<<"\n");
			return 0;
		}
		return 1;
	}

	return 0;
}

void trimite_mesaj(char* mesaj, int8_t status_but)
{
	if(status_but && status_conexiune)
	{
		int lung_mes = strlen((const char*)mesaj);
		if(lung_mes>0)
		{
			send(sock_client, (const char*) mesaj, lung_mes, 0);
			int eroare_sock = WSAGetLastError();
			if(eroare_sock!=WSAEWOULDBLOCK && eroare_sock!=0)
			{
				status_conexiune = false;
				LOG("Socket error: "<<eroare_sock<<"\n");
			}
		}
	}
}

_declspec(dllexport) void  getArt2(float *X, float *Y, float *Z, float *RX, float *RY, float *RZ)
{
	*X = ::X;
	*Y = ::Y;
	*Z = ::Z;
	*RX = ::RX;
	*RY = ::RY;
	*RZ = ::RZ;
}

_declspec(dllexport) void getArt(float *X2, float *Y2, float *Z2, float *RX2, float *RY2, float *RZ2)
{
	*X2 = ::X2;
	*Y2 = ::Y2;
	*Z2 = ::Z2;
	*RX2 = ::RX2;
	*RY2 = ::RY2;
	*RZ2 = ::RZ2;
}

_declspec(dllexport) void movej(int32_t but)
{
	if(clock()-tt>TIMEOUT_BUT && but>=1 && but<=12 && status_conexiune == true)
	{
		tt = clock();
		char mes[MAX_COMMAND_SIZE];
		memset(mes, 0, MAX_COMMAND_SIZE);
		switch(but)
		{
			case 1:
			{
				X-=INCREMENT_ROT;
			}break;
			case 2:
			{
				X+=INCREMENT_ROT;
			}break;
			default:
			{
			}break;
		}

		sprintf(mes, "movej([%f, %f, %f, %f, %f, %f], %f, %f)", X, Y, Z, RX, RY, RZ, a, v);
		trimite_mesaj(mes, true);
	}
}

}
