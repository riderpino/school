/*
 *  UDPReceive.cpp
 *  MyReceiver
 *
 *  Created by Helmut Hlavacs on 11.12.10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "UDPReceive.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>

#include <SDL2/SDL.h>
#include <arpa/inet.h>
#include <unistd.h>

std::queue<std::vector<uint8_t>> queue;
std::mutex mutex;
std::condition_variable frame_is_avilable;

extern "C" {
	#include <stdio.h>
	#include <string.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/imgutils.h>
	#include <libswscale/swscale.h>
}



typedef struct RTHeader {
	double		  time;
	unsigned long packetnum;
	unsigned char fragments;
	unsigned char fragnum;
} RTHeader_t;

struct struttura_report { 
    double bitrate;
    double framerate;
};


// shared between client and server
enum EventType { KEYDOWN = 0, KEYUP = 1 };

struct struttura_eventi { //it needs a struct
    uint8_t type;     // 
    uint32_t keycode; // 
};




UDPReceive::UDPReceive() {
	recbuffer = new char[65000];
}


void UDPReceive::init( int port ) {
	sock = socket( PF_INET, SOCK_DGRAM, 0 );
	
	addr.sin_family = AF_INET;  
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons( port );
	int ret = bind(sock, (const sockaddr *)&addr, sizeof(addr));
	
	printf("Binding port %d return %d\n", port, ret );
	
	leftover=false;
}


int UDPReceive::receive( char *buffer, int len, double *ptime ) {
	return receive( buffer, len, "", ptime );
}


int UDPReceive::receive( char *buffer, int len, char *tag, double *ptime ) {
	struct sockaddr_in si_other;
    socklen_t slen=sizeof(si_other);

	RTHeader_t *pheader = (RTHeader_t*)recbuffer;
	
	bool goon=true;
	int bytes = 0;
	int packetnum=-1;
	int fragments=-1;
	int fragnum=-1;
	int nextfrag=1;
	
	while( goon ) {
		
		int ret=0;
		
		if( !leftover ) {
			ret = recvfrom(sock, recbuffer, 65000, 0,(sockaddr*) &si_other, &slen);
		}
		leftover=false;
		
		//printf("%s UDP Packet %ld Size %d Fragment %d of %d Nextfrag %d\n", tag, pheader->packetnum, ret, pheader->fragnum, pheader->fragments, nextfrag );
		
		if( ret>sizeof( RTHeader_t ) ) {
			if( packetnum==-1 ) {						//first fragment of the new packet
				packetnum = pheader->packetnum;
			}
			
			if( packetnum != pheader->packetnum ) {		//last fragments lost
				printf("Last Frag %d lost", nextfrag );
				leftover = true;
				return -1;
			}

			//printf("%s UDP Packet %ld Size %d Fragment %d of %d Nextfrag %d\n", tag, pheader->packetnum, ret, pheader->fragnum, pheader->fragments, nextfrag );

			if( nextfrag!= pheader->fragnum ) {			//a fragment is missing
				printf("Fragment %d lost\n", nextfrag );
				return -1;
			}
			nextfrag++;
			
			memcpy( buffer+bytes, recbuffer + sizeof( RTHeader_t), ret - sizeof(RTHeader_t) );
			bytes += ret - sizeof( RTHeader_t );
			
			if(pheader->fragments == pheader->fragnum) goon=false;		//last fragment
			
			packetnum = pheader->packetnum;
			fragments = pheader->fragments;
			fragnum   = pheader->fragnum;
			
			*ptime = pheader->time;
			
		} else {
			printf("Fragment %d not larger than %d", pheader->fragnum, sizeof( RTHeader_t ) );
			return -1;
		}
		
	}
	
	leftover = false;
	return bytes;
}


void UDPReceive::closeSock() {
	close( sock );
}



//this is the thread that shows the video (for the assignment the trehad b)
void thread_b() {
	FILE* pipe = popen("ffplay -fflags nobuffer -f h264 -i -", "w");
    while (true) {
		//here it waits until the frame is available
        std::unique_lock<std::mutex> lock(mutex);
        frame_is_avilable.wait(lock, [] { return !queue.empty(); });

		//take the frame in front of the queue
        auto frame = queue.front();
        queue.pop();
        lock.unlock();

       
		//FILE* ffplayPipe = popen("ffplay -fflags nobuffer -f h264 -i -", "w");

        
        if (pipe) {
            fwrite(frame.data(), 1, frame.size(), pipe);
            fflush(pipe);
           
        }

        
    }
	 pclose(pipe);
}

void thread_a(UDPReceive& receiver) {
    auto start = std::chrono::steady_clock::now(); //take the time when i start the thread
    int tot_bytes_recieved = 0; //i want to sum all the byte i recieve
    int received_frames = 0;

	//i'm gonna put here all the socket managment to send the socket 
	int socket_report = socket(PF_INET, SOCK_DGRAM, 0);
	sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // same ip different port for the socket as the sender
	addr.sin_port = htons(9000); 

	//create an instance of the report_struttura struct
	struttura_report report; 

    while (true) { //here the operations
        char buffer[65000];
        double ptime = 0;
        int received = receiver.receive(buffer, sizeof(buffer), &ptime); //exaclty as i did before to receive the frame and encode
        if (received > 0) {
            std::vector<uint8_t> frame(buffer, buffer + received); //here in this vector i put the frame
            {
                std::lock_guard<std::mutex> lock(mutex); //is a critical section that's why it must be protected by the mutex
                queue.push(frame); //i put inside the frame
            }
            frame_is_avilable.notify_one(); //when is done, it notify that the frame is available (so i will never happend the starvation)
            tot_bytes_recieved = tot_bytes_recieved + received; //sum of the total bytes received
            received_frames++; //every frame i just increment this 
        }

        // Send report every 10 seconds
        auto right_now = std::chrono::steady_clock::now();


        if (std::chrono::duration_cast<std::chrono::seconds>(right_now - start).count() >= 10) { //when the time is 10 second then do all the report

            double bitrate = (tot_bytes_recieved * 8.0 )  / 10.0;  // bit per second 
            double framerate = received_frames / 10.0;
            

            // Send report to sender here (use a different udp socket)
            std::cout << "the bitrate: " << bitrate << " kbibs, the framerate: " << framerate << " fps\n";
			report.bitrate = bitrate; 
			report.framerate = framerate;

            sendto(socket_report, &report, sizeof(report), 0,(sockaddr*)&addr, sizeof(addr));

            tot_bytes_recieved = 0;
            received_frames = 0;
            start = right_now;
        }
    }
}

/*
//in this thread i open an sdl window and check the keyboard input, and send them to a new socket
void thread_c (){
	SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("remote", 100, 100, 800, 600, SDL_WINDOW_SHOWN);	

    //new socket same address but dofferent port
    int inputSock = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7000);  
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);  
	SDL_Event event;
	while (true) {
			while (SDL_PollEvent(&event)) {
				struttura_eventi messaggio{};

				if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
					
					messaggio.type = (event.type == SDL_KEYDOWN) ? KEYDOWN : KEYUP;
					messaggio.keycode = event.key.keysym.scancode;
					

					sendto(inputSock, &messaggio, sizeof(messaggio), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
					printf("the key is", messaggio.keycode);

				}
			}
			SDL_Delay(10);
		}
	close(inputSock);
    SDL_Quit();


}*/




