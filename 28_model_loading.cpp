

//#include "vulkan/vulkan.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define VOLK_IMPLEMENTATION
#include "volk.h"

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
//#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"

#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_vulkan.h"


#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>
#include <glm/gtx/string_cast.hpp>


const std::string m_MODEL_PATH = "models/viking_room.obj";
const std::string plane_model = "models/plane_t_n_s.obj";
const std::string m_TEXTURE_PATH = "textures/viking_room.png";
const std::string m_Grass = "textures/grass.jpg";

const std::string bomb_model = "models/bomb_shading_v005.obj";
const std::string bomb_texture = "textures/bump.png";

const std::string cube_model = "models/cube.obj";
const std::string cube_texture = "textures/diffuse.png";






const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> m_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

std::vector<const char*> m_sdl_instance_extensions = {};

#ifdef __APPLE__
const std::vector<const char*> m_deviceExtensions = {
    "VK_KHR_swapchain",
    "VK_KHR_portability_subset"
};
#else
const std::vector<const char*> m_deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#endif

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


//ImGUI
static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

VkInstance _instance;

auto loadVolk(const char* name, void* context) {
    return vkGetInstanceProcAddr(_instance, name);
}


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo
    , const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger
    , const VkAllocationCallbacks* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    //glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }
};

namespace std {

    template<> struct hash<glm::vec2> {
        size_t operator()(glm::vec2 const& vec) const {
            return (hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1)) >> 1;
        }
    };

    template<> struct hash<glm::vec3> {
        size_t operator()(glm::vec3 const& vec) const {
            return ((hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1)) >> 1) ^ (hash<float>()(vec.z) << 1);
        }
    };

    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
                ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}


//--------------------------------------------------------------------------------------------------


