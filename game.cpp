
#include <iostream>
#include <utility>
#include <format>
#include "VHInclude.h"
#include "VEInclude.h"

//those struct then go inside for the imgui, and whe have the name and the position
struct Name { std::string value; };
struct TypeTag { std::string value; };



class MyGame : public vve::System {

        enum class State : int {
            STATE_RUNNING,
            STATE_DEAD
        };

        const float c_max_time = 40.0f;
        const int c_field_size = 50;
        const int c_number_cubes = 10;

        int nextRandom() {
            return rand() % (c_field_size) - c_field_size/2;
        }

        int nextRandomBombHeight(){ //generate an height for the bomb
            return rand()% 10 + 6;
        }

    public:
        MyGame( vve::Engine& engine ) : vve::System("MyGame", engine ) {
    
            m_engine.RegisterCallbacks( { 
                {this,      0, "LOAD_LEVEL", [this](Message& message){ return OnLoadLevel(message);} },
                {this,  10000, "UPDATE", [this](Message& message){ return OnUpdate(message);} },
                {this, -10000, "RECORD_NEXT_FRAME", [this](Message& message){ return OnRecordNextFrame(message);} }
            } );
        };
        
        ~MyGame() {};

        void GetCamera() {
            if(m_cameraHandle.IsValid() == false) { 
                auto [handle, camera, parent] = *m_registry.GetView<vecs::Handle, vve::Camera&, vve::ParentHandle>().begin(); 
                m_cameraHandle = handle;
                m_cameraNodeHandle = parent;
            };
        }
    
        inline static std::string plane_obj  { "assets/test/plane/plane_t_n_s.obj" };
        inline static std::string plane_mesh { "assets/test/plane/plane_t_n_s.obj/plane" };
        inline static std::string plane_txt  { "assets/test/plane/grass.jpg" };

        inline static std::string cube_obj  { "assets/test/bomb/bomb_shading_v005.obj" };

        bool OnLoadLevel( Message message ) {
            auto msg = message.template GetData<vve::System::MsgLoadLevel>();	
            std::cout << "Loading level: " << msg.m_level << std::endl;
            std::string level = std::string("Level: ") + msg.m_level;

            // ----------------- Load Plane -----------------

            m_engine.SendMsg( MsgSceneLoad{ vve::Filename{plane_obj}, aiProcess_FlipWindingOrder });

           
            auto m_handlePlane = m_registry.Insert( 
                            vve::Position{ {0.0f,0.0f,0.0f } }, 
                            vve::Rotation{ mat3_t { glm::rotate(glm::mat4(1.0f), 3.14152f / 2.0f, glm::vec3(1.0f,0.0f,0.0f)) }}, 
                            vve::Scale{vec3_t{1000.0f,1000.0f,1000.0f}}, 
                            vve::MeshName{plane_mesh},
                            vve::TextureName{plane_txt},
                            vve::UVScale{ { 1000.0f, 1000.0f } },
                            Name{ "ground" },
                            TypeTag{ "plane" }
                        );

            m_engine.SendMsg(MsgObjectCreate{  vve::ObjectHandle(m_handlePlane), vve::ParentHandle{} });
    
            // ----------------- Load Cube -----------------

            for(int i = 0; i<11; i++ ){
                vecs::Handle m_handleCube{};
                m_handleCube = m_registry.Insert( 
                    vve::Position{ { nextRandom(), nextRandom(), nextRandomBombHeight() } }, 
                    vve::Rotation{ mat3_t { glm::rotate(glm::mat4(1.0f), 3.14152f / 2.0f, glm::vec3(1.0f,0.0f,1.0f)) }},
                    vve::Scale{vec3_t{0.01f}},
                    Name{ std::format("Bomb_{}", i) },
                    TypeTag{ "Bomb" }
                );
                m_fallingCubes.push_back(m_handleCube);
                m_engine.SendMsg(MsgSceneCreate{ vve::ObjectHandle(m_handleCube), vve::ParentHandle{}, vve::Filename{cube_obj}, aiProcess_FlipWindingOrder });
            }
            

            

            

            GetCamera();
            m_registry.Get<vve::Rotation&>(m_cameraHandle)() = mat3_t{ glm::rotate(mat4_t{1.0f}, 3.14152f/2.0f, vec3_t{1.0f, 0.0f, 0.0f}) };

            m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/ophelia.wav"}, 2, 50 });
			m_engine.SendMsg(MsgSetVolume{ (int)m_volume });

            return false;
        };
    
