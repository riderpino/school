#pragma once

#include "VEInclude.h"
#include "VESystem.h"
#include "VECS.h"

extern "C" {
	//#include <winsock2.h>
	#include <stdio.h>
	#include <stdlib.h>
	//#include <in6addr.h>
	//#include <ws2ipdef.h>
	#include <stdlib.h>
	#include <sys/types.h>
	#include <string.h>
	//#include <WS2tcpip.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <netdb.h>
	}

namespace vve {


	//-------------------------------------------------------------------------------------------------------

	class GUI : public System {

	public:
		GUI(std::string systemName, Engine& engine, std::string windowName);
    	~GUI() {};

		int sock;
		struct sockaddr_in addr;
		unsigned long packetnum;


		int keyboard_inputsock;
		struct sockaddr_in addr_keyboard;


		void init( char *address, int port );
		int send( char *buffer, int len  );
		void closeSock();

		//i add the second socket for reading msg: 

		int socket_report;
		sockaddr_in addrReport;

	private:
		bool OnAnnounce(Message message);
		bool OnKeyDown(Message message);
		bool OnKeyUp(Message message);
		bool OnMouseButtonDown(Message message);
		bool OnMouseButtonUp(Message message);
		bool OnMouseMove(Message message);
		bool OnMouseWheel(Message message);
		bool OnFrameEnd(Message message);
		void GetCamera();

		std::string m_windowName;
		bool m_mouseButtonDown=false;
		bool m_shiftPressed=false;
		int m_x = -1;
		int m_y = -1;
		vecs::Handle m_cameraHandle{};
		vecs::Handle m_cameraNodeHandle{};
		bool m_makeScreenshot{false};
		int m_numScreenshot{0};
		bool m_makeScreenshotDepth{false};


		//i'm adding this in order to take the output: 
		FILE* ffmpegPipe = nullptr;
		//UDPSend6 videoSender;
		bool isStreaming = false;
		

		

	};

};  // namespace vve

