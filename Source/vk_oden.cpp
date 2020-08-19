/*
 *
 * Copyright (c) 2020 gyabo <gyaboyan@gmail.com>
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#include "ODEN.h"

#include <stdio.h>
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

#include <map>
#include <vector>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "advapi32.lib")

#pragma comment(lib, "vulkan-1.lib")

#define LOG_INFO(...) printf("INFO : " __FUNCTION__ ":" __VA_ARGS__)
#define LOG_ERR(...) printf("ERR : " __FUNCTION__ ":" __VA_ARGS__)

using namespace oden;

static VKAPI_ATTR VkBool32
VKAPI_CALL debug_callback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT,
	uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData)
{
	printf("vkdbg: ");
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		printf("ERROR : ");
	}
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		printf("WARNING : ");
	}
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		printf("PERFORMANCE : ");
	}
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		printf("INFO : ");
	}
	printf("%s", pMessage);
	printf("\n");
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		printf("\n");
	}
	return VK_FALSE;
}

inline void
bind_debug_fn(VkInstance instance, VkDebugReportCallbackCreateInfoEXT ext)
{
	VkDebugReportCallbackEXT callback;
	PFN_vkCreateDebugReportCallbackEXT func = PFN_vkCreateDebugReportCallbackEXT(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));

	if (func) {
		func(instance, &ext, nullptr, &callback);
	} else {
		printf("PFN_vkCreateDebugReportCallbackEXT IS NULL\n");
	}
}

static VkImage
create_image(VkDevice device, uint32_t width, uint32_t height, VkFormat format,
	VkImageUsageFlags usageFlags)
{
	VkImage ret = VK_NULL_HANDLE;

	VkImageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.pNext = NULL;
	info.flags = 0;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = format;
	info.extent.width = width;
	info.extent.height = height;
	info.extent.depth = 1;
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.queueFamilyIndexCount = 0;
	info.pQueueFamilyIndices = NULL;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	vkCreateImage(device, &info, NULL, &ret);
	return (ret);
}

static VkImageView
create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectMask)
{
	VkImageView ret = VK_NULL_HANDLE;

	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = NULL;
	info.flags = 0;
	info.image = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = format;
	info.components.r = VK_COMPONENT_SWIZZLE_R;
	info.components.g = VK_COMPONENT_SWIZZLE_G;
	info.components.b = VK_COMPONENT_SWIZZLE_B;
	info.components.a = VK_COMPONENT_SWIZZLE_A;

	info.subresourceRange.aspectMask = aspectMask;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;

	vkCreateImageView(device, &info, NULL, &ret);
	return (ret);
}

static VkBuffer
create_buffer(VkDevice device, VkDeviceSize size)
{
	VkBuffer ret = VK_NULL_HANDLE;

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size  = size;
	info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkCreateBuffer(device, &info, nullptr, &ret);
	return (ret);
}

static VkImageMemoryBarrier
get_barrier(VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout old_image_layout,
	VkImageLayout new_image_layout,
	VkAccessFlags srcAccessMask,
	VkPipelineStageFlags src_stages,
	VkPipelineStageFlags dest_stages)
{
	VkImageMemoryBarrier ret = {};
	ret.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ret.pNext = NULL;
	ret.srcAccessMask = (VkAccessFlags)srcAccessMask;
	ret.dstAccessMask = 0;
	ret.oldLayout = old_image_layout;
	ret.newLayout = new_image_layout;
	ret.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ret.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ret.image = image;
	ret.subresourceRange = { aspectMask, 0, 1, 0, 1 };

	switch (new_image_layout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		/* Make sure anything that was copying from this image has completed */
		ret.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		ret.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		ret.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		ret.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		ret.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		ret.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		break;

	default:
		ret.dstAccessMask = 0;
		break;
	}

	return (ret);
}

static VkRenderPass
create_renderpass(
	VkDevice device,
	uint32_t color_num,
	bool is_presentable,
	VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM,
	VkFormat depth_format = VK_FORMAT_D32_SFLOAT)
{
	VkRenderPass ret = VK_NULL_HANDLE;

	auto finalLayout = is_presentable ?
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	//VK_IMAGE_LAYOUT_GENERAL
	//VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV

	int attachment_index = 0;
	std::vector<VkAttachmentDescription> vattachments;
	std::vector<VkAttachmentReference> vattachment_refs;
	std::vector<VkSubpassDependency> vsubpassdepends;
	VkAttachmentDescription color_attachment = {};

	color_attachment.flags = 0;
	color_attachment.format = color_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	//color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = finalLayout;

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.flags = 0;
	depth_attachment.format = depth_format;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	//depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_reference = {};
	color_reference.attachment = 0;
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment = 1;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDependency color_subpass_dependency = {};
	VkSubpassDependency depth_subpass_dependency = {};
	color_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	color_subpass_dependency.dstSubpass = 0;
	color_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	color_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	color_subpass_dependency.srcAccessMask = 0;
	color_subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	color_subpass_dependency.dependencyFlags = 0;

	depth_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depth_subpass_dependency.dstSubpass = 0;
	depth_subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_subpass_dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depth_subpass_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depth_subpass_dependency.dependencyFlags = 0;

	//gather
	for (int i = 0 ; i < color_num; i++) {
		auto ref = color_reference;
		ref.attachment = attachment_index;
		vattachments.push_back(color_attachment);
		vattachment_refs.push_back(ref);
		vsubpassdepends.push_back(color_subpass_dependency);
		attachment_index++;
	}
	vattachments.push_back(depth_attachment);
	vsubpassdepends.push_back(depth_subpass_dependency);
	depth_reference.attachment = attachment_index;

	VkSubpassDescription subpass = {};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = (uint32_t)vattachment_refs.size();
	subpass.pColorAttachments = vattachment_refs.data();
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depth_reference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;


	VkRenderPassCreateInfo rp_info = {};
	rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rp_info.pNext = NULL;
	rp_info.flags = 0;
	rp_info.attachmentCount = (uint32_t)vattachments.size();
	rp_info.pAttachments = vattachments.data();
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &subpass;
	/*
	rp_info.dependencyCount = 0;
	rp_info.pDependencies = nullptr;
	*/
	rp_info.dependencyCount = (uint32_t)vsubpassdepends.size();
	rp_info.pDependencies = vsubpassdepends.data();

	auto err = vkCreateRenderPass(device, &rp_info, NULL, &ret);

	return (ret);
}