        bool OnUpdate( Message& message ) {
            auto msg = message.template GetData<vve::System::MsgUpdate>();
            
           
            if( m_state == State::STATE_RUNNING ) {
                m_time_left -= msg.m_dt;
                auto pos = m_registry.Get<vve::Position&>(m_cameraNodeHandle);
                pos().z = 0.5f;
                //m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/ophelia.wav"}, 0, 50 });



                //here the for to let all the bombs move
                for(auto& cube : m_fallingCubes){
                    //m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/ophelia.wav"}, 0, 50 });
                    //m_time_left -= msg.m_dt; // i think that if i don't update here it starts to make wierd things
                    auto posCube = m_registry.Get<vve::Position&>(cube);
                    posCube().x += 0.7f*msg.m_dt;
                    posCube().z -= 1.0f*msg.m_dt;   
                    float distance = glm::length( vec2_t{pos().x, pos().y} - vec2_t{posCube().x, posCube().y} );
                if (distance < 2.5f && posCube().x<2.5f){ //if the distance between the camera and the bomb is small then state is dead 
                    m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/ophelia.wav"}, 0, 50 });
                    m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/explosion.wav"}, 1 });
                    m_state = State::STATE_DEAD;
                    return false;
                }

                //here now i recicle the bombs: 
                if (posCube().z < 0.0f) {
                    posCube().x = nextRandom();
                    posCube().y = nextRandom();
                    posCube().z = nextRandomBombHeight();
                }
                    


                }
                


                 
                    //m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/ophelia.wav"}, 0, 50 });
                    //m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/gameover.wav"}, 1 });
                    ////return false;
                
                
                float timestamp = msg.m_dt;

                //for(int i; i < m_fallingCubes.size();i++){
                 //   auto posCube = m_registry.Get<vve::Position&>(m_fallingCubes[i]);
                 //   distance = glm::length( vec2_t{pos().x, pos().y} - vec2_t{posCube().x, posCube().y} );
                    //posCube().x += 0.5f*timestamp;
                    //posCube().z -= 0.5f*timestamp;           
                //}
               

                if (m_time_left <= 0.0f) {
                    m_state = State::STATE_DEAD;
                    m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/ophelia.wav"}, 0, 50 });
                    m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/bell.wav"}, 1 });
                    return false;
                }
            }

            return false;
        }
    
        bool OnRecordNextFrame(Message message) {  
            

            //just create the new window
            ImGui::Begin("Scene Overview");

            // and here i just put the type and the position into the window
            auto view = m_registry.GetView<vecs::Handle, Name, TypeTag, vve::Position>();

            for (auto&& [handle, name, type, pos] : view) {
                ImGui::SeparatorText(name.value.c_str());
                ImGui::Text("Type: %s", type.value.c_str());
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos().x, pos().y, pos().z);
            }

            ImGui::End();
    
            
            

            if( m_state == State::STATE_RUNNING ) {
                ImGui::Begin("Game State");
                char buffer[100];
                std::snprintf(buffer, 100, "Time Left: %.2f s", m_time_left);
                ImGui::TextUnformatted(buffer);
                //std::snprintf(buffer, 100, "Cubes Left: %d", m_cubes_left);
               // ImGui::TextUnformatted(buffer);
				if (ImGui::SliderFloat("Sound Volume", &m_volume, 0, 100.0)) {
					m_engine.SendMsg(MsgSetVolume{ (int)m_volume });
				}
                ImGui::End();
            }

            if( m_state == State::STATE_DEAD ) {
                ImGui::Begin("Game State");
                ImGui::TextUnformatted("Game Over");
                if (ImGui::Button("Restart")) {
                    m_state = State::STATE_RUNNING;
                    m_time_left = c_max_time;
                    //m_cubes_left = c_number_cubes;

                    //in case of restart then restart all the parameters
                    for (auto& cube : m_fallingCubes) {
                        auto pos = m_registry.Get<vve::Position&>(cube);
                        pos().x = nextRandom();
                        pos().y = nextRandom();
                        pos().z = 15.0f;
                    }

                    m_engine.SendMsg(MsgPlaySound{ vve::Filename{"assets/sounds/ophelia.wav"}, 2 });
                }
                ImGui::End();
            }
            return false;
        }

    private:
        State m_state = State::STATE_RUNNING;
        float m_time_left = c_max_time;
        int m_cubes_left = c_number_cubes;  
        vecs::Handle m_handlePlane{};
        
        std::vector<vecs::Handle> m_fallingCubes; //here i create a vector of cubes of the handle type 
		vecs::Handle m_cameraHandle{};
		vecs::Handle m_cameraNodeHandle{};
		float m_volume{50.0f};
    };
    
    
    
    int main() {
        vve::Engine engine("My Engine", VK_MAKE_VERSION(1, 3, 0)) ;
        MyGame mygui{engine};  
        engine.Run();
    
        return 0;
    }
    
    