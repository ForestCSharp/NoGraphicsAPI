// The texture map is adapted from Vulkan-Tools' vkcube sample. Copyright
// notices and the Apache-2.0 license are in NOTICE.md and LICENSE-Apache-2.0.txt.

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "cube_shared.h"

#include <NoGraphicsAPI/NoGraphicsAPI.hpp>
#include <NoGraphicsAPIUtility/bump_allocator.hpp>
#include <NoGraphicsAPIUtility/math.hpp>
#include <NoGraphicsAPIUtility/texture_allocator.hpp>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace gpu;

namespace {

	constexpr uint32 width = 500;
	constexpr uint32 height = 500;
	constexpr uint32 texture_width = 256;
	constexpr uint32 texture_height = 256;
	constexpr size_t texture_byte_count = size_t(texture_width) * texture_height * 4;
	constexpr uint64 data_heap_size = 1024 * 1024;
	constexpr uint64 texture_heap_size = 256 * 1024 * 1024;
	constexpr float radians_per_frame = 4.0f * math::pi / 180.0f;

	constexpr CubeVertex cube_vertices[] = {
		{ .position = { .x = -1.0f, .y = -1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 1.0f } },
		{ .position = { .x = -1.0f, .y = -1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 1.0f } },
		{ .position = { .x = -1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 0.0f } },
		{ .position = { .x = -1.0f, .y = 1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 0.0f } },

		{ .position = { .x = -1.0f, .y = -1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 1.0f } },
		{ .position = { .x = -1.0f, .y = 1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 0.0f } },
		{ .position = { .x = 1.0f, .y = 1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 0.0f } },
		{ .position = { .x = 1.0f, .y = -1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 1.0f } },

		{ .position = { .x = -1.0f, .y = -1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 0.0f } },
		{ .position = { .x = 1.0f, .y = -1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 1.0f } },
		{ .position = { .x = 1.0f, .y = -1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 1.0f } },
		{ .position = { .x = -1.0f, .y = -1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 0.0f } },

		{ .position = { .x = -1.0f, .y = 1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 0.0f } },
		{ .position = { .x = -1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 0.0f } },
		{ .position = { .x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 1.0f } },
		{ .position = { .x = 1.0f, .y = 1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 1.0f } },

		{ .position = { .x = 1.0f, .y = 1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 0.0f } },
		{ .position = { .x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 0.0f } },
		{ .position = { .x = 1.0f, .y = -1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 1.0f } },
		{ .position = { .x = 1.0f, .y = -1.0f, .z = -1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 1.0f } },

		{ .position = { .x = -1.0f, .y = -1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 1.0f } },
		{ .position = { .x = 1.0f, .y = -1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 1.0f } },
		{ .position = { .x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 1.0f, .y = 0.0f } },
		{ .position = { .x = -1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f }, .uv = { .x = 0.0f, .y = 0.0f } },
	};

	constexpr uint16 cube_indices[] = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4,
		8, 9, 10, 10, 11, 8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20,
	};

	constexpr size_t cube_vertex_count = sizeof(cube_vertices) / sizeof(cube_vertices[0]);
	constexpr uint32 cube_index_count = uint32(sizeof(cube_indices) / sizeof(cube_indices[0]));

} // namespace

namespace {

Span<uint32> read_spirv(const char* path) noexcept
{
	assert(path);
	FILE* file = fopen(path, "rb");
	if (!file)
	{
		fprintf(stderr, "Failed to open SPIR-V file: %s\n", path);
		return {};
	}

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fprintf(stderr, "Failed to read SPIR-V file: %s\n", path);
		fclose(file);
		return {};
	}
	const long byte_count = ftell(file);
	if (byte_count < static_cast<long>(5 * sizeof(uint32)) || byte_count % static_cast<long>(sizeof(uint32)) != 0)
	{
		fprintf(stderr, "Invalid SPIR-V file size: %s\n", path);
		fclose(file);
		return {};
	}
	rewind(file);

	Span<uint32> code(static_cast<uint32*>(malloc(size_t(byte_count))), size_t(byte_count) / sizeof(uint32));
	const bool read_succeeded = fread(code.data, sizeof(uint32), code.size, file) == code.size;
	fclose(file);
	if (!read_succeeded || code.data[0] != 0x07230203u)
	{
		fprintf(stderr, "Invalid SPIR-V file: %s\n", path);
		free(code.data);
		return {};
	}
	return code;
}

bool read_binary_file(const char* path, Span<byte> data) noexcept
{
	assert(path && data.data && data.size);
	FILE* file = fopen(path, "rb");
	if (!file)
	{
		fprintf(stderr, "Failed to open resource file: %s\n", path);
		return false;
	}
	const bool size_succeeded = fseek(file, 0, SEEK_END) == 0 && ftell(file) == static_cast<long>(data.size);
	rewind(file);
	const bool read_succeeded = size_succeeded && fread(data.data, 1, data.size, file) == data.size;
	fclose(file);
	if (!read_succeeded)
		fprintf(stderr, "Invalid resource file: %s\n", path);
	return read_succeeded;
}

void* create_glfw_surface(void* vulkan_instance, void* window) noexcept
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	const VkResult result = glfwCreateWindowSurface(
		static_cast<VkInstance>(vulkan_instance), static_cast<GLFWwindow*>(window), nullptr, &surface);
	return result == VK_SUCCESS ? reinterpret_cast<void*>(surface) : nullptr;
}

void get_window_size(void* window, uint32x2* size) noexcept
{
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(static_cast<GLFWwindow*>(window), &width, &height);
	*size = { .x = static_cast<uint32>(width), .y = static_cast<uint32>(height) };
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

} // namespace

int main() {
	if (!glfwInit())
		return 1;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(int(width), int(height), "NoGraphicsAPI spinning textured cube", nullptr, nullptr);
	uint32_t instance_extension_count = 0;
	const char** instance_extensions = glfwGetRequiredInstanceExtensions(&instance_extension_count);
	DeviceInit init{};
	if (window && instance_extensions)
	{
		init = create_device({
			.window = window,
			.swapchain_format = Format::bgra8_srgb,
			.instance_extensions = {instance_extensions, size_t(instance_extension_count)},
			.create_surface = create_glfw_surface,
			.get_window_size = get_window_size,
		});
	}
	Device* device = init.device;

	if (!window || !device)
	{
		if (init.error != Error::none)
			fprintf(stderr, "Failed to create device: error %d\n", int(init.error));
		destroy_device(device);
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	glfwSetKeyCallback(window, key_callback);

	const DeviceCaps& caps = get_device_caps(device);
	printf("Using %s\n", caps.device_name);

	// Shaders
	const Span<uint32> vertex_spirv = read_spirv(NOGRAPHICSAPI_CUBE_VERTEX_SPV_PATH);
	const Span<uint32> fragment_spirv = read_spirv(NOGRAPHICSAPI_CUBE_FRAGMENT_SPV_PATH);
	PSO* cube_pso = create_graphics_pso(device, {
		.vertex_spirv = vertex_spirv,
		.fragment_spirv = fragment_spirv,
		.color_targets = {{.format = Format::bgra8_srgb}},
		.depth_format = Format::d32_float,
		.rasterization = { .cull = CullMode::clockwise },
	});
	free(fragment_spirv.data);
	free(vertex_spirv.data);

	// GPU resources
	GpuHeap data_heap = create_gpu_heap(device, data_heap_size);
	BumpAllocator data_allocator(data_heap.range);
	const GpuCpuRange<CubeVertex> vertex_allocation = data_allocator.allocate<CubeVertex>(cube_vertex_count);
	const GpuCpuRange<uint16> index_allocation = data_allocator.allocate<uint16>(cube_index_count);
	const GpuCpuRange<byte> upload_allocation = data_allocator.allocate(texture_byte_count);
	memcpy(vertex_allocation.cpu, cube_vertices, sizeof(cube_vertices));
	memcpy(index_allocation.cpu, cube_indices, sizeof(cube_indices));
	read_binary_file(NOGRAPHICSAPI_CUBE_TEXTURE_PATH, Span<byte>(upload_allocation.cpu, texture_byte_count));

	GpuHeap texture_descriptor_heap = create_gpu_heap(device, caps.texture_descriptor_size, MemoryType::texture_descriptor_heap);
	GpuHeap sampler_descriptor_heap = create_gpu_heap(device, caps.sampler_descriptor_size, MemoryType::sampler_descriptor_heap);
	TextureHeap texture_heap = create_texture_heap(device, texture_heap_size);
	TextureAllocator texture_allocator(device, texture_heap, 16);

	TimelinePoint latest_completion{.semaphore = create_timeline_semaphore(device)};

	// Textures
	PlacedTexture texture = texture_allocator.allocate({
		.extent = {.x = texture_width, .y = texture_height, .z = 1},
		.format = Format::rgba8_srgb,
		.usage = TextureUsage::sampled | TextureUsage::transfer_destination,
	});

	write_texture_descriptor(device, texture_descriptor_heap.range.cpu, texture.texture, TextureDescriptorType::sampled);
	write_sampler_descriptor(device, sampler_descriptor_heap.range.cpu, {
		.min_filter = Filter::nearest,
		.mag_filter = Filter::nearest,
		.address_u = AddressMode::clamp_to_edge,
		.address_v = AddressMode::clamp_to_edge,
	});

	CommandBuffer* upload_commands = begin_commands(device);
	copy_memory_to_texture(upload_commands, gpu_range(upload_allocation), texture.texture);

	barrier(upload_commands,
		Stage::transfer, Access::transfer_write,
		Stage::fragment, Access::shader_read);

	latest_completion.value++;
	submit({ upload_commands }, latest_completion);

	PlacedTexture depth{};
	RenderView* depth_render_view = nullptr;
	uint32x2 depth_extent{};
	uint64 frame_index = 0;
	const float4x4 view = math::look_at_rh({.x = 0.0f, .y = 3.0f, .z = 5.0f}, {.x = 0.0f, .y = 0.0f, .z = 0.0f}, {.x = 0.0f, .y = 1.0f, .z = 0.0f});

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(window))
			break;
		const SwapchainFrame frame = acquire(device);
		if (!frame.render_view)
			continue;

		// Window resize?
		if (frame.extent.x != depth_extent.x || frame.extent.y != depth_extent.y)
		{
			if (depth.texture)
				wait_timeline(latest_completion);
			destroy_render_view(depth_render_view);
			texture_allocator.free(depth);
			depth = texture_allocator.allocate({
				.extent = {.x = frame.extent.x, .y = frame.extent.y, .z = 1},
				.format = Format::d32_float,
				.usage = TextureUsage::depth_stencil_attachment,
			});
			depth_render_view = create_render_view(depth.texture);
			depth_extent = frame.extent;
		}

		// Render
		CommandBuffer* commands = begin_commands(device);
		set_texture_descriptor_heap(commands, gpu_range(texture_descriptor_heap));
		set_sampler_descriptor_heap(commands, gpu_range(sampler_descriptor_heap));

		barrier(commands,
			Stage::depth_stencil_tests, Access::depth_stencil_write,
			Stage::depth_stencil_tests, Access::depth_stencil_write);

		begin_render_pass(commands, {
			.colors = { {
				.render_view = frame.render_view,
				.load = LoadOp::clear,
				.clear = { .x = 0.2f, .y = 0.2f, .z = 0.2f, .w = 0.2f },
			}},
			.depth = {
				.render_view = depth_render_view,
				.load = LoadOp::clear,
				.store = StoreOp::discard,
			},
		});

		set_depth_stencil(commands, {.depth_test = true, .depth_write = true});
		bind_pso(commands, cube_pso);

		float4x4 projection = math::perspective_rh_zo(45.0f * math::pi / 180.0f,
		                                             float(frame.extent.x) / float(frame.extent.y), 0.1f, 100.0f);
		projection.rows[1].y = -projection.rows[1].y;
		const CubeRootArguments root {
			.vertices = vertex_allocation.gpu,
			.transform = projection * view * math::rotation_y(radians_per_frame * float(frame_index++)),
		};

		draw_indexed(commands, root, gpu_range(index_allocation), IndexType::uint16, cube_index_count);

		end_render_pass(commands);

		// Submit
		latest_completion.value++;
		submit_and_present(device, {commands}, latest_completion);
	}

	wait_idle(device);

	// Cleanup
	destroy_timeline_semaphore(latest_completion.semaphore);
	destroy_pso(cube_pso);
	destroy_render_view(depth_render_view);
	texture_allocator.free(depth);
	texture_allocator.free(texture);
	destroy_texture_heap(texture_heap);
	destroy_gpu_heap(sampler_descriptor_heap);
	destroy_gpu_heap(texture_descriptor_heap);
	destroy_gpu_heap(data_heap);

	destroy_device(device);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
