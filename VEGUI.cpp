#include <ctime>
#include "VHInclude.h"
#include "VEInclude.h"
//#include "UDPsend6.h"
#include <time.h>


#define min(x,y)  ((x) <= (y) ? (x) : (y))
#define max(x,y)  ((x) >= (y) ? (x) : (y))



// shared between client and server
enum EventType { KEYDOWN = 0, KEYUP = 1 };

//same struct as before in the reciever
struct struttura_event {
    uint8_t type;     
    uint32_t keycode; 
};



// defining the classs ffmpegencoder

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <poll.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <vector>
#include <stdexcept>
#include <iostream>

//this class is excatly the same code as store.cpp but with very few changes, like the method encode 
class fmmpeg_encoder {
public:
    fmmpeg_encoder(int width, int height, int fps)
        : larghezza(width), altezza(height) {

        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) throw std::runtime_error("Codec not found");

        codecCtx = avcodec_alloc_context3(codec);
        codecCtx->bit_rate = 800000;
        codecCtx->width = width;
        codecCtx->height = height;
        codecCtx->time_base = {1, fps};
        codecCtx->framerate = {fps, 1};
        codecCtx->gop_size = 10;
        codecCtx->max_b_frames = 1;
        codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

		//i added those because was suggested in the moodle forum and actually reduce the latency
		av_opt_set(codecCtx->priv_data, "preset", "ultrafast", 0);
		av_opt_set(codecCtx->priv_data, "tune", "zerolatency", 0);

		//had to change with a throw because otherwise ive compiling errors
        if (avcodec_open2(codecCtx, codec, nullptr) < 0)
            throw std::runtime_error("Could not open codec");

        pkt = av_packet_alloc();
        yuvFrame = av_frame_alloc();
        yuvFrame->format = AV_PIX_FMT_YUV420P;
        yuvFrame->width = width;
        yuvFrame->height = height;
        av_image_alloc(yuvFrame->data, yuvFrame->linesize, width, height, AV_PIX_FMT_YUV420P, 32);

		

        swsCtx = sws_getContext(width, height, AV_PIX_FMT_BGRA,
                                  width, height, AV_PIX_FMT_YUV420P,
                                  SWS_BILINEAR, nullptr, nullptr, nullptr);
    }

    ~fmmpeg_encoder() {
        
    }

	//technically should work this setter method 
	void setFPS(int fps){
		codecCtx->time_base = {1, fps};
        codecCtx->framerate = {fps, 1};
	}

    std::vector<uint8_t> encode(uint8_t* rgbFrame) {

        uint8_t* data[1] = { rgbFrame };
        int linesize[1] = { 4 * larghezza };

		// Convert RGB frame to YUV frame
        sws_scale(swsCtx, data, linesize, 0, altezza, yuvFrame->data, yuvFrame->linesize);
		
		// Set the PTS (presentation timestamp)		  
        yuvFrame->pts = conteggio_frame++;

        if (avcodec_send_frame(codecCtx, yuvFrame) < 0)
            throw std::runtime_error("Error sending frame to codec");

        std::vector<uint8_t> output;
		
		//here i had to do this insert because i have to append the file in the vector (output.end)
        while (avcodec_receive_packet(codecCtx, pkt) == 0) {

            output.insert(output.end(), pkt->data, pkt->data + pkt->size);
            av_packet_unref(pkt);
        }

        return output;
    }




private:
    int larghezza;
	int altezza;
	
    int conteggio_frame = 0;

    AVCodecContext* codecCtx = nullptr;
    AVFrame* yuvFrame = nullptr;
    AVPacket* pkt = nullptr;
    SwsContext* swsCtx = nullptr;
};







namespace vve {
	fmmpeg_encoder* encoder = nullptr;