static VkFramebuffer
create_framebuffer(
	VkDevice device,
	VkRenderPass renderpass,
	std::vector<VkImageView> vimageview,
	uint32_t width,
	uint32_t height,
	VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM,
	VkFormat depth_format = VK_FORMAT_D32_SFLOAT)
{
	VkFramebuffer fb = nullptr;
	VkFramebufferCreateInfo fb_info = {};

	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = NULL;
	fb_info.renderPass = renderpass;
	fb_info.attachmentCount = (uint32_t)vimageview.size();
	fb_info.pAttachments = vimageview.data();
	fb_info.width = width;
	fb_info.height = height;
	fb_info.layers = 1;
	auto err = vkCreateFramebuffer(device, &fb_info, NULL, &fb);

	return (fb);
}
void
oden::oden_present_graphics(
	const char * appname, std::vector<cmd> & vcmd,
	void *handle, uint32_t w, uint32_t h,
	uint32_t count, uint32_t heapcount, uint32_t slotmax)
{
	HWND hwnd = (HWND) handle;

	enum {
		RDT_SLOT_SRV = 0,
		RDT_SLOT_CBV,
		RDT_SLOT_UAV,
		RDT_SLOT_MAX,
	};

	struct DeviceBuffer {
		uint64_t value = 0;
		VkCommandBuffer cmdbuf = VK_NULL_HANDLE;
		VkFence fence = VK_NULL_HANDLE;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;

		std::vector<VkBuffer> vscratch_buffers;
		std::vector<VkDeviceMemory> vscratch_devmems;
	};

	static VkInstance inst = VK_NULL_HANDLE;
	static VkPhysicalDevice gpudev = VK_NULL_HANDLE;
	static VkDevice device = VK_NULL_HANDLE;
	static VkQueue graphics_queue = VK_NULL_HANDLE;
	static VkSurfaceKHR surface = VK_NULL_HANDLE;
	static VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	static VkCommandPool cmd_pool = VK_NULL_HANDLE;
	static VkSampler sampler_nearest = VK_NULL_HANDLE;
	static VkSampler sampler_linear = VK_NULL_HANDLE;
	static VkDescriptorPool desc_pool = VK_NULL_HANDLE;
	static VkDescriptorSetLayout layout = VK_NULL_HANDLE;
	static VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	static VkPhysicalDeviceMemoryProperties devicememoryprop = {};

	static std::map<std::string, VkRenderPass> mrenderpasses;
	static std::map<std::string, VkFramebuffer> mframebuffers;

	static std::map<std::string, VkImage> mimages;
	static std::map<std::string, VkImageView> mimageviews;
	static std::map<std::string, VkClearValue> mclearvalues;

	static std::map<std::string, VkBuffer> mbuffers;
	static std::map<std::string, VkMemoryRequirements> mmemreqs;
	static std::map<std::string, VkDeviceMemory> mdevmem;
	static std::map<std::string, VkDescriptorSet> mdescriptor_sets;
	static std::map<std::string, VkPipeline> mgraphics_pipelines;

	static uint32_t backbuffer_index = 0;
	static uint64_t frame_count = 0;

	static std::vector<DeviceBuffer> devicebuffer;

	auto alloc_devmem = [&](auto name, VkDeviceSize size, VkMemoryPropertyFlags flags, bool is_entry = true) {
		if (mdevmem.count(name) != 0)
			LOG_INFO("already allocated name=%s\n", name.c_str());

		VkMemoryAllocateInfo ma_info = {};
		ma_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ma_info.pNext = nullptr;
		ma_info.allocationSize = size;
		ma_info.memoryTypeIndex = 0;

		for (uint32_t i = 0; i < devicememoryprop.memoryTypeCount; i++) {
			if ((devicememoryprop.memoryTypes[i].propertyFlags & flags) == flags) {
				ma_info.memoryTypeIndex = i;
				break;
			}
		}
		VkDeviceMemory devmem = nullptr;
		vkAllocateMemory(device, &ma_info, nullptr, &devmem);
		if (is_entry) {
			if (devmem)
				mdevmem[name] = devmem;
			else
				LOG_ERR("Can't alloc name=%s\n", name.c_str());
		}
		return devmem;
	};

	if (inst == nullptr) {
		uint32_t inst_ext_cnt = 0;
		uint32_t gpu_count = 0;
		uint32_t device_extension_count = 0;
		uint32_t queue_family_count = 0;
		std::vector<const char *> vinstance_ext_names;
		std::vector<const char *> ext_names;

		//Is supported vk?
		vkEnumerateInstanceExtensionProperties(NULL, &inst_ext_cnt, NULL);
		std::vector<VkExtensionProperties> vinstance_ext(inst_ext_cnt);
		vkEnumerateInstanceExtensionProperties(NULL, &inst_ext_cnt, vinstance_ext.data());
		for (auto x : vinstance_ext) {
			auto name = std::string(x.extensionName);
			if (name == VK_KHR_SURFACE_EXTENSION_NAME)
				vinstance_ext_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
			if (name == VK_KHR_WIN32_SURFACE_EXTENSION_NAME)
				vinstance_ext_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			LOG_INFO("vkEnumerateInstanceExtensionProperties : name=%s\n", name.c_str());
		}
		VkDebugReportCallbackCreateInfoEXT drcc_info = {};
		drcc_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		drcc_info.flags = 0;
		drcc_info.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
		drcc_info.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
		drcc_info.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		drcc_info.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
		drcc_info.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
		drcc_info.pfnCallback = &debug_callback;

		//Create vk instances
		VkApplicationInfo vkapp = {};
		vkapp.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vkapp.pNext = &drcc_info;
		vkapp.pApplicationName = appname;
		vkapp.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		vkapp.pEngineName = appname;
		vkapp.engineVersion = 0;
		vkapp.apiVersion = VK_API_VERSION_1_0;

		//DEBUG
		vinstance_ext_names.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		static const char *debuglayers[] = {
			//"VK_LAYER_LUNARG_standard_validation",
			"VK_LAYER_KHRONOS_validation",
		};
		VkInstanceCreateInfo inst_info = {};
		inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		inst_info.pNext = NULL;
		inst_info.pApplicationInfo = &vkapp;
		inst_info.enabledLayerCount = _countof(debuglayers);
		inst_info.ppEnabledLayerNames = debuglayers;
		inst_info.enabledExtensionCount = (uint32_t)vinstance_ext_names.size();
		inst_info.ppEnabledExtensionNames = (const char *const *)vinstance_ext_names.data();
		auto err = vkCreateInstance(&inst_info, NULL, &inst);

		bind_debug_fn(inst, drcc_info);

		//Enumaration GPU's
		err = vkEnumeratePhysicalDevices(inst, &gpu_count, NULL);
		LOG_INFO("gpu_count=%d\n", gpu_count);
		if (gpu_count != 1) {
			LOG_ERR("---------------------------------------------\n");
			LOG_ERR("muri\n");
			LOG_ERR("---------------------------------------------\n");
			exit(1);
		}
		err = vkEnumeratePhysicalDevices(inst, &gpu_count, &gpudev);
		err = vkEnumerateDeviceExtensionProperties(gpudev, NULL, &device_extension_count, NULL);
		std::vector<VkExtensionProperties> vdevice_extensions(device_extension_count);
		err = vkEnumerateDeviceExtensionProperties(gpudev, NULL, &device_extension_count, vdevice_extensions.data());
		LOG_INFO("vkEnumerateDeviceExtensionProperties : device_extension_count = %lu, VK_KHR_SWAPCHAIN_EXTENSION_NAME=%s\n", vdevice_extensions.size(), VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		for (auto x : vdevice_extensions) {
			auto name = std::string(x.extensionName);
			if (name == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
				ext_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			LOG_INFO("vkEnumerateDeviceExtensionProperties : extensionName=%s\n", x.extensionName);
		}

		//Enumeration Queue attributes.
		VkPhysicalDeviceProperties gpu_props = {};
		vkGetPhysicalDeviceProperties(gpudev, &gpu_props);
		vkGetPhysicalDeviceQueueFamilyProperties(gpudev, &queue_family_count, NULL);
		LOG_INFO("vkGetPhysicalDeviceQueueFamilyProperties : queue_family_count=%d\n", queue_family_count);
		std::vector<VkQueueFamilyProperties> vqueue_props(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(gpudev, &queue_family_count, vqueue_props.data());
		VkPhysicalDeviceFeatures physDevFeatures;
		vkGetPhysicalDeviceFeatures(gpudev, &physDevFeatures);
		uint32_t graphics_queue_family_index = UINT32_MAX;
		uint32_t presentQueueFamilyIndex = UINT32_MAX;
		for (uint32_t i = 0; i < queue_family_count; i++) {
			auto flags = vqueue_props[i].queueFlags;
			if (flags & VK_QUEUE_GRAPHICS_BIT) {
				LOG_INFO("index=%d : VK_QUEUE_GRAPHICS_BIT\n", i);
				if (graphics_queue_family_index == UINT32_MAX)
					graphics_queue_family_index = i;
			}

			if (flags & VK_QUEUE_COMPUTE_BIT)
				LOG_INFO("index=%d : VK_QUEUE_COMPUTE_BIT\n", i);

			if (flags & VK_QUEUE_TRANSFER_BIT)
				LOG_INFO("index=%d : VK_QUEUE_TRANSFER_BIT\n", i);

			if (flags & VK_QUEUE_SPARSE_BINDING_BIT)
				LOG_INFO("index=%d : VK_QUEUE_SPARSE_BINDING_BIT\n", i);

			if (flags & VK_QUEUE_PROTECTED_BIT)
				LOG_INFO("index=%d : VK_QUEUE_PROTECTED_BIT\n", i);
		}

		//Create Device and Queue's
		float queue_priorities[1] = {0.0};
		VkDeviceQueueCreateInfo queue_info = {};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.pNext = NULL;
		queue_info.queueFamilyIndex = graphics_queue_family_index;
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = queue_priorities;
		queue_info.flags = 0;
		VkDeviceCreateInfo device_info = {};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.pNext = NULL;
		device_info.queueCreateInfoCount = 1;
		device_info.pQueueCreateInfos = &queue_info;
		device_info.enabledLayerCount = 1;
		device_info.ppEnabledLayerNames = debuglayers;
		device_info.enabledExtensionCount = (uint32_t)ext_names.size();
		device_info.ppEnabledExtensionNames = (const char *const *)ext_names.data();
		device_info.pEnabledFeatures = NULL;
		err = vkCreateDevice(gpudev, &device_info, NULL, &device);
		vkGetPhysicalDeviceMemoryProperties(gpudev, &devicememoryprop);
		vkGetDeviceQueue(device, graphics_queue_family_index, 0, &graphics_queue);

		//Create Swapchain's
		VkWin32SurfaceCreateInfoKHR surfaceinfo = {};
		surfaceinfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceinfo.hinstance = GetModuleHandle(NULL);
		surfaceinfo.hwnd = hwnd;
		vkCreateWin32SurfaceKHR(inst, &surfaceinfo, NULL, &surface);

		//todo determine supported format from swapchain devices.
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(gpudev, 0, surface, &presentSupport);
		VkSurfaceCapabilitiesKHR capabilities = {};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpudev, surface, &capabilities);
		LOG_INFO("vkGetPhysicalDeviceSurfaceSupportKHR Done\n", __LINE__);


		VkSwapchainCreateInfoKHR sc_info = {};
		sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		sc_info.surface = surface;
		sc_info.minImageCount = count;
		sc_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM; //todo
		sc_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		sc_info.imageExtent.width = w;
		sc_info.imageExtent.height = h;
		sc_info.imageArrayLayers = 1;
		sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		sc_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		sc_info.queueFamilyIndexCount = 0; //VK_SHARING_MODE_CONCURRENT
		sc_info.pQueueFamilyIndices = nullptr;   //VK_SHARING_MODE_CONCURRENT
		//http://vulkan-spec-chunked.ahcox.com/ch29s05.html#VkSurfaceTransformFlagBitsKHR
		sc_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		sc_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		sc_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		//sc_info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		sc_info.clipped = VK_TRUE;
		sc_info.oldSwapchain = VK_NULL_HANDLE;

		err = vkCreateSwapchainKHR(device, &sc_info, nullptr, &swapchain);

		//Get BackBuffer Images
		{
			std::vector<VkImage> temp;
			uint32_t count = 0;

			vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr);
			temp.resize(count);
			vkGetSwapchainImagesKHR(device, swapchain, &count, temp.data());
			for (auto & x : temp)
				LOG_INFO("temp = %p\n", x);

			for (int i = 0 ; i < temp.size(); i++) {
				auto name_color = oden_get_backbuffer_name(i);
				mimages[name_color] = temp[i];
				VkMemoryRequirements dummy = {};
				mmemreqs[name_color] = dummy;
			}
		}

		//Create CommandBuffers
		VkCommandPoolCreateInfo cmd_pool_info = {};
		cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_info.pNext = NULL;
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmd_pool_info.queueFamilyIndex = graphics_queue_family_index;

		vkCreateCommandPool(device, &cmd_pool_info, NULL, &cmd_pool);

		//Create Frame Resources
		devicebuffer.resize(count);
		for (int i = 0 ; i < count; i++) {
			auto & ref = devicebuffer[i];
			VkFenceCreateInfo fence_ci = {};
			fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_ci.pNext = NULL;
			fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VkCommandBufferAllocateInfo cballoc_info = {};
			cballoc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cballoc_info.pNext = nullptr;
			cballoc_info.commandPool = cmd_pool;
			cballoc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cballoc_info.commandBufferCount = 1;

			err = vkAllocateCommandBuffers(device, &cballoc_info, &ref.cmdbuf);
			err = vkCreateFence(device, &fence_ci, NULL, &ref.fence);
			vkResetFences(device, 1, &ref.fence);
			LOG_INFO("backbuffer cmdbuf[%d] = %p\n", i, ref.cmdbuf);
			LOG_INFO("backbuffer fence[%d] = %p\n", i, ref.fence);
		}

		//Create General samplers
		VkSamplerCreateInfo sampler = {};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.pNext = NULL;
		sampler.magFilter = VK_FILTER_NEAREST;
		sampler.minFilter = VK_FILTER_NEAREST;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.mipLodBias = 0.0f;
		sampler.anisotropyEnable = VK_FALSE;
		sampler.maxAnisotropy = 1;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		sampler.maxLod = 0.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		sampler.unnormalizedCoordinates = VK_FALSE;

		err = vkCreateSampler(device, &sampler, NULL, &sampler_nearest);
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		err = vkCreateSampler(device, &sampler, NULL, &sampler_linear);

		//VkDescriptorPool
		//VkLayout
		//  |
		//  V
		//descrpter buffer layout pipeline shader ImageView Depth
		std::vector<VkDescriptorPoolSize> vpoolsizes;
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, heapcount});
		vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, heapcount});
		//vpoolsizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, heapcount});

		VkDescriptorPoolCreateInfo desc_pool_info = {};
		desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		desc_pool_info.pNext = nullptr;
		desc_pool_info.flags = 0;
		desc_pool_info.maxSets = 0xFFFF; //todo
		desc_pool_info.poolSizeCount = (uint32_t)vpoolsizes.size();
		desc_pool_info.pPoolSizes = vpoolsizes.data();

		vkCreateDescriptorPool(device, &desc_pool_info, nullptr, &desc_pool);

		//todo
		std::vector<VkDescriptorSetLayoutBinding> vdesc_setlayout_binding;
		for (int i = 0 ; i < slotmax; i++) {
			auto offset = i * RDT_SLOT_MAX;
			vdesc_setlayout_binding.push_back({(uint32_t)RDT_SLOT_SRV + offset, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_ALL_GRAPHICS, nullptr});
			vdesc_setlayout_binding.push_back({(uint32_t)RDT_SLOT_CBV + offset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS, nullptr});
			vdesc_setlayout_binding.push_back({(uint32_t)RDT_SLOT_UAV + offset, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1, VK_SHADER_STAGE_ALL_GRAPHICS, nullptr});
		}

		VkDescriptorSetLayoutCreateInfo desc_setlayout_info = {};
		desc_setlayout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		desc_setlayout_info.bindingCount = (uint32_t)vdesc_setlayout_binding.size();
		desc_setlayout_info.pBindings = vdesc_setlayout_binding.data();
		vkCreateDescriptorSetLayout(device, &desc_setlayout_info, nullptr, &layout);

		VkPipelineLayoutCreateInfo plc_info = {};
		plc_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		plc_info.pNext = NULL;
		plc_info.setLayoutCount = 1;
		plc_info.pSetLayouts = &layout;

		err = vkCreatePipelineLayout(device, &plc_info, NULL, &pipeline_layout);
		LOG_INFO("VkInstance inst = %p\n", inst);
		LOG_INFO("VkPhysicalDevice gpudev = %p\n", gpudev);
		LOG_INFO("VkDevice device = %p\n", device);
		LOG_INFO("VkQueue graphics_queue = %p\n", graphics_queue);
		LOG_INFO("VkSurfaceKHR surface = %p\n", surface);
		LOG_INFO("VkSwapchainKHR swapchain = %p\n", swapchain);
		LOG_INFO("vkCreateCommandPool cmd_pool = %p\n", cmd_pool);
		LOG_INFO("vkCreateDescriptorPool desc_pool = %p\n", desc_pool);
		LOG_INFO("vkCreateDescriptorSetLayout layout = %p\n", layout);
		/*
		auto renderpass_present = create_renderpass(device, 8, true);
		auto renderpass_mrt = create_renderpass(device, 8, false);
		LOG_INFO("create_renderpass = %p\n", renderpass_present);
		LOG_INFO("renderpass_mrt = %p\n", renderpass_mrt);
		*/
		LOG_INFO("vkCreatePipelineLayout = %p\n", pipeline_layout);
	}

	LOG_INFO("frame_count=%llu\n", frame_count);

	//Determine resource index.
	auto & ref = devicebuffer[backbuffer_index];



	LOG_INFO("vkWaitForFences[%d]\n", backbuffer_index);
	auto fence_status = vkGetFenceStatus(device, ref.fence);
	if (fence_status == VK_SUCCESS) {
		LOG_INFO("The fence specified by fence is signaled.\n");
		auto wait_result = vkWaitForFences(device, 1, &ref.fence, VK_TRUE, UINT64_MAX);
		LOG_INFO("vkWaitForFences[%d] Done wait_result=%d(%s)\n", backbuffer_index, wait_result, wait_result ? "NG" : "OK");
		vkResetFences(device, 1, &ref.fence);
	}

	if (fence_status == VK_NOT_READY)
		LOG_INFO("The fence specified by fence is unsignaled.\n");

	if (fence_status == VK_ERROR_DEVICE_LOST)
		LOG_INFO("The device has been lost.\n");

	//check swapchain
	/*
	auto swapchain_status = vkGetSwapchainStatusKHR(device, swapchain);
	if (swapchain_status == VK_ERROR_OUT_OF_HOST_MEMORY)
		LOG_INFO("VK_ERROR_OUT_OF_HOST_MEMORY");
	if (swapchain_status == VK_ERROR_OUT_OF_DEVICE_MEMORY)
		LOG_INFO("VK_ERROR_OUT_OF_DEVICE_MEMORY");
	if (swapchain_status == VK_ERROR_DEVICE_LOST)
		LOG_INFO("VK_ERROR_DEVICE_LOST");
	if (swapchain_status == VK_ERROR_OUT_OF_DATE_KHR)
		LOG_INFO("VK_ERROR_OUT_OF_DATE_KHR");
	if (swapchain_status == VK_ERROR_SURFACE_LOST_KHR)
		LOG_INFO("VK_ERROR_SURFACE_LOST_KHR");
	if (swapchain_status == VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
		LOG_INFO("VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT");
	*/

	uint32_t present_index = 0;
	auto acquire_result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, VK_NULL_HANDLE, ref.fence, &present_index);
	LOG_INFO("vkAcquireNextImageKHR acquire_result=%d, present_index=%d, ref.fence=%p\n",
		acquire_result, present_index, ref.fence);


	//Destroy scratch resources
	for (auto & x : ref.vscratch_buffers)
		vkDestroyBuffer(device, x, NULL);
	ref.vscratch_buffers.clear();

	for (auto & x : ref.vscratch_devmems)
		vkFreeMemory(device, x, NULL);
	ref.vscratch_devmems.clear();

	//Destroy resources
	if (hwnd == nullptr) {
	}

	//Begin
	VkCommandBufferBeginInfo cmdbegininfo = {};
	cmdbegininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdbegininfo.pNext = nullptr;
	cmdbegininfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	cmdbegininfo.pInheritanceInfo = nullptr;

	vkResetCommandBuffer(ref.cmdbuf, 0);
	vkBeginCommandBuffer(ref.cmdbuf, &cmdbegininfo);

	printf("vcmd.size=%lu\n", vcmd.size());

	struct record {
		VkDescriptorSet descriptor_sets;
		VkRenderPass renderpass;
	};
	record rec = {};

	//Proc command.
	int cmd_index = 0;
	for (auto & c : vcmd) {
		auto type = c.type;
		auto name = c.name;
		printf("cmd_index = %04d : %s\n", cmd_index++, oden_get_cmd_name(type));

		//CMD_SET_RENDER_TARGET
		if (type == CMD_SET_RENDER_TARGET) {
			if (rec.renderpass) {
				vkCmdEndRenderPass(ref.cmdbuf);
				rec.renderpass = nullptr;
			}

			//todo mrt
			auto x = c.set_render_target.rect.x;
			auto y = c.set_render_target.rect.y;
			auto w = c.set_render_target.rect.w;
			auto h = c.set_render_target.rect.h;

			bool is_backbuffer = false;
			if (oden_get_backbuffer_name(backbuffer_index) == name) {
				is_backbuffer = true;
			}

			VkViewport viewport = {};
			viewport.width = (float)w;
			viewport.height = (float)h;
			viewport.minDepth = (float)0.0f;
			viewport.maxDepth = (float)1.0f;

			VkRect2D scissor = {};
			scissor.extent.width = w;
			scissor.extent.height = h;
			scissor.offset.x = 0;
			scissor.offset.y = 0;

			vkCmdSetViewport(ref.cmdbuf, 0, 1, &viewport);
			vkCmdSetScissor(ref.cmdbuf, 0, 1, &scissor);

			//COLOR
			auto name_color = name;
			auto image_color = mimages[name_color];
			auto fmt_color = VK_FORMAT_B8G8R8A8_UNORM;
			if (image_color == nullptr) {
				image_color = create_image(device, w, h, fmt_color, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				mimages[name_color] = image_color;
				LOG_INFO("create_image name_color=%s, image_color=0x%p\n", name_color.c_str(), image_color);
			}

			//allocate color memreq and Bind
			if (mmemreqs.count(name_color) == 0) {
				VkMemoryRequirements memreqs = {};

				vkGetImageMemoryRequirements(device, image_color, &memreqs);
				memreqs.size = memreqs.size + (memreqs.alignment - 1);
				memreqs.size &= ~(memreqs.alignment - 1);
				mmemreqs[name_color] = memreqs;

				VkDeviceMemory devmem = alloc_devmem(
						name_color, memreqs.size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				vkBindImageMemory(device, image_color, devmem, 0);
			}

			//COLOR VIEW
			auto imageview_color = mimageviews[name_color];
			if (imageview_color == nullptr) {
				LOG_INFO("create_image_view name=%s\n", name_color.c_str());
				imageview_color = create_image_view(device, image_color, fmt_color, VK_IMAGE_ASPECT_COLOR_BIT);
				mimageviews[name_color] = imageview_color;
				LOG_INFO("create_image_view imageview_color=0x%p\n", imageview_color);
			}

			//DEPTH
			auto name_depth = oden_get_depth_render_target_name(name);
			auto image_depth = mimages[name_depth];
			auto fmt_depth = VK_FORMAT_D32_SFLOAT;
			if (image_depth == nullptr) {
				image_depth = create_image(device, w, h, fmt_depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
				mimages[name_depth] = image_depth;
				LOG_INFO("create_image name_depth=%s, image_depth=0x%p\n", name_depth.c_str(), image_depth);
			}

			//allocate depth memreq and Bind
			if (mmemreqs.count(name_depth) == 0) {
				VkMemoryRequirements memreqs = {};

				vkGetImageMemoryRequirements(device, image_depth, &memreqs);
				memreqs.size = memreqs.size + (memreqs.alignment - 1);
				memreqs.size &= ~(memreqs.alignment - 1);
				mmemreqs[name_depth] = memreqs;
				VkDeviceMemory devmem = alloc_devmem(
						name_depth, memreqs.size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				vkBindImageMemory(device, image_depth, devmem, 0);
			}

			//DEPTH VIEW
			auto imageview_depth = mimageviews[name_depth];
			if (imageview_depth == nullptr) {
				LOG_INFO("create_image_view name=%s\n", name_color.c_str());
				imageview_depth = create_image_view(device, image_depth, fmt_depth, VK_IMAGE_ASPECT_DEPTH_BIT);
				mimageviews[name_depth] = imageview_depth;
				LOG_INFO("create_image_view imageview_depth=0x%p\n", imageview_depth);
			}

			//RENDER PASS
			auto renderpass = mrenderpasses[name];
			if (renderpass == nullptr) {
				//create render pass -> framebuffers -> BeginPass?
				renderpass = create_renderpass(device, 1, is_backbuffer, fmt_color, fmt_depth);
				LOG_INFO("create_renderpass name=%s, ptr=%p\n", name_color.c_str(), renderpass);
				mrenderpasses[name] = renderpass;
			}

			//FRAMEBUFFER
			auto framebuffer = mframebuffers[name];
			if (framebuffer == nullptr && renderpass) {
				std::vector<VkImageView> imageviews;
				imageviews.push_back(imageview_color);
				imageviews.push_back(imageview_depth);

				framebuffer = create_framebuffer(device, renderpass, imageviews, w, h);
				LOG_INFO("create_framebuffer name=%s, ptr=%p\n", name_color.c_str(), framebuffer);
				mframebuffers[name] = framebuffer;
			}
			LOG_INFO("found renderpass name=%s, ptr=%p\n", name_color.c_str(), renderpass);
			LOG_INFO("found framebuffer name=%s, ptr=%p\n", name_color.c_str(), framebuffer);

			//BeginRenderPass
			if (renderpass && framebuffer) {
				VkClearValue clear_values[2];
				clear_values[0].color.float32[0] = 1;
				clear_values[0].color.float32[1] = 0;
				clear_values[0].color.float32[2] = 0;
				clear_values[0].color.float32[3] = 1;
				clear_values[1].depthStencil.depth = 1.0f;
				clear_values[1].depthStencil.stencil = 0;
				VkRenderPassBeginInfo rp_begin = {};
				rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				rp_begin.pNext = NULL;
				rp_begin.renderPass = renderpass;
				rp_begin.framebuffer = framebuffer;
				rp_begin.renderArea.offset.x = 0;
				rp_begin.renderArea.offset.y = 0;
				rp_begin.renderArea.extent.width = w;
				rp_begin.renderArea.extent.height = h;
				rp_begin.clearValueCount = 0;
				rp_begin.pClearValues = nullptr;
				vkCmdBeginRenderPass(ref.cmdbuf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

				rec.renderpass = renderpass;
			}

			//Descriptor sets for pat.
			auto descriptor_sets = mdescriptor_sets[name];
			if (descriptor_sets == nullptr) {
				VkDescriptorSetAllocateInfo alloc_info = {};
				alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				alloc_info.pNext = NULL,
				alloc_info.descriptorPool = desc_pool;
				alloc_info.descriptorSetCount = 1,
				alloc_info.pSetLayouts = &layout;
				auto err = vkAllocateDescriptorSets(device, &alloc_info, &descriptor_sets);
				mdescriptor_sets[name] = descriptor_sets;
				LOG_INFO("vkAllocateDescriptorSets name=%s addr=%p\n", name.c_str(), descriptor_sets);
			}
			if (!descriptor_sets) {
				LOG_ERR("descrptor_sets is null name=%s\n", name.c_str());
				exit(1);
			}
			rec.descriptor_sets = descriptor_sets;
			vkCmdBindDescriptorSets(
				ref.cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0,
				1, (const VkDescriptorSet *)&descriptor_sets, 0, NULL);
			LOG_INFO("vkCmdBindDescriptorSets name=%s Done\n", name.c_str());
		}

		//CMD_SET_TEXTURE
		if (type == CMD_SET_TEXTURE || type == CMD_SET_TEXTURE_UAV) {
			uint32_t w = c.set_texture.rect.w;
			uint32_t h = c.set_texture.rect.h;
			auto slot = c.set_texture.slot;

			//COLOR
			auto name_color = name;
			auto fmt_color = VK_FORMAT_B8G8R8A8_UNORM;
			auto image_color = mimages[name_color];
			if (image_color == nullptr) {
				image_color = create_image(device, w, h, fmt_color,
						VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				mimages[name_color] = image_color;
				LOG_INFO("create_image name_color=%s, image_color=0x%p\n", name_color.c_str(), image_color);
			}

			//allocate color memreq and Bind
			if (mmemreqs.count(name_color) == 0) {
				VkMemoryRequirements memreqs = {};

				vkGetImageMemoryRequirements(device, image_color, &memreqs);
				memreqs.size = memreqs.size + (memreqs.alignment - 1);
				memreqs.size &= ~(memreqs.alignment - 1);
				mmemreqs[name_color] = memreqs;

				VkDeviceMemory devmem = alloc_devmem(
						name_color, memreqs.size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				vkBindImageMemory(device, image_color, devmem, 0);

				//create stating textures
				{
					LOG_INFO("create_buffer-staging name=%s\n", name.c_str());
					auto scratch_buffer = create_buffer(device, c.set_texture.size);
					ref.vscratch_buffers.push_back(scratch_buffer);

					LOG_INFO("create_buffer-staging name=%s Done\n", name.c_str());
					VkMemoryRequirements memreqs = {};
					vkGetBufferMemoryRequirements(device, scratch_buffer, &memreqs);
					memreqs.size = memreqs.size + (memreqs.alignment - 1);
					memreqs.size &= ~(memreqs.alignment - 1);
					VkDeviceMemory devmem = alloc_devmem(std::string(name) + "_staging", memreqs.size,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);
					ref.vscratch_devmems.push_back(devmem);
					vkBindBufferMemory(device, scratch_buffer, devmem, 0);
					LOG_INFO("vkBindBufferMemory name=%s Done\n", name.c_str());
					void *dest = nullptr;
					vkMapMemory(device, devmem, 0, memreqs.size, 0, (void **)&dest);
					if (dest) {
						LOG_INFO("vkMapMemory name=%s addr=0x%p\n", name.c_str(), dest);
						memcpy(dest, c.set_texture.data, c.set_texture.size);
						vkUnmapMemory(device, devmem);
					} else {
						LOG_ERR("vkMapMemory name=%s addr=0x%p\n", name.c_str(), dest);
						Sleep(1000);
					}

					auto before_barrier = get_barrier(
							image_color,
							VK_IMAGE_ASPECT_COLOR_BIT,
							VK_IMAGE_LAYOUT_PREINITIALIZED,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							0,
							VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
							VK_PIPELINE_STAGE_TRANSFER_BIT);
					vkCmdPipelineBarrier(
						ref.cmdbuf,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						0, 0, NULL, 0, NULL, 1, &before_barrier);

					VkBufferImageCopy copy_region = {};
					copy_region.bufferOffset = 0;
					copy_region.bufferRowLength = w;
					copy_region.bufferImageHeight = h;
					copy_region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
					copy_region.imageOffset = {0, 0, 0};
					copy_region.imageExtent = {w, h, 1};
					vkCmdCopyBufferToImage(
						ref.cmdbuf,
						scratch_buffer,
						image_color,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1, &copy_region);
					auto after_barrier = get_barrier(
							image_color,
							VK_IMAGE_ASPECT_COLOR_BIT,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
							VK_ACCESS_TRANSFER_WRITE_BIT,
							VK_PIPELINE_STAGE_TRANSFER_BIT,
							VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
					vkCmdPipelineBarrier(
						ref.cmdbuf,
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						0, 0, NULL, 0, NULL, 1, &after_barrier);
				}
			}

			//COLOR VIEW
			auto imageview_color = mimageviews[name_color];
			if (imageview_color == nullptr) {
				LOG_INFO("create_image_view name=%s\n", name_color.c_str());
				imageview_color = create_image_view(device, image_color, fmt_color, VK_IMAGE_ASPECT_COLOR_BIT);
				mimageviews[name_color] = imageview_color;
				LOG_INFO("create_image_view imageview_color=0x%p\n", imageview_color);
			}

			auto descriptor_sets = rec.descriptor_sets;
			if (descriptor_sets == nullptr) {
				LOG_ERR("descriptor_sets is null. create the rendertarget inners.\n");
				exit(1);
			}

			auto binding = (RDT_SLOT_MAX * slot) + RDT_SLOT_SRV;
			VkDescriptorImageInfo image_info = {};
			image_info.sampler = sampler_linear;
			image_info.imageView = imageview_color;
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet descriptor_writes = {};
			descriptor_writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes.pNext = NULL;
			descriptor_writes.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_writes.descriptorCount = 1;
			descriptor_writes.dstSet = descriptor_sets;
			descriptor_writes.dstBinding = binding;
			descriptor_writes.dstArrayElement = 0;
			descriptor_writes.pImageInfo = &image_info;
			LOG_INFO("vkUpdateDescriptorSets(image) start name=%s binding=%d\n", name.c_str(), binding);
			vkUpdateDescriptorSets(device, 1, &descriptor_writes, 0, NULL);
			LOG_INFO("vkUpdateDescriptorSets(image) name=%s Done\n", name.c_str());
		}

		//CMD_SET_CONSTANT
		if (type == CMD_SET_CONSTANT) {
			auto slot = c.set_constant.slot;
			auto data = c.set_constant.data;
			auto size = c.set_constant.size;

			//Create Constant Buffer
			auto buffer = mbuffers[name];
			if (buffer == nullptr) {
				LOG_INFO("create_buffer-constant name=%s\n", name.c_str());
				buffer = create_buffer(device, size);
				mbuffers[name] = buffer;
				LOG_INFO("create_buffer-constant name=%s Done\n", name.c_str());
			}

			//allocate buffer memreq and Bind
			if (mmemreqs.count(name) == 0) {
				VkMemoryRequirements memreqs = {};

				vkGetBufferMemoryRequirements(device, buffer, &memreqs);
				memreqs.size = memreqs.size + (memreqs.alignment - 1);
				memreqs.size &= ~(memreqs.alignment - 1);
				mmemreqs[name] = memreqs;
				VkDeviceMemory devmem = alloc_devmem(name, memreqs.size,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				vkBindBufferMemory(device, buffer, devmem, 0);
				LOG_INFO("vkBindBufferMemory name=%s Done\n", name.c_str());

				void *dest = nullptr;
				vkMapMemory(device, devmem, 0, memreqs.size, 0, (void **)&dest);
				if (dest) {
					LOG_INFO("vkMapMemory name=%s addr=0x%p\n", name.c_str(), dest);
					memcpy(dest, data, size);
					vkUnmapMemory(device, devmem);
				} else {
					LOG_ERR("vkMapMemory name=%s addr=0x%p\n", name.c_str(), dest);
					Sleep(1000);
				}
			}

			auto descriptor_sets = rec.descriptor_sets;
			if (descriptor_sets == nullptr) {
				LOG_ERR("descriptor_sets is null. create the rendertarget inners.\n");
				exit(1);
			}

			auto binding = (RDT_SLOT_MAX * slot) + RDT_SLOT_CBV;
			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = buffer;
			buffer_info.offset = 0;
			buffer_info.range = size;

			VkWriteDescriptorSet descriptor_writes = {};
			descriptor_writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes.pNext = NULL;
			descriptor_writes.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writes.descriptorCount = 1;
			descriptor_writes.dstSet = descriptor_sets;
			descriptor_writes.dstBinding = binding;
			descriptor_writes.dstArrayElement = 0;
			descriptor_writes.pBufferInfo = &buffer_info;
			LOG_INFO("vkUpdateDescriptorSets start name=%s binding=%d\n", name.c_str(), binding);
			vkUpdateDescriptorSets(device, 1, &descriptor_writes, 0, NULL);
			LOG_INFO("vkUpdateDescriptorSets name=%s Done\n", name.c_str());


		}

		//CMD_SET_VERTEX
		if (type == CMD_SET_VERTEX) {
			auto buffer = mbuffers[name];
			auto size = c.set_vertex.size;
			auto data = c.set_vertex.data;
			if (buffer == nullptr) {
				LOG_INFO("create_buffer-vertex name=%s\n", name.c_str());
				buffer = create_buffer(device, size);
				mbuffers[name] = buffer;
				LOG_INFO("create_buffer-vertex name=%s Done\n", name.c_str());
			}

			//allocate buffer memreq and Bind
			if (mmemreqs.count(name) == 0) {
				VkMemoryRequirements memreqs = {};

				vkGetBufferMemoryRequirements(device, buffer, &memreqs);
				memreqs.size = memreqs.size + (memreqs.alignment - 1);
				memreqs.size &= ~(memreqs.alignment - 1);
				mmemreqs[name] = memreqs;
				VkDeviceMemory devmem = alloc_devmem(name, memreqs.size,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				vkBindBufferMemory(device, buffer, devmem, 0);
				LOG_INFO("vkBindBufferMemory name=%s Done\n", name.c_str());

				void *dest = nullptr;
				vkMapMemory(device, devmem, 0, memreqs.size, 0, (void **)&dest);
				if (dest) {
					LOG_INFO("vkMapMemory name=%s addr=0x%p\n", name.c_str(), dest);
					memcpy(dest, data, size);
					vkUnmapMemory(device, devmem);
				} else {
					LOG_ERR("vkMapMemory name=%s addr=0x%p\n", name.c_str(), dest);
					Sleep(1000);
				}
			}

			VkDeviceSize offsets[1] = {0};
			vkCmdBindVertexBuffers(ref.cmdbuf, 0, 1, &buffer, offsets);
			LOG_INFO("vkCmdBindVertexBuffers name=%s\n", name.c_str());
		}

		//CMD_SET_INDEX
		if (type == CMD_SET_INDEX) {
			auto buffer = mbuffers[name];
			auto size = c.set_index.size;
			auto data = c.set_index.data;
			if (buffer == nullptr) {
				LOG_INFO("create_buffer-index name=%s\n", name.c_str());
				buffer = create_buffer(device, size);
				mbuffers[name] = buffer;
				LOG_INFO("create_buffer-index name=%s Done\n", name.c_str());
			}

			//allocate buffer memreq and Bind
			if (mmemreqs.count(name) == 0) {
				VkMemoryRequirements memreqs = {};

				vkGetBufferMemoryRequirements(device, buffer, &memreqs);
				memreqs.size = memreqs.size + (memreqs.alignment - 1);
				memreqs.size &= ~(memreqs.alignment - 1);
				mmemreqs[name] = memreqs;
				VkDeviceMemory devmem = alloc_devmem(name, memreqs.size,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				vkBindBufferMemory(device, buffer, devmem, 0);
				LOG_INFO("vkBindBufferMemory index name=%s Done\n", name.c_str());

				void *dest = nullptr;
				vkMapMemory(device, devmem, 0, memreqs.size, 0, (void **)&dest);
				if (dest) {
					LOG_INFO("vkMapMemory index name=%s addr=0x%p\n", name.c_str(), dest);
					memcpy(dest, data, size);
					vkUnmapMemory(device, devmem);
				} else {
					LOG_ERR("vkMapMemory name=%s addr=0x%p\n", name.c_str(), dest);
					Sleep(1000);
				}
			}

			VkDeviceSize offset = {};
			vkCmdBindIndexBuffer(ref.cmdbuf, buffer, offset, VK_INDEX_TYPE_UINT32);
			LOG_INFO("vkCmdBindIndexBuffers name=%s\n", name.c_str());
		}

		//CMD_SET_SHADER
		if (type == CMD_SET_SHADER) {

			auto graphics_pipeline = mgraphics_pipelines[name];
			if (graphics_pipeline == nullptr) {
				VkShaderModule frag_shader_module = nullptr;
				VkShaderModule vert_shader_module = nullptr;
				VkGraphicsPipelineCreateInfo pipeline_info = {};
				VkPipelineCacheCreateInfo pipelineCache = {};
				VkPipelineVertexInputStateCreateInfo vi = {};
				VkPipelineInputAssemblyStateCreateInfo ia = {};
				VkPipelineRasterizationStateCreateInfo rs = {};
				VkPipelineColorBlendStateCreateInfo cb = {};
				VkPipelineDepthStencilStateCreateInfo ds = {};
				VkPipelineViewportStateCreateInfo vp = {};
				VkPipelineMultisampleStateCreateInfo ms = {};
				VkDynamicState dynamicStateEnables[2] = {};
				VkPipelineDynamicStateCreateInfo dynamicState = {};

				memset(&dynamicState, 0, sizeof dynamicState);
				dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				dynamicState.pDynamicStates = dynamicStateEnables;

				memset(&pipeline_info, 0, sizeof(pipeline_info));
				pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipeline_info.layout = pipeline_layout;

				memset(&vi, 0, sizeof(vi));
				vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

				memset(&ia, 0, sizeof(ia));
				ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

				memset(&rs, 0, sizeof(rs));
				rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rs.polygonMode = VK_POLYGON_MODE_FILL;
				rs.cullMode = VK_CULL_MODE_BACK_BIT;
				rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				rs.depthClampEnable = VK_FALSE;
				rs.rasterizerDiscardEnable = VK_FALSE;
				rs.depthBiasEnable = VK_FALSE;
				rs.lineWidth = 1.0f;

				memset(&cb, 0, sizeof(cb));
				cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				VkPipelineColorBlendAttachmentState att_state[1];
				memset(att_state, 0, sizeof(att_state));
				att_state[0].colorWriteMask = 0xf;
				att_state[0].blendEnable = VK_FALSE;
				cb.attachmentCount = 1;
				cb.pAttachments = att_state;

				memset(&vp, 0, sizeof(vp));
				vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				vp.viewportCount = 1;
				dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
				vp.scissorCount = 1;
				dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

				memset(&ds, 0, sizeof(ds));
				ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				ds.depthTestEnable = VK_TRUE;
				ds.depthWriteEnable = VK_TRUE;
				ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				ds.depthBoundsTestEnable = VK_FALSE;
				ds.back.failOp = VK_STENCIL_OP_KEEP;
				ds.back.passOp = VK_STENCIL_OP_KEEP;
				ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
				ds.stencilTestEnable = VK_FALSE;
				ds.front = ds.back;

				memset(&ms, 0, sizeof(ms));
				ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				ms.pSampleMask = NULL;
				ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

				//TODO compile shader and bind shader

				// Two stages: vs and fs
				VkPipelineShaderStageCreateInfo shaderStages[2];
				memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

				shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
				shaderStages[0].module = vert_shader_module;
				shaderStages[0].pName = "main";

				shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				shaderStages[1].module = frag_shader_module;
				shaderStages[1].pName = "main";

				pipeline_info.pVertexInputState = &vi;
				pipeline_info.pInputAssemblyState = &ia;
				pipeline_info.pRasterizationState = &rs;
				pipeline_info.pColorBlendState = &cb;
				pipeline_info.pMultisampleState = &ms;
				pipeline_info.pViewportState = &vp;
				pipeline_info.pDepthStencilState = &ds;
				pipeline_info.stageCount = _countof(shaderStages);
				pipeline_info.pStages = shaderStages;
				pipeline_info.pDynamicState = &dynamicState;
				pipeline_info.renderPass = rec.renderpass;

				LOG_INFO("vkCreateGraphicsPipelines name=%s\n", name.c_str());
				//auto pipeline_result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, NULL, &graphics_pipeline);
				LOG_INFO("vkCreateGraphicsPipelines Done name=%s\n", name.c_str());

				if (frag_shader_module)
					vkDestroyShaderModule(device, frag_shader_module, NULL);
				if (vert_shader_module)
					vkDestroyShaderModule(device, vert_shader_module, NULL);
			}
		}

		//CMD_CLEAR
		if (type == CMD_CLEAR) {
			auto name_color = name;
			auto image_color = mimages[name_color];
			if (image_color == nullptr)
				LOG_ERR("NULL image_color name=%s\n", name.c_str());

			VkClearColorValue clearColor = {};
			clearColor.float32[0] = c.clear.color[0];
			clearColor.float32[1] = c.clear.color[1];
			clearColor.float32[2] = c.clear.color[2];
			clearColor.float32[3] = c.clear.color[3];

			VkClearValue clearValue = {};
			clearValue.color = clearColor;

			VkImageSubresourceRange imageRange = {};
			imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageRange.baseMipLevel = 0;
			imageRange.levelCount = 1;
			imageRange.baseArrayLayer = 0;
			imageRange.layerCount = 1;
			LOG_INFO("vkCmdClearColorImage name=%s\n", name_color.c_str());

			vkCmdClearColorImage(ref.cmdbuf, image_color, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &imageRange);
		}

		//CMD_CLEAR_DEPTH
		if (type == CMD_CLEAR_DEPTH) {

		}

		//CMD_DRAW_INDEX
		if (type == CMD_DRAW_INDEX) {
			auto count = c.draw_index.count;
		}

		//CMD_DRAW
		if (type == CMD_DRAW) {
			auto vertex_count = c.draw.vertex_count;
		}

		//CMD_DISPATCH
		if (type == CMD_DISPATCH) {
			auto x = c.dispatch.x;
			auto y = c.dispatch.y;
			auto z = c.dispatch.z;
		}
	}
	if (rec.renderpass)
		vkCmdEndRenderPass(ref.cmdbuf);

	//End Command Buffer
	vkEndCommandBuffer(ref.cmdbuf);

	//Submit and Present
	VkPipelineStageFlags wait_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = &wait_mask;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &ref.cmdbuf;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;
	{
		auto fence_status = vkGetFenceStatus(device, ref.fence);
		LOG_INFO("BEFORE vkGetFenceStatus fence_status=%d\n", fence_status);
		vkResetFences(device, 1, &ref.fence);
	}

	LOG_INFO("vkQueueSubmit backbuffer_index=%d, fence=%p\n", backbuffer_index, ref.fence);
	auto submit_result = vkQueueSubmit(graphics_queue, 1, &submit_info, ref.fence);
	LOG_INFO("vkQueueSubmit Done backbuffer_index=%d, fence=%p, submit_result=%d\n", backbuffer_index, ref.fence, submit_result);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;
	present_info.waitSemaphoreCount = 0;
	present_info.pWaitSemaphores = nullptr;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain;
	present_info.pImageIndices = &present_index;
	present_info.pResults = nullptr;

	vkQueuePresentKHR(graphics_queue, &present_info);

	frame_count++;
	backbuffer_index = frame_count % count;
	LOG_INFO("FRAME Done frame_count=%d\n", frame_count);
}