//this is bascially all the same as UDPReceive cpp but the only difference is in this main that opens a ffplaypipe and reads the h264 stream 
/*
int main() {
    

	//the main strarts a ffplaypipe and open a window
    FILE* ffplayPipe = popen("ffplay -fflags nobuffer -f h264 -i -", "w");

    UDPReceive receiver;
    receiver.init(8000);   // on the sender i am sending on the port 8000

    std::cout << "is workinggggg" << std::endl;

    while (true) { 
		//start the while true cycle 
        char buffer[65000];
        double ptime = 0;
		//
        int received = receiver.receive(buffer, sizeof(buffer), &ptime);
        if (received > 0) {
            // write the frame
            size_t written = fwrite(buffer, 1, received, ffplayPipe);

            fflush(ffplayPipe); 
        }
        
    }

    pclose(ffplayPipe);
    receiver.closeSock();

    return 0;
}*/

int main() {
	
	UDPReceive receiver;
    receiver.init(8000);

    std::thread threada(thread_a, std::ref(receiver));

    //std::thread threadb(thread_b);
	//thread_c();
    
    //threadb.join();




	
    const int INPUT_PORT = 7000;
    const int WIDTH = 1680;
    const int HEIGHT = 1050;

    // 
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_Window* window = SDL_CreateWindow("Remote Game Stream",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    // socket
    int inputSock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serverAddr {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(INPUT_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // decoder  to decode the input and read the sdl events
    
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    avcodec_open2(codecContext, codec, nullptr);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    bool running = true;
    while (running) {
        //so here using sdl event to get the pressed key
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                struttura_eventi message;
                message.type = (event.type == SDL_KEYDOWN) ? KEYDOWN : KEYUP;
                message.keycode = event.key.keysym.scancode;
                sendto(inputSock, &message, sizeof(message), 0,
                    (sockaddr*)&serverAddr, sizeof(serverAddr));
            }
        }

        // here it waits for the frame, as before 
        std::unique_lock<std::mutex> lock(mutex);
        frame_is_avilable.wait(lock, [] { return !queue.empty(); });
        std::vector<uint8_t> encoded = std::move(queue.front());
        queue.pop();
        lock.unlock();

		//here this part is similar to the one of load.cpp and it neads to decode the frame that comes from the game. 
        av_packet_unref(packet);
        av_new_packet(packet, encoded.size());
        memcpy(packet->data, encoded.data(), encoded.size());

        if (avcodec_send_packet(codecContext, packet) == 0) {
			while (avcodec_receive_frame(codecContext, frame) == 0) {
				SDL_UpdateYUVTexture(texture, nullptr,frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1],frame->data[2], frame->linesize[2]);
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, nullptr, nullptr);
				SDL_RenderPresent(renderer);
			}
		}

        
    }

    threada.join();
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecContext);

    close(inputSock);

	
	

    return 0;

}