class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    VmaAllocator m_vmaAllocator;

    SDL_Window* m_sdlWindow{nullptr};
    bool m_isMinimized = false;
    bool m_quit = false;

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;

    QueueFamilyIndices m_queueFamilies;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    struct SwapChain {
        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImageView> m_swapChainImageViews;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;
    } m_swapChain;

    VkRenderPass m_renderPass;
    VkDescriptorSetLayout m_descriptorSetLayout;

    struct Pipeline {
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_pipeline;
    } m_graphicsPipeline;

    struct DepthImage {
        VkImage         m_depthImage;
        VmaAllocation   m_depthImageAllocation;
        VkImageView     m_depthImageView;
    } m_depthImage;

	//The texture of an object
    struct Texture {
        VkImage         m_textureImage;
        VmaAllocation   m_textureImageAllocation;
        VkImageView     m_textureImageView;
        VkSampler       m_textureSampler;
    };

	//Mesh of an object
    struct Geometry {
        std::vector<Vertex>     m_vertices;
        //std::vector<uint32_t>   m_indices;
        VkBuffer                m_vertexBuffer;
        VmaAllocation           m_vertexBufferAllocation;
        //VkBuffer                m_indexBuffer;
        //VmaAllocation           m_indexBufferAllocation;
    };

	//Uniform buffers of an object
    struct UniformBuffers {
        std::vector<VkBuffer>       m_uniformBuffers;
        std::vector<VmaAllocation>  m_uniformBuffersAllocation;
        std::vector<void*>          m_uniformBuffersMapped;
    };

    VkDescriptorPool m_descriptorPool;

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
        alignas(16) glm::vec4 cameraPos; 
	};

	//This holds all information an object with texture needs!
	struct Object {
         
		UniformBufferObject m_ubo; //holds model, view and proj matrix
        std::string name; //every object need an id, so i can play with them
		UniformBuffers m_uniformBuffers;
		Texture m_texture;
		Geometry m_geometry;

        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_pipeline;

        VkPipeline m_pipeline_o; //so every object has is own pipeline 
        VkPipelineLayout m_pipelineLayout_o; //here is say for the object which kind of pipeline layout it needs, bacause the struct 
        //pipeline is made like
        // VkPipelineLayout m_pipelineLayout;
        // and  VkPipeline m_pipeline;
        //so it needs both 
		std::vector<VkDescriptorSet> m_descriptorSets;
	};

	std::vector<Object> m_objects;

    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    struct SyncObjects {
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence>     m_inFlightFences;
    } m_syncObjects;

    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;

    //here i set some valute 
    std::unordered_map<SDL_Keycode, bool> whichKeyPressed;
    float thecameraYaw = 0.0f;
    float thecameraPitch = 0.0f;
    glm::vec3 m_cameraPosition = glm::vec3(0.0f, 0.0f, 1.5f); //starting from 0 0 0 
    glm::vec3 m_cameraFrontalView = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f)); //same here 
    glm::vec3 m_cameraUp = glm::vec3(0.0f, 0.0f, 1.0f); //so the camera knows the up èart


    void initWindow() {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
        #ifdef SDL_HINT_IME_SHOW_UI
            SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
        #endif

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        m_sdlWindow = SDL_CreateWindow("Tutorial", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
    }

    void initVMA(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& allocator) {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
        vmaCreateAllocator(&allocatorCreateInfo, &allocator);
    }


    //on this, i just add the selection of the pipeline that i'm gonna use
    //in create object i add the name
	void createObject(const std::string& name, VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator,
			VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout,
			glm::mat4&& model, std::string modelPath, std::string texturePath, Pipeline pipeline, std::vector<Object>& objects) {

		Object object{{model}};
        object.name = name;
		createTextureImage(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, texturePath, object.m_texture);
        createTextureImageView(device, object.m_texture);
        createTextureSampler(physicalDevice, device, object.m_texture);

        //loadModel(object.m_geometry, modelPath);
        loadModel_no_index(object.m_geometry, modelPath); 
        
        createVertexBuffer(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, object.m_geometry);
       //createIndexBuffer( physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, object.m_geometry);
        createUniformBuffers(physicalDevice, device, vmaAllocator, object.m_uniformBuffers);
        createDescriptorSets(device, object.m_texture, descriptorSetLayout, object.m_uniformBuffers, descriptorPool, object.m_descriptorSets);
        
        //i pass to create object the pipeline struct and i put inside the field of m_pipelinelayout and m_pipeline
        
        object.m_pipeline_o = pipeline.m_pipeline; //yess i'm adding this to the field of the struct object because i added it
        object.m_pipelineLayout_o = pipeline.m_pipelineLayout; 

		objects.push_back(object);
	}


    void initVulkanWOVkb() {
        createInstance(&m_instance, m_validationLayers);
        setupDebugMessenger(m_instance);
        createSurface(m_instance, m_surface);
        pickPhysicalDevice(m_instance, m_deviceExtensions, m_surface, m_physicalDevice);
        createLogicalDevice(m_surface, m_physicalDevice, m_queueFamilies, m_validationLayers, m_deviceExtensions, m_device, m_graphicsQueue, m_presentQueue);
    }   
    
    void initVulkanVkb() {
        volkInitialize();

        vkb::InstanceBuilder builder;
        auto inst_ret = builder.set_app_name ("Adapted Vulkan Tutorial")
							.require_api_version(VKB_VK_API_VERSION_1_1)
                            .request_validation_layers ()
                            .use_default_debug_messenger ()
                            .build ();
        if (!inst_ret) { std::cerr << "Failed to create instance!\n"; exit(-1); }
        vkb::Instance vkb_inst = inst_ret.value ();
        m_instance = vkb_inst.instance;
        _instance = m_instance;
        volkLoadInstance(m_instance);

        createSurface(m_instance, m_surface);

        vkb::PhysicalDeviceSelector selector{ vkb_inst };
        auto phys_ret = selector.set_surface (m_surface)
                            .set_minimum_version (1, 1)
                            .require_dedicated_transfer_queue ()
                            .select ();
        if (!phys_ret) { std::cerr << "Failed to select a physical device!\n"; exit(-1); }
        m_physicalDevice = phys_ret.value ();
    
        vkb::DeviceBuilder device_builder{ phys_ret.value () };
        auto dev_ret = device_builder.build ();
        if (!dev_ret) { std::cerr << "Failed to create logical device!\n"; exit(-1); }
        vkb::Device vkb_device = dev_ret.value ();
        m_device = vkb_device.device;
        volkLoadDevice(m_device);

        auto graphics_queue_ret = vkb_device.get_queue (vkb::QueueType::graphics);
        if (!graphics_queue_ret)  { std::cerr << "Failed to get graphics queue!\n"; exit(-1); }
        m_graphicsQueue = graphics_queue_ret.value (); 
        m_queueFamilies.graphicsFamily = vkb_device.get_queue_index (vkb::QueueType::graphics).value();

        auto present_queue_ret = vkb_device.get_queue (vkb::QueueType::present);
        if (!graphics_queue_ret)  { std::cerr << "Failed to get present queue!\n"; exit(-1); }
        m_presentQueue = present_queue_ret.value ();
        m_queueFamilies.presentFamily = m_queueFamilies.graphicsFamily = vkb_device.get_queue_index (vkb::QueueType::present).value();

    }


        const float c_max_time = 40.0f;
        const int c_field_size = 5000;
        const int c_number_cubes = 10;

        int nextRandom() {
            return rand() % (c_field_size) - c_field_size/2;
        }

        int nextRandomBombHeight(){ //generate an height for the bomb
            return rand()% 10 + 10;
        }

        int nextRandom_pasqua() {
            return rand() % (2000) - 2000/2;
        }

        int nextRandom_tree() {
            return rand() % (200) - 500/2;
        }


    void initVulkan() {


        //so now two different pipeline
        Pipeline pipeline_with_fill {};
        Pipeline pipeline_with_wire {};     //we pass this struct  to the function create graphics pipeline 

        //initVulkanVkb();
        initVulkanWOVkb();

        initVMA(m_instance, m_physicalDevice, m_device, m_vmaAllocator);
        createSwapChain(m_surface, m_physicalDevice, m_device, m_swapChain);
        createImageViews(m_device, m_swapChain);
        createRenderPass(m_physicalDevice, m_device, m_swapChain, m_renderPass);
        createDescriptorSetLayout(m_device, m_descriptorSetLayout);

        //i create the two different pipeline that i'm goign to use, one with object that are compitely filled and the other one with objects that are like the wire
        

        createGraphicsPipeline(m_device, m_renderPass, m_descriptorSetLayout, pipeline_with_fill, true);
        createGraphicsPipeline(m_device, m_renderPass, m_descriptorSetLayout, pipeline_with_wire, false);



        createCommandPool(m_surface, m_physicalDevice, m_device, m_commandPool);
        createDepthResources(m_physicalDevice, m_device, m_vmaAllocator, m_swapChain, m_depthImage);
        createFramebuffers(m_device, m_swapChain, m_depthImage, m_renderPass);
        createDescriptorPool(m_device, m_descriptorPool);



        //I change this create Object to upload the cube instead the original viking room, and i am gonna use the fill pipeline
        // x, y, z su giù, avanti indietro, destra sinistra

        //this is the cube - old pipeline
		//createObject("cube_1", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.90f, 0.90f, 0.90f)), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(-1.0f, 0.0f, 0.0f)), "models/cube.obj", "textures/cube3.png", pipeline_with_fill, m_objects);

        createObject("present", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.10f, 0.10f, 0.10f)), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(nextRandom_pasqua(),nextRandom_pasqua(), 0.1f)), "models/gift_box.obj", "textures/gift_texture.jpg", pipeline_with_fill, m_objects);
        createObject("balloon", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.50f, 0.50f, 0.50f)), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(10.0f, 5.0f, 15.0f)), "models/balloon.obj", "textures/balloon_texture.png", pipeline_with_fill, m_objects);
            //simply copy paste of the same function but i want it to rotate my plane of course so i add glm rotate and i know that i can put those functions together
         createObject("easter", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f)), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(nextRandom_pasqua(), nextRandom_pasqua(), -3.0f)), "models/easter.obj", "textures/easter_diffuse.jpg", pipeline_with_fill, m_objects);
            //always possible to compbine those translate rotate and translate 

        //tree
        createObject("tree", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(50.0f,0.0f, 10.0f)), "models/tree.obj", "textures/tree_color.png", pipeline_with_fill, m_objects);
        createObject("tree", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(-20.0f,0.0f, -30.0f)), "models/tree.obj", "textures/tree_color.png", pipeline_with_fill, m_objects);
        createObject("tree", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(10.0f,0.0f, -60.0f)), "models/tree.obj", "textures/tree_color.png", pipeline_with_fill, m_objects);

        //this is the plane - old pipeline
        createObject("plane",m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(200.0f, 200.0f, 10.0f)), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(0.0f, -0.01f, 0.0f)), plane_model, m_Grass, pipeline_with_fill, m_objects);
        
        
        // now the old pipeline for the bomb
        //createObject("bomb_1", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.02f, 0.02f)), glm::radians(90.0f), glm::vec3(1.0f, 2.0f, 2.0f)), glm::vec3(150.0f, 100.0f, 100.0f)), "models/bomb_shading_v005.obj", "textures/grunge-wall-texture.jpg", pipeline_with_fill, m_objects);
        //the new pipeline wire for the bomb 2
        //createObject("bomb_2", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.02f, 0.02f)), glm::radians(90.0f), glm::vec3(1.0f, 2.0f, 2.0f)), glm::vec3(200.0f, 120.0f, 200.0f)), "models/bomb_shading_v005.obj", "textures/grunge-wall-texture.jpg", pipeline_with_wire, m_objects);
        //third object for the new pipeline bomb n.3 
        //createObject("bomb_3", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.02f, 0.02f)), glm::radians(90.0f), glm::vec3(1.0f, 2.5f, 2.0f)), glm::vec3(50.0f, 120.0f, 200.0f)), "models/bomb_shading_v005.obj", "textures/grunge-wall-texture.jpg", pipeline_with_wire, m_objects);
        //another cube but this time with the new pipeline and a different texture
        //createObject("cube", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.90f, 0.90f, 0.90f)), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(-1.0f, 2.50f, 0.0f)), "models/cube.obj", "textures/bump.png", pipeline_with_wire, m_objects);
       
        

        for (int i = 0; i<3; i++){
            createObject("bomb_bad", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.04f, 0.04f, 0.04f)), glm::radians(90.0f), glm::vec3(1.0f, 2.0f, 2.0f)), glm::vec3(nextRandom(),120.0f, nextRandom())), "models/bomb_shading_v005.obj", "textures/grunge-wall-texture.jpg", pipeline_with_fill, m_objects);


        }
        for (int i = 0; i<3; i++){
            createObject("bomb_time", m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool, m_descriptorPool, m_descriptorSetLayout, glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.04f, 0.04f, 0.04f)), glm::radians(90.0f), glm::vec3(1.0f, 2.0f, 2.0f)), glm::vec3(nextRandom(),120.0f, nextRandom())), "models/bomb_shading_v005.obj", "textures/grunge-wall-texture.jpg", pipeline_with_wire, m_objects);


        }

            //insert the bomb pipeline filled for this other object
		//createObject(m_physicalDevice, m_device, m_vmaAllocator, m_graphicsQueue, m_commandPool,
		//	m_descriptorPool, m_descriptorSetLayout, glm::mat4{1.0f}, m_MODEL_PATH, m_TEXTURE_PATH, pipeline_with_fill, m_objects);

            //simply copy paste of the same function but i want it to rotate my plane of course so i add glm rotate and i know that i can put those functions together
            //always possible to compbine those translate rotate and translate 
        
       
        
            
            

		createCommandBuffers(m_device, m_commandPool, m_commandBuffers);
        createSyncObjects(m_device, m_syncObjects);
        setupImgui(m_instance, m_physicalDevice, m_queueFamilies, m_device, m_graphicsQueue, m_commandPool
            , m_descriptorPool, m_renderPass);
    }


    void mainLoop() {
        SDL_Event event;
        SDL_PollEvent(&event);
        
        while (!m_quit) {
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event); // Forward your event to backend

                if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE )
                    m_quit = true;

                if (event.type == SDL_WINDOWEVENT) {
                    switch (event.window.event) {
                    case SDL_WINDOWEVENT_MINIMIZED:
                        m_isMinimized = true;
                        break;

                    case SDL_WINDOWEVENT_MAXIMIZED:
                        m_isMinimized = false;
                        break;

                    case SDL_WINDOWEVENT_RESTORED:
                        m_isMinimized = false;
                        break;
                    }
                }
                //this inthe main loop help me to catch the pressed key and save it in this variable whichkeypressed where basically save the status of the pressedekeu
                if (event.type == SDL_KEYDOWN) {
                    whichKeyPressed[event.key.keysym.sym] = true;
                }
                if (event.type == SDL_KEYUP) {
                    whichKeyPressed[event.key.keysym.sym] = false;
                }
            }

            if(!m_isMinimized) {
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplSDL2_NewFrame();
                ImGui::NewFrame();

                //ImGui::ShowDemoWindow(); // Show demo window! :)
                ImGui::Begin("catch all the pasquas");
                //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::Text("Pasquas remaining: %d", pasqua_left);
                ImGui::Text("Time remaining seconds: %d", time_left);
                ImGui::Text("Speed X%d", (int)speed_multiplier);
                ImGui::End();

                if(pasqua_left>0){
                    game_stop = false;
                    

                }else {
                     ImGui::Text("🎉 winner! 🎉");
                     game_stop = true;
                     if (ImGui::Button("Restart")) {
                         // Logica di restart: azzera stato, rigenera pasquas ecc.
                        //ResetGame(); 
                        
                        pasqua_left = 10;
                        time_left = 120;
                        game_stop = false;
                     }
                
                }

                if(time_left<=0 || hitted){
                    ImGui::Text(" loser! ");
                    game_stop = true;
                    if (ImGui::Button("Restart")) {
                         // Logica di restart: azzera stato, rigenera pasquas ecc.
                        //ResetGame(); 
                        
                        pasqua_left = 10;
                        time_left = 120;
                        game_stop = false;
                     }
                }

                drawFrame(m_sdlWindow, m_surface, m_physicalDevice, m_device, m_vmaAllocator
                    , m_graphicsQueue, m_presentQueue, m_swapChain, m_depthImage
                    , m_renderPass, m_graphicsPipeline, m_objects, m_commandBuffers
					, m_syncObjects, m_currentFrame, m_framebufferResized);

                
            }
        }
        vkDeviceWaitIdle(m_device);
    }

    void cleanupSwapChain(VkDevice device, VmaAllocator vmaAllocator, SwapChain& swapChain, DepthImage& depthImage) {
        vkDestroyImageView(device, depthImage.m_depthImageView, nullptr);

        destroyImage(device, vmaAllocator, depthImage.m_depthImage, depthImage.m_depthImageAllocation);

        for (auto framebuffer : swapChain.m_swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto imageView : swapChain.m_swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain.m_swapChain, nullptr);
    }

    void cleanup() {

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        cleanupSwapChain(m_device, m_vmaAllocator, m_swapChain, m_depthImage);

        vkDestroyPipeline(m_device, m_graphicsPipeline.m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicsPipeline.m_pipelineLayout, nullptr);
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);


		for( auto& object : m_objects) {
	        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
	            destroyBuffer(m_device, m_vmaAllocator, object.m_uniformBuffers.m_uniformBuffers[i], object.m_uniformBuffers.m_uniformBuffersAllocation[i]);
	        }

	        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

	        vkDestroySampler(m_device, object.m_texture.m_textureSampler, nullptr);
	        vkDestroyImageView(m_device, object.m_texture.m_textureImageView, nullptr);

	        destroyImage(m_device, m_vmaAllocator, object.m_texture.m_textureImage, object.m_texture.m_textureImageAllocation);

	        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

	        //destroyBuffer(m_device, m_vmaAllocator, object.m_geometry.m_indexBuffer, object.m_geometry.m_indexBufferAllocation);

	        destroyBuffer(m_device, m_vmaAllocator, object.m_geometry.m_vertexBuffer, object.m_geometry.m_vertexBufferAllocation);
		}

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device, m_syncObjects.m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_syncObjects.m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_syncObjects.m_inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(m_device, m_commandPool, nullptr);

        vmaDestroyAllocator(m_vmaAllocator);

        vkDestroyDevice(m_device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);

        SDL_DestroyWindow(m_sdlWindow);
        SDL_Quit();

    }

    void recreateSwapChain(SDL_Window* window, VkSurfaceKHR surface
        , VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, SwapChain& swapChain
        , DepthImage& depthImage, VkRenderPass renderPass) {
        int width = 0, height = 0;

        SDL_GetWindowSize(window, &width, &height);
        while (width == 0 || height == 0) {
            SDL_Event event;
            SDL_WaitEvent(&event);
            SDL_GetWindowSize(window, &width, &height);
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain(device, vmaAllocator, swapChain, depthImage);

        createSwapChain(surface, physicalDevice, device, swapChain);
        createImageViews(device, swapChain);
        createDepthResources(physicalDevice, device, vmaAllocator, swapChain, depthImage);
        createFramebuffers(device, swapChain, depthImage, renderPass);
    }


    void createInstance(VkInstance *instance, const std::vector<const char*>& validationLayers) {
        volkInitialize();

        if (enableValidationLayers && !checkValidationLayerSupport( validationLayers)) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Tutorial";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        #ifdef __APPLE__
        appInfo.apiVersion = VK_API_VERSION_1_2;
        #else
        appInfo.apiVersion = VK_API_VERSION_1_3;
        #endif

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        #ifdef __APPLE__
		createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
		#endif
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        _instance = *instance;

   		volkLoadInstance(*instance);
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger(VkInstance instance) {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface(VkInstance instance, VkSurfaceKHR& surface) {
        if (SDL_Vulkan_CreateSurface(m_sdlWindow, instance, &surface) == 0) {
            printf("Failed to create Vulkan surface.\n");
        }
    }


    //here is where i have all the device informations

    void pickPhysicalDevice(VkInstance instance, const std::vector<const char*>& deviceExtensions, VkSurfaceKHR surface, VkPhysicalDevice& physicalDevice) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        //here i'm adding a code to iterate trough the information
        int kount = 0;
        for (const auto& device : devices) {
            kount++;
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
        
            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);
            
        
            //std::cout << ".... device n. "<< kount <<  "...." << "\n";
            //std::cout << "name : " << props.deviceName << "\n";
            //std::cout << "the vendor identifier: " << props.vendorID << "\n";
            
            //here an if because i want to know wich type if discrete or integrated
            //std::cout << "type is : "  << (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "externa gpu" : props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "integrated gpu" : "something else") << "\n";
            //with this just ask for the anisotropy 
            //std::cout << "is anisotropy available?: " << (features.samplerAnisotropy ? "Yes" : "No") << "\n";
        
            // get the family
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            //std::cout << "family: " << queueFamilyCount << "\n";
        
            // here to find the extentsions
            uint32_t extCount = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
            //std::cout << "get the device extencion: " << extCount << "\n";
        
            //std::cout << "........\n";
        }
        

        for (const auto& device : devices) {
            VkPhysicalDeviceProperties2 deviceProperties2{};
            deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vkGetPhysicalDeviceProperties2(device, &deviceProperties2);

            physicalDevice = device;

            //if (deviceProperties2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            //    && isDeviceSuitable(device, deviceExtensions, surface)) {

                //physicalDevice = device;
                //break;
            //}
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            for (const auto& device : devices) {
                if (isDeviceSuitable(device, deviceExtensions, surface)) {
                    physicalDevice = device;
                    break;
                }
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, QueueFamilyIndices& queueFamilies
        , const std::vector<const char*>& validationLayers, const std::vector<const char*>& deviceExtensions
        , VkDevice& device, VkQueue& graphicsQueue, VkQueue& presentQueue) {

        queueFamilies = findQueueFamilies(physicalDevice, surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { queueFamilies.graphicsFamily.value(), queueFamilies.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        volkLoadDevice(device);

        vkGetDeviceQueue(device, queueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilies.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice device, SwapChain& swapChain) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain.m_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain.m_swapChain, &imageCount, nullptr);
        swapChain.m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain.m_swapChain, &imageCount, swapChain.m_swapChainImages.data());

        swapChain.m_swapChainImageFormat = surfaceFormat.format;
        swapChain.m_swapChainExtent = extent;
    }

    void createImageViews(VkDevice device, SwapChain& swapChain) {
        swapChain.m_swapChainImageViews.resize(swapChain.m_swapChainImages.size());

        for (uint32_t i = 0; i < swapChain.m_swapChainImages.size(); i++) {
            swapChain.m_swapChainImageViews[i] = createImageView(device, swapChain.m_swapChainImages[i]
                , swapChain.m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void createRenderPass(VkPhysicalDevice physicalDevice, VkDevice device, SwapChain& swapChain, VkRenderPass& renderPass) {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChain.m_swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat(physicalDevice);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout) {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createGraphicsPipeline(VkDevice device, VkRenderPass renderPass
        , VkDescriptorSetLayout descriptorSetLayout, Pipeline& graphicsPipeline, bool fill_or_line) {

        auto vertShaderCode = readFile("shaders/vert_phong.spv");
        auto fragShaderCode = readFile("shaders/frag_phong.spv");

        VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;

        //change this maybe??
        //so if true is fill if false is line and draws only lines
        if (fill_or_line){
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

        }else{
            rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        }
        //rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        


        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE; // VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipeline.m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = graphicsPipeline.m_pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline.m_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFramebuffers(VkDevice device, SwapChain& swapChain, DepthImage& depthImage, VkRenderPass renderPass) {
        swapChain.m_swapChainFramebuffers.resize(swapChain.m_swapChainImageViews.size());

        for (size_t i = 0; i < swapChain.m_swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                swapChain.m_swapChainImageViews[i],
                depthImage.m_depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChain.m_swapChainExtent.width;
            framebufferInfo.height = swapChain.m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChain.m_swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool& commandPool) {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }

    void createDepthResources(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, SwapChain& swapChain, DepthImage& depthImage) {
        VkFormat depthFormat = findDepthFormat(physicalDevice);

        createImage(physicalDevice, device, vmaAllocator, swapChain.m_swapChainExtent.width
            , swapChain.m_swapChainExtent.height, depthFormat
            , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage.m_depthImage, depthImage.m_depthImageAllocation);
        depthImage.m_depthImageView = createImageView(device, depthImage.m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates
        , VkImageTiling tiling, VkFormatFeatureFlags features) {

        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
        return findSupportedFormat(physicalDevice,
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }


    void MemCopy(VkDevice device, void* source, VmaAllocationInfo& allocInfo, VkDeviceSize size) {
        memcpy(allocInfo.pMappedData, source, size);
    }


  void createTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkQueue graphicsQueue, VkCommandPool commandPool, const std::string& TEXTURE_PATH, Texture& texture) {

        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VmaAllocation stagingBufferAllocation;
        VmaAllocationInfo allocInfo;
        createBuffer(physicalDevice, device, vmaAllocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
            , stagingBuffer, stagingBufferAllocation, &allocInfo);

        MemCopy(device, pixels, allocInfo, imageSize);

        stbi_image_free(pixels);

        createImage(physicalDevice, device, vmaAllocator, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.m_textureImage, texture.m_textureImageAllocation);

        transitionImageLayout(device, graphicsQueue, commandPool, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(device, graphicsQueue, commandPool, stagingBuffer, texture.m_textureImage
                , static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(device, graphicsQueue, commandPool, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }

    void createTextureImageView(VkDevice device, Texture& texture) {
        texture.m_textureImageView = createImageView(device, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
                                        , VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device, Texture &texture) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &texture.m_textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }


    void createImage(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, uint32_t width, uint32_t height
        , VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties
        , VkImage& image, VmaAllocation& imageAllocation) {
        
        findMemoryType(physicalDevice, 0xFFFFFFFF, properties);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocInfo.priority = 1.0f;
        vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr);
    }

    void destroyImage(VkDevice device, VmaAllocator vmaAllocator, VkImage image, VmaAllocation& imageAllocation) {
        vmaDestroyImage(vmaAllocator, image, imageAllocation);
    }


    void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool
        , VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
    }

    void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool
        , VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
    }
/*
    void loadModel( Geometry& geometry, const std::string& MODEL_PATH) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(geometry.m_vertices.size());
                    geometry.m_vertices.push_back(vertex);
                }

                geometry.m_indices.push_back(uniqueVertices[vertex]);
            }
        }
    } */

    void loadModel_no_index(Geometry& geometry, const std::string& MODEL_PATH){
        tinyobj::attrib_t              attrib;
        std::vector<tinyobj::shape_t>  shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
            throw std::runtime_error(warn + err);

        
        for (const auto& shape : shapes) {
            for (const auto& idx : shape.mesh.indices) {
                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                vertex.normal = {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    1.f - attrib.texcoords[2 * idx.texcoord_index + 1]
                };

                //vertex.color = {1.f, 1.f, 1.f};           
                geometry.m_vertices.emplace_back(vertex);
            }
        }
    }


    void createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkQueue graphicsQueue, VkCommandPool commandPool, Geometry& geometry) {

        VkDeviceSize bufferSize = sizeof(geometry.m_vertices[0]) * geometry.m_vertices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        VmaAllocationInfo allocInfo;
        createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
            , stagingBuffer, stagingBufferAllocation, &allocInfo);

        MemCopy(device, geometry.m_vertices.data(), allocInfo, bufferSize);

        createBuffer(physicalDevice, device, vmaAllocator, bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, geometry.m_vertexBuffer
            , geometry.m_vertexBufferAllocation);

        copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, geometry.m_vertexBuffer, bufferSize);

        destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }
    /*
    void createIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkQueue graphicsQueue, VkCommandPool commandPool, Geometry& geometry) {

        VkDeviceSize bufferSize = sizeof(geometry.m_indices[0]) * geometry.m_indices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        VmaAllocationInfo allocInfo;
        createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
            , stagingBuffer, stagingBufferAllocation, &allocInfo);

        MemCopy(device, geometry.m_indices.data(), allocInfo, bufferSize);

        createBuffer(physicalDevice, device, vmaAllocator, bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0
            , geometry.m_indexBuffer, geometry.m_indexBufferAllocation);

        copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, geometry.m_indexBuffer, bufferSize);

        destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);
    } */

    void createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& vmaAllocator
            , UniformBuffers &uniformBuffers) {

        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffers.m_uniformBuffersAllocation.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffers.m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VmaAllocationInfo allocInfo;
            createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
                , uniformBuffers.m_uniformBuffers[i]
                , uniformBuffers.m_uniformBuffersAllocation[i], &allocInfo);

            uniformBuffers.m_uniformBuffersMapped[i] = allocInfo.pMappedData;
        }
    }

    void createDescriptorPool(VkDevice device, VkDescriptorPool& descriptorPool) {

        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VkDescriptorPool imguiPool;
        vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);
    }

    void createDescriptorSets(VkDevice device, Texture& texture
        , VkDescriptorSetLayout descriptorSetLayout, UniformBuffers& uniformBuffers, VkDescriptorPool descriptorPool
        , std::vector<VkDescriptorSet>& descriptorSets) {

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers.m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.m_textureImageView;
            imageInfo.sampler = texture.m_textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }


    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties
        , VmaAllocationCreateFlags vmaFlags, VkBuffer& buffer
        , VmaAllocation& allocation, VmaAllocationInfo* allocationInfo = nullptr) {
        
        findMemoryType(physicalDevice, 0xFFFFFFFF, properties);

        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = vmaFlags;
        vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, allocationInfo);
    }

    void destroyBuffer(VkDevice device, VmaAllocator vmaAllocator, VkBuffer buffer, VmaAllocation& allocation) {

        vmaDestroyBuffer(vmaAllocator, buffer, allocation);
    }


    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer
        , VkBuffer dstBuffer, VkDeviceSize size) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
    }


    //here is definitly where i get all the info about the memory
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        std::cout << "the number of memoery types is: "<<std::endl;
        std::cout<< memProperties.memoryTypeCount << std::endl;

        for (int i = 0; i < memProperties.memoryTypeCount; i++) {


            std::cout <<" heap: " << memProperties.memoryTypes[i].heapIndex<< " flag is : " << memProperties.memoryTypes[i].propertyFlags << std::endl;
        }
        std::cout << "total heap typology: ";
        std::cout << memProperties.memoryHeapCount << std::endl;
        for (int i = 0; i < memProperties.memoryHeapCount; i++) {
            std::cout << "heap indexs: " << i << " heap flags: " << memProperties.memoryHeaps[i].flags<<std::endl;

        }


        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createCommandBuffers(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers) {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex
        , SwapChain& swapChain, VkRenderPass renderPass, Pipeline& graphicsPipeline
        , std::vector<Object>& objects //Geometry& geometry, std::vector<VkDescriptorSet>& descriptorSets
		, uint32_t currentFrame) {

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChain.m_swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain.m_swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.4f, 0.5f, 6.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            //i just need to move this inside the loop
            //vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.m_pipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float) swapChain.m_swapChainExtent.width;
            viewport.height = (float) swapChain.m_swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapChain.m_swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			for( auto& object : objects ) {
	            VkBuffer vertexBuffers[] = {object.m_geometry.m_vertexBuffer};
	            VkDeviceSize offsets[] = {0};

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.m_pipeline_o);
                
	            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	            //vkCmdBindIndexBuffer(commandBuffer, object.m_geometry.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	            //vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.m_pipelineLayout
	             //   , 0, 1, &object.m_descriptorSets[currentFrame], 0, nullptr);
                
                 //i'm gonna change also this one because it still on the old pipeline layout so i just pass object.m_pipelineLayout_o
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.m_pipelineLayout_o
	                , 0, 1, &object.m_descriptorSets[currentFrame], 0, nullptr);    

	            //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.m_geometry.m_indices.size()), 1, 0, 0, 0);
                vkCmdDraw(commandBuffer, static_cast<uint32_t>(object.m_geometry.m_vertices.size()), 1, 0, 0); 
                
			}

            //----------------------------------------------------------------------------------
            ImGui::Render();

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            //----------------------------------------------------------------------------------


        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects(VkDevice device, SyncObjects& syncObjects) {
        syncObjects.m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        syncObjects.m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        syncObjects.m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects.m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &syncObjects.m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &syncObjects.m_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }


    glm::vec3 getPosition(const Object& obj){
        return glm::vec3(obj.m_ubo.model * glm::vec4(0,0,0,1));
    }

    void setPosition(Object& obj, const glm::vec3& p){
        obj.m_ubo.model[3] = glm::vec4(p, 1.0f);
    }

    
    int nextRandom_world() {
        return rand() % 151 - 75;
    }

    int nextRandomBombHeight_world(){ //generate an height for the bomb
            return rand()% 20 + 9;
        }

    int contatore = 1000;



    //all the positions values 

    
    glm::vec3 balloon_pos = glm::vec3(0.0f,0.0f,0.0f);
    glm::vec3 easter_pos = glm::vec3(0.0f,0.0f,0.0f);
    glm::vec3 tree_pos = glm::vec3(0.0f,0.0f,0.0f);
    glm::vec3 bomb_pos = glm::vec3(0.0f,0.0f,0.0f);
    glm::vec3 camera_pos = glm::vec3(0.0f,0.0f,0.0f);
    float speed_multiplier = 1.0f;
    bool speed_boost_active = false;
    std::chrono::high_resolution_clock::time_point speed_boost_start;

    int pasqua_left = 3; //how many pasquas should i collect  

    bool game_stop = false;

    int time_left = 120;
    bool hitted = false;

    bool is_protected_by_a_tree = false;


    void updateUniformBuffer(uint32_t currentImage, SwapChain& swapChain, std::vector<Object>& objects ) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		startTime = currentTime;
        static float second_accumolo = 0.0f;

        if(!game_stop && time_left > 0){
            second_accumolo = second_accumolo +dt;
            if(second_accumolo >= 1.0f){ //here is when the entire second pass
                time_left--;
                second_accumolo = second_accumolo - 1.0f;
                if(time_left <=0){
                    time_left = 0;
                    
                }

            }

        }
            

        if(game_stop){
            dt = 0.0;
        }


        if (speed_boost_active) {
            auto now = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(now - speed_boost_start).count();

            if (elapsed > 4.0f) {
                std::cout << "Boost finito\n";
                speed_multiplier = 1.0f;
                speed_boost_active = false;
            }
        }
        
		for( auto& object : objects ) {

            

            //i comment this beacuse i want it to stop rotation
            if(object.name == "present"){
                object.m_ubo.model = glm::rotate(object.m_ubo.model, dt * 0.5f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            }


	        
	        
            //glm cross is giving me the perpendicular of the front and the up, so i can compute with glm cross the diretion of the right 
            //glm::vec3 cameraleftright = glm::normalize(glm::cross(m_cameraFrontalView, m_cameraUp));
            glm::vec3 cameraDirectionFlat = glm::normalize(glm::vec3(m_cameraFrontalView.x, m_cameraFrontalView.y, 0.0f));
            glm::vec3 cameraleftright = glm::normalize(glm::cross(m_cameraUp, cameraDirectionFlat));
            
            //using the camera speed like this coold always work even without multiplying for the dt, but then for monitor with higher framerate would be 
            //too fast then i'm leaving this constant to this speed
            float cameraSpeed = 1.0f * speed_multiplier * dt;

            //this if they check acutally which key ive pressed, and then they just change the camera position, i could acutally use a switch case
            //if (whichKeyPressed[SDLK_w]) m_cameraPosition +=cameraSpeed * m_cameraFrontalView;


            //if (whichKeyPressed[SDLK_s]) m_cameraPosition -=cameraSpeed * m_cameraFrontalView;

            //if (whichKeyPressed[SDLK_a]) m_cameraPosition -=cameraSpeed * cameraleftright;
            //if (whichKeyPressed[SDLK_d]) m_cameraPosition +=cameraSpeed * cameraleftright;

            if (whichKeyPressed[SDLK_w]) m_cameraPosition += cameraSpeed * cameraDirectionFlat;
            if (whichKeyPressed[SDLK_s]) m_cameraPosition -= cameraSpeed * cameraDirectionFlat;
            if (whichKeyPressed[SDLK_d]) m_cameraPosition -= cameraSpeed * cameraleftright;
            if (whichKeyPressed[SDLK_a]) m_cameraPosition += cameraSpeed * cameraleftright;

            //manage the camera pitch and yaw witht the keyboard key arrow
            if (whichKeyPressed[SDLK_UP])  thecameraPitch +=  dt * 20.0f;
            if (whichKeyPressed[SDLK_DOWN])  thecameraPitch -= dt * 20.0f;
            if (whichKeyPressed[SDLK_LEFT])  thecameraYaw +=  dt * 20.0f;
            if (whichKeyPressed[SDLK_RIGHT]) thecameraYaw -=  dt * 20.0f;

            // this actually helps to not flip over the top 
            thecameraPitch = std::clamp(thecameraPitch, -10.0f, 30.0f);

            // here i calculate the camera front dircetion vector 
            glm::vec3 front;
            //first the x
            front.x = cos(glm::radians(thecameraYaw)) * cos(glm::radians(thecameraPitch));
            //y
            front.y = sin(glm::radians(thecameraYaw)) * cos(glm::radians(thecameraPitch));
            //and finally the z
            front.z = sin(glm::radians(thecameraPitch));
            m_cameraFrontalView = glm::normalize(front);

            //classic view matrizx with lookat of glm 
            object.m_ubo.view = glm::lookAt(m_cameraPosition, m_cameraPosition + m_cameraFrontalView, m_cameraUp);

			object.m_ubo.proj = glm::perspective(glm::radians(45.0f), swapChain.m_swapChainExtent.width / (float) swapChain.m_swapChainExtent.height, 0.1f, 500.0f);
	        object.m_ubo.proj[1][1] *= -1;

            //for the camera position i need to block it when it goes close to the end of the map wich is 100 100 0 and 100 -100 0  and -100 100 and -100 -100
            m_cameraPosition.x = std::clamp(m_cameraPosition.x, -100.0f, 100.0f);
            m_cameraPosition.y = std::clamp(m_cameraPosition.y, -100.0f, 100.0f);
            object.m_ubo.cameraPos = glm::vec4(m_cameraPosition, 1.0f);
            
            

	        memcpy(object.m_uniformBuffers.m_uniformBuffersMapped[currentImage], &object.m_ubo, sizeof(object.m_ubo));
            contatore --;
            
            if (contatore == 0){ //check the camera position in the world 
                std::cout << "posi camera: "<< glm::to_string(object.m_ubo.cameraPos) << "\n";
                contatore = 1000;
            }




            ///the game start here: 

            /// //////

            //get the objects position all the time: 
            
            

            std::cout << ".... position of balloon X. "<< bomb_pos.x <<  ".Y." << bomb_pos.y <<  ".Z.."<< bomb_pos.z <<  "...."<< "\n";


            if(object.name == "present"){
                glm::vec3 present_pos = getPosition(object);
                glm::vec3 camera_pos = glm::vec3(object.m_ubo.cameraPos);

                if(glm::distance(present_pos, camera_pos) < 2.0f){
                    std::cout << "velocità ++" << "\n";
                    speed_multiplier = 5.0f;
                    speed_boost_active = true;
                    speed_boost_start = std::chrono::high_resolution_clock::now();
                    glm::vec3 settingpos = glm::vec3(nextRandom_world(),nextRandom_world(), 0.0f);
                    setPosition(object, settingpos);
                }

            } 

            if (object.name == "easter"){
                glm::vec3 easter_pos = getPosition(object);
                glm::vec3 camera_pos = glm::vec3(object.m_ubo.cameraPos);

                if(glm::distance(easter_pos, camera_pos) < 2.0f){

                    pasqua_left --;
                    glm::vec3 settingpos = glm::vec3(nextRandom_world(),nextRandom_world(), 0.0f);
                    setPosition(object, settingpos);
                }


            }

            if (object.name == "bomb_time") {
                float offset = dt * 5.0f; 
                //object.m_ubo.model =  glm::translate(object.m_ubo.model, glm::vec3(0.0f, offset * 0.5f, -offset));
                glm::vec3 newPos = getPosition(object) + glm::vec3(0.0f, -offset, -offset);
                setPosition(object, newPos);
                //glm::translate(glm::mat4(1.0f), glm::vec3(offset, 0.0f, 0.0f))
                //glm::rotate(object.m_ubo.model, dt * 1.0f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
                //glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.02f, 0.02f)), glm::radians(90.0f), glm::vec3(1.0f, 2.5f, 2.0f)), glm::vec3(50.0f, 120.0f, 200.0f))
                if(newPos.z < 0.0f){
                    //std::cout << ".... position of bomb X. "<< newPos.x <<  ".Y." << newPos.y <<  ".Z.."<< newPos.z <<  "...."<< "\n";
                    //glm::vec3 settingpos = getPosition(object) + glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    glm::vec3 settingpos = glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    setPosition(object, settingpos);

                }
                glm::vec3 cameraPos3 = glm::vec3(object.m_ubo.cameraPos);
                if(glm::distance(newPos, cameraPos3) < 3.5f){
                    std::cout << "meno time"<< "\n";
                    time_left = time_left - 10;
                    glm::vec3 settingpos = glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    setPosition(object, settingpos);
                    
                }
                
                   
            }

            if (object.name == "tree"){
                glm::vec3 easter_pos = getPosition(object);
                glm::vec3 camera_pos = glm::vec3(object.m_ubo.cameraPos);

                if(glm::distance(easter_pos, camera_pos) < 2.0f){

                    is_protected_by_a_tree = true;
                    
                }else{

                    is_protected_by_a_tree = false;
                }


            }


            



                








            if (object.name == "balloon") {
                float offset = dt * 2.0f; 
                //object.m_ubo.model =  glm::translate(object.m_ubo.model, glm::vec3(0.0f, offset * 0.5f, -offset));
                glm::vec3 newPos = getPosition(object) + glm::vec3(0.0f, -offset, -offset);
                setPosition(object, newPos);
                //glm::translate(glm::mat4(1.0f), glm::vec3(offset, 0.0f, 0.0f))
                //glm::rotate(object.m_ubo.model, dt * 1.0f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
                //glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.02f, 0.02f)), glm::radians(90.0f), glm::vec3(1.0f, 2.5f, 2.0f)), glm::vec3(50.0f, 120.0f, 200.0f))
                if(newPos.z < 0.0f){
                    //std::cout << ".... position of balloon X. "<< newPos.x <<  ".Y." << newPos.y <<  ".Z.."<< newPos.z <<  "...."<< "\n";
                    //glm::vec3 settingpos = getPosition(object) + glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    glm::vec3 settingpos = glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    setPosition(object, settingpos);

                }
                glm::vec3 cameraPos3 = glm::vec3(object.m_ubo.cameraPos);
                if(glm::distance(newPos, cameraPos3) < 3.0f){
                    std::cout << "regalo di tempo"<< "\n";
                    time_left = time_left + 10; //add time if hitted by a balloon
                    glm::vec3 settingpos = glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    setPosition(object, settingpos);
                    
                }
            
                
                   
            }











           
            //here i manouver the bombs//




            if (object.name == "bomb_bad") {
                float offset = dt * 5.0f; 
                //object.m_ubo.model =  glm::translate(object.m_ubo.model, glm::vec3(0.0f, offset * 0.5f, -offset));
                glm::vec3 newPos = getPosition(object) + glm::vec3(0.0f, -offset, -offset);
                setPosition(object, newPos);
                //glm::translate(glm::mat4(1.0f), glm::vec3(offset, 0.0f, 0.0f))
                //glm::rotate(object.m_ubo.model, dt * 1.0f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
                //glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.02f, 0.02f)), glm::radians(90.0f), glm::vec3(1.0f, 2.5f, 2.0f)), glm::vec3(50.0f, 120.0f, 200.0f))
                if(newPos.z < 0.0f){
                    //std::cout << ".... position of bomb X. "<< newPos.x <<  ".Y." << newPos.y <<  ".Z.."<< newPos.z <<  "...."<< "\n";
                    //glm::vec3 settingpos = getPosition(object) + glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    glm::vec3 settingpos = glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    setPosition(object, settingpos);

                }
                glm::vec3 cameraPos3 = glm::vec3(object.m_ubo.cameraPos);
                if(glm::distance(newPos, cameraPos3) < 3.0f ){
                    std::cout << "Beccatoooooaaaaaaaaa"<< "\n";
                    
                    glm::vec3 settingpos = glm::vec3(nextRandom_world(),nextRandom_world(), nextRandomBombHeight_world());
                    setPosition(object, settingpos);
                    hitted = true;
                    
                }
                
                   
            }



		}
        auto seconds_timer = std::chrono::high_resolution_clock::now();
    }

    void drawFrame(SDL_Window* window, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice
        , VkDevice device, VmaAllocator vmaAllocator, VkQueue graphicsQueue, VkQueue presentQueue
        , SwapChain& swapChain, DepthImage& depthImage, VkRenderPass renderPass, Pipeline& graphicsPipeline
		, std::vector<Object>& objects, std::vector<VkCommandBuffer>& commandBuffers
        , SyncObjects& syncObjects, uint32_t& currentFrame, bool& framebufferResized) {

        vkWaitForFences(device, 1, &syncObjects.m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain.m_swapChain, UINT64_MAX
                            , syncObjects.m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ) {
            recreateSwapChain(window, surface, physicalDevice, device, vmaAllocator, swapChain, depthImage, renderPass);
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        
        updateUniformBuffer(currentFrame, swapChain, objects);
        
        

        vkResetFences(device, 1, &syncObjects.m_inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame],  0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex, swapChain, renderPass, graphicsPipeline, objects, currentFrame);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {syncObjects.m_imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {syncObjects.m_renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, syncObjects.m_inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain.m_swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain(window, surface, physicalDevice, device, vmaAllocator, swapChain, depthImage, renderPass);
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        //return VK_PRESENT_MODE_FIFO_KHR;
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            SDL_GetWindowSize(m_sdlWindow, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width
                , capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height
                , capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions, VkSurfaceKHR surface) {
        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate  && supportedFeatures.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t extensions_count = 0;
        std::vector<const char*> extensions;
        SDL_Vulkan_GetInstanceExtensions(m_sdlWindow, &extensions_count, nullptr);
        extensions.resize(extensions_count);
        SDL_Vulkan_GetInstanceExtensions(m_sdlWindow, &extensions_count, extensions.data());
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        #ifdef __APPLE__
        extensions.push_back("VK_MVK_macos_surface");
        // The next line is only required when using API_VERSION_1_0
        // enabledInstanceExtensions.push_back("VK_KHR_get_physical_device_properties2");
        extensions.push_back("VK_KHR_portability_enumeration");
        #endif

        return extensions;
    }

    bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
        , VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
        , void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }


    //------------------------------------------------------------------------


    void setupImgui(VkInstance instance, VkPhysicalDevice physicalDevice, QueueFamilyIndices queueFamilies
        , VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorPool descriptorPool
        , VkRenderPass renderPass) {

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

        ImGui_ImplVulkan_LoadFunctions( VK_API_VERSION_1_0, &loadVolk );

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForVulkan(m_sdlWindow);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        init_info.QueueFamily = queueFamilies.graphicsFamily.value();
        init_info.Queue = graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = descriptorPool;
        init_info.RenderPass = renderPass;
        init_info.Subpass = 0;
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info);
        // (this gets a bit more complicated, see example app for full reference)
        //ImGui_ImplVulkan_CreateFontsTexture(YOUR_COMMAND_BUFFER);
        // (your code submit a queue)
        //ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
};




int main() {
    HelloTriangleApplication app;
    std::filesystem::current_path();

    app.run();


    return EXIT_SUCCESS;
}