	GUI::GUI(std::string systemName, Engine& engine, std::string windowName ) : 
		System(systemName, engine), m_windowName(windowName) {
		m_engine.RegisterCallbacks( { 
			{this, 0, "SDL_KEY_DOWN", [this](Message& message){ return OnKeyDown(message);} },
			{this, 0, "SDL_KEY_REPEAT", [this](Message& message){ return OnKeyDown(message);} },
			{this, 0, "SDL_KEY_UP", [this](Message& message){ return OnKeyUp(message);} },
			{this, 0, "SDL_MOUSE_BUTTON_DOWN", [this](Message& message){ return OnMouseButtonDown(message);} },
			{this, 0, "SDL_MOUSE_BUTTON_UP", [this](Message& message){return OnMouseButtonUp(message);} },
			{this, 0, "SDL_MOUSE_MOVE", [this](Message& message){ return OnMouseMove(message); } },
			{this, 0, "SDL_MOUSE_WHEEL", [this](Message& message){ return OnMouseWheel(message); } },
			{this, 0, "FRAME_END", [this](Message& message){ return OnFrameEnd(message); } }
		} );
	};


	bool GUI::OnKeyDown(Message message) {
		GetCamera();
		int key;
		real_t dt;
		if (message.HasType<MsgKeyDown>()) {
			auto msg = message.template GetData<MsgKeyDown>();
			key = msg.m_key;
			dt = (real_t)msg.m_dt;
		} else {
			auto msg = message.template GetData<MsgKeyRepeat>();
			key = msg.m_key;
			dt = (real_t)msg.m_dt;
		}

		//to enable the streaming it's necessary to press the key V
		if (key == SDL_SCANCODE_V) {  //so here i just add this to start the video recording, i've check online and i found popen where i can set the video. i had to hardcode the frame size. to change the codec i can use  c:v and to change -b:v i change the bitrate
				encoder = new fmmpeg_encoder(1680, 1050, 30);
				
				//here there is the same initialization as in UDPSend.cpp for the IPV4 send 
				packetnum=0;
				sock=0;

				std::cout << "i have started the streaming"<< std::endl;
				if( sock ) close(sock);
	
				sock = socket( PF_INET, SOCK_DGRAM, 0 );

				//I hadcoded the address and the port for testing 
				addr.sin_family = AF_INET;  
				addr.sin_addr.s_addr = inet_addr("127.0.0.1");
				addr.sin_port = htons( 8000 );

				// i add the second socket for recieving messages: 

				socket_report = socket(AF_INET, SOCK_DGRAM, 0);
				addrReport.sin_family = AF_INET;
				addrReport.sin_port = htons(9000);  
				addrReport.sin_addr.s_addr = INADDR_ANY;

				if (bind(socket_report, (sockaddr*)&addrReport, sizeof(addrReport)) < 0) {
				perror("filed bind report");
				}


				// inside GUI::OnKeyDown (when V is pressed)
				keyboard_inputsock = socket(AF_INET, SOCK_DGRAM, 0);
				addr_keyboard.sin_family = AF_INET;
				addr_keyboard.sin_port = htons(7000);  // port for receiving remote input
				addr_keyboard.sin_addr.s_addr = INADDR_ANY;

				if (bind(keyboard_inputsock, (sockaddr*)&addr_keyboard, sizeof(addr_keyboard)) < 0) {
					perror("bind keyboard input");
}


				isStreaming = true;
			
			return false;
		}
		
		if (key == SDL_SCANCODE_B) {  // i had to find  way to close the recording so when i decide to stop it i have to press B
			if (isStreaming) {
				
				//to close the streaming and the socket 

				close(sock);
				close(socket_report);
				sock=0;
				isStreaming = false;
				
			}
			return false;
		}

		if( key == SDL_SCANCODE_ESCAPE  ) { m_engine.Stop(); return false; }
		if( key == SDL_SCANCODE_LSHIFT || key == SDL_SCANCODE_RSHIFT  ) { m_shiftPressed = true; return false; }

		if( key == SDL_SCANCODE_O  ) { m_makeScreenshot = true; return false; }
		if( key == SDL_SCANCODE_P  ) { m_makeScreenshotDepth = true; return false; }

		auto [pn, rn, sn, LtoPn] = m_registry.template Get<Position&, Rotation&, Scale&, LocalToParentMatrix>(m_cameraNodeHandle);
		auto [pc, rc, sc, LtoPc] = m_registry.template Get<Position&, Rotation&, Scale&, LocalToParentMatrix>(m_cameraHandle);		
	
		vec3_t translate = vec3_t(0.0f, 0.0f, 0.0f); //total translation
		vec3_t axis1 = vec3_t(1.0f); //total rotation around the axes, is 4d !
		float angle1 = 0.0f;
		vec3_t axis2 = vec3_t(1.0f); //total rotation around the axes, is 4d !
		float angle2 = 0.0f;

		int dx{0}, dy{0};
		switch( key )  {
			case SDL_SCANCODE_W : { translate = vec3_t{ LtoPn() * LtoPc() * vec4_t{0.0f, 0.0f, -1.0f, 0.0f} }; break; }
			case SDL_SCANCODE_S : { translate = vec3_t{ LtoPn() * LtoPc() * vec4_t{0.0f, 0.0f, 1.0f, 0.0f} }; break; }
			case SDL_SCANCODE_A : { translate = vec3_t{ LtoPn() * LtoPc() * vec4_t{-1.0f, 0.0f, 0.0f, 0.0f} }; break; }
			case SDL_SCANCODE_D : { translate = vec3_t{ LtoPn() * LtoPc() * vec4_t{1.0f, 0.0f, 0.0f, 0.0f} }; break; }
			case SDL_SCANCODE_Q : { translate = vec3_t(0.0f, 0.0f, -1.0f); break; }
			case SDL_SCANCODE_E : { translate = vec3_t(0.0f, 0.0f, 1.0f); break; }
			case SDL_SCANCODE_LEFT : { dx=-1; break; }
			case SDL_SCANCODE_RIGHT : { dx=1; break; }
			case SDL_SCANCODE_UP : { dy=1; break; }
			case SDL_SCANCODE_DOWN : { dy=-1; break; }
		}

		float speed = m_shiftPressed ? 20.0f : 10.0f; ///add the new translation vector to the previous one
		pn() = pn() + translate * (real_t)dt * speed;

		float rotSpeed = m_shiftPressed ? 2.0f : 1.0f;
		angle1 = rotSpeed * (float)dt * -dx; //left right
		axis1 = glm::vec3(0.0, 0.0, 1.0);
		rn() = mat3_t{ glm::rotate(mat4_t{1.0f}, angle1, axis1) * mat4_t{ rn() } };

		angle2 = rotSpeed * (float)dt * -dy; //up down
		axis2 = vec3_t{ LtoPc() * vec4_t{1.0f, 0.0f, 0.0f, 0.0f} };
		rc() = mat3_t{ glm::rotate(mat4_t{1.0f}, angle2, axis2) * mat4_t{ rc() } };

		glm::vec4 ctopp = LtoPc()[3];
		glm::vec4 ptppp = LtoPn()[3];
		//std::cout << "Camera Parent: (" << pn().x << ", " << pn().y << ", " << pn().z << ") " <<
	    //		        " Camera PT: (" << ptppp.x << ", " << ptppp.y << ", " << ptppp.z << ") " << std::endl;

		//glm::vec4 test = LtoPc() * vec4_t{0.0f, 0.0f, -1.0f, 0.0f};
		//std::cout << "Test: (" << test.x << ", " << test.y << ", " << test.z << ") " << std::endl;

		return false;
    }

	bool GUI::OnKeyUp(Message message) {
		auto msg = message.template GetData<MsgKeyUp>();
		int key = msg.m_key;
		if( key == SDL_SCANCODE_LSHIFT || key == SDL_SCANCODE_RSHIFT ) { m_shiftPressed = false; }
		return false;
	}

	bool GUI::OnMouseButtonDown(Message message) {
		auto msg = message.template GetData<MsgMouseButtonDown>();
		if(msg.m_button != SDL_BUTTON_RIGHT) return false;
 		m_mouseButtonDown = true; 
		m_x = m_y = -1; 
		return false;
	}

	bool GUI::OnMouseButtonUp(Message message) {
		auto msg = message.template GetData<MsgMouseButtonUp>();
		if(msg.m_button != SDL_BUTTON_RIGHT) return false;
		m_mouseButtonDown = false; 
		return false;
	}


	bool GUI::OnMouseMove(Message message) {
		if( m_mouseButtonDown == false ) return false;
		GetCamera();

		auto msg = message.template GetData<MsgMouseMove>();
		real_t dt = (real_t)msg.m_dt;
		if( m_x==-1 ) { m_x = msg.m_x; m_y = msg.m_y; }
		int dx = msg.m_x - m_x;
		m_x = msg.m_x;
		int dy = msg.m_y - m_y;
		m_y = msg.m_y;

		auto [pn, rn, sn] = m_registry.template Get<Position&, Rotation&, Scale&>(m_cameraNodeHandle);
		auto [pc, rc, sc, LtoPc] = m_registry.template Get<Position&, Rotation&, Scale&, LocalToParentMatrix>(m_cameraHandle);		
		
		vec3_t translate = vec3_t(0.0f, 0.0f, 0.0f); //total translation
		vec3_t axis1 = vec3_t(1.0f); //total rotation around the axes, is 4d !
		float angle1 = 0.0f;
		vec3_t axis2 = vec3_t(1.0f); //total rotation around the axes, is 4d !
		float angle2 = 0.0f;

		float rotSpeed = m_shiftPressed ? 1.0f : 0.5f;
		angle1 = rotSpeed * (float)dt * -dx; //left right
		axis1 = glm::vec3(0.0, 0.0, 1.0);
		rn() = mat3_t{ glm::rotate(mat4_t{1.0f}, angle1, axis1) * mat4_t{ rn() } };

		angle2 = rotSpeed * (float)dt * -dy; //up down
		axis2 = vec3_t{ LtoPc() * vec4_t{1.0f, 0.0f, 0.0f, 0.0f} };
		rc() = mat3_t{ glm::rotate(mat4_t{1.0f}, angle2, axis2) * mat4_t{ rc() } };

		return false;
	}

	
	bool GUI::OnMouseWheel(Message message) {
		GetCamera();
		auto msg = message.template GetData<MsgMouseWheel>();
		real_t dt = (real_t)msg.m_dt;
		auto [pn, rn, sn, LtoPn] = m_registry.template Get<Position&, Rotation&, Scale&, LocalToParentMatrix>(m_cameraNodeHandle);
		auto [pc, rc, sc, LtoPc] = m_registry.template Get<Position&, Rotation&, Scale&, LocalToParentMatrix>(m_cameraHandle);		
		
		float speed = m_shiftPressed ? 500.0f : 100.0f; ///add the new translation vector to the previous one
		auto translate = vec3_t{ LtoPn() * LtoPc() * vec4_t{0.0f, 0.0f, -1.0f, 0.0f} };
		pn() = pn() + translate * (real_t)dt * (real_t)msg.m_y * speed;
		return false;
	}

	void GUI::GetCamera() {
		if(m_cameraHandle.IsValid() == false) { 
			auto [handle, camera, parent] = *m_registry.GetView<vecs::Handle, Camera&, ParentHandle>().begin(); 
			m_cameraHandle = handle;
			m_cameraNodeHandle = parent;
		};
	}


	bool GUI::OnFrameEnd(Message message) {



		
		//i put all the code
		//startWinsock();
		

		//here i want to read from the reaciver the report, to do so i also to initiate the struct 
		struct struttura_report {
   		double bitrate;
   		double framerate;
		};

		struttura_report report;

		

		int bitrate_new = 30; 
		
		int framerate_new = 30;
		int framerate_old = 25;
		int recived_report;

		struct struttura_event {//yes i know is the same as before but i need it here and it wasent worling in those duble places 
			uint8_t type;     
			uint32_t keycode; 
		};

		
		//the rest of the code of udp send is here 

		if (isStreaming) { 
			//i put here the part that control the keyboard

			struttura_event remote_event;
			sockaddr_in senderAddr{};
			socklen_t senderLen = sizeof(senderAddr);

			int informazione = recvfrom(keyboard_inputsock, &remote_event, sizeof(remote_event), MSG_DONTWAIT,
								(sockaddr*)&senderAddr, &senderLen);

								//just listening for the events 
			if (informazione == sizeof(remote_event)) {
				SDL_Event sdl_event{};
				if (remote_event.type == 0) {
					sdl_event.type = SDL_KEYDOWN;
					sdl_event.key.state = SDL_PRESSED;
				} else if (remote_event.type == 1) {
					sdl_event.type = SDL_KEYUP;
					sdl_event.key.state = SDL_RELEASED;
				}

				sdl_event.key.type = sdl_event.type;
				sdl_event.key.keysym.scancode = (SDL_Scancode)remote_event.keycode;
				sdl_event.key.keysym.sym = SDL_GetKeyFromScancode(sdl_event.key.keysym.scancode);
				sdl_event.key.repeat = 0;

				SDL_PushEvent(&sdl_event);
			}
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			//same as before but check if ffmpegPipe is open then it starts makeing the video
			//std::cout << "i'm in the streaming"<< std::endl;;
			//here reading of the socket and the report:


			socklen_t lunghezza = sizeof(addrReport);
			recived_report = recvfrom(socket_report, &report, sizeof(report), MSG_DONTWAIT,(sockaddr*)&addrReport, &lunghezza);

			if(recived_report == sizeof(report)){
				bitrate_new = report.bitrate; 
				framerate_new = report.framerate; 
				if(framerate_new != framerate_old){
					std::cout << "the bitrate: " << bitrate_new << " kbibs, the framerate: " << framerate_new << " fps\n";
					framerate_old = framerate_new; 
					encoder->setFPS(framerate_new);
				}
				


			}



					

				
			auto vstate = std::get<1>(Renderer::GetState(m_registry));
			//1680 1050
			VkExtent2D extent = {1680, 1050}; //i am using mac, that's why ive decide to hardcode this into the code, because otherwise dismatch the right file size
			uint32_t imageSize = extent.width * extent.height * 4;
			VkImage image = vstate().m_swapChain.m_swapChainImages[vstate().m_imageIndex]; 
	
			uint8_t *dataImage = new uint8_t[imageSize];
	
			vh::ImgCopyImageToHost( //those are the image that i get from the
				vstate().m_device, vstate().m_vmaAllocator, vstate().m_graphicsQueue,
				vstate().m_commandPool, image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, dataImage, extent.width, extent.height,
				imageSize, 2, 1, 0, 3
			);

			//basically everything is the same as before but i addfed fwrite and fflush to write the frame 


			//here the method send

			

			//same as before but from now i implement the upd send, as implemented in UDPSend.cpp
			
			//here i encode the frames, and put everything in a vector 
			std::vector<uint8_t> h264Data = encoder->encode(dataImage);
			


			char *buffer = reinterpret_cast<char*>(h264Data.data());

			int len = static_cast<int>(h264Data.size());
			//int len = 64000;
			char test[] = "hello world";
			char sendbuffer[65000];
	
			packetnum++;
			
			
			typedef struct RTHeader {
				double		  time;
				unsigned long packetnum;
				unsigned char fragments;
				unsigned char fragnum;
			} RTHeader_t;
				
			
			//char sendbuffer[65000];
	
			//char sendbuffer[65000];
	
			
			
			if( len>65000 ) {
				return 0;
				std::cout <<"the value of len (if here there is a problem): "<< len << std::endl;
			}
			
			
			int maxpacketsize = 1400;
			
			//the header
			RTHeader_t header;
			header.time = clock() / static_cast<double>(CLOCKS_PER_SEC);
			header.packetnum = packetnum;
			header.fragments = len/maxpacketsize;
			header.fragnum=1;
			
			int left = len - header.fragments * maxpacketsize;
			if( left>0 ) header.fragments++;
			
			int ret, bytes=0;
			for( int i=0; i<header.fragments; i++ ) {
				memcpy( sendbuffer, &header, sizeof( header ) );
				memcpy( sendbuffer + sizeof( header), buffer+bytes, min(maxpacketsize, len - bytes) );
				
				ret = sendto( sock, sendbuffer, min(maxpacketsize, len - bytes) + sizeof(header), 0, (const struct sockaddr *) &addr, sizeof(addr) );
		
				if(ret == -1 ) {
					return ret;
				} else {
					ret = ret - sizeof( header );
				}
				bytes  += ret;
				header.fragnum++;
				//std::cout <<"the value of ret: "<< ret << std::endl;
			}
			
			

			delete[] dataImage;
		}
		
		return false;
	}
	


};  // namespace vve









