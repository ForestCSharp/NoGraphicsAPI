#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <NoGraphicsAPI/NoGraphicsAPI.hpp>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

using namespace gpu;

namespace
{

Span<byte> read_shader(const char* path) noexcept
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

    Span<byte> code(static_cast<byte*>(malloc(size_t(byte_count))), size_t(byte_count));
    const bool read_succeeded = fread(code.data, 1, code.size, file) == code.size;
    fclose(file);
    if (!read_succeeded || *reinterpret_cast<uint32*>(code.data) != 0x07230203u)
    {
        fprintf(stderr, "Invalid SPIR-V file: %s\n", path);
        free(code.data);
        return {};
    }
    return code;
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

int main()
{
    constexpr uint32 width = 512;
    constexpr uint32 height = 512;

    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(int(width), int(height), "NoGraphicsAPI triangle", nullptr, nullptr);
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

    printf("Using %s\n", get_device_caps(device).device_name);

    const Span<byte> vertex_code = read_shader(NOGRAPHICSAPI_VERTEX_SHADER_PATH);
    const Span<byte> fragment_code = read_shader(NOGRAPHICSAPI_FRAGMENT_SHADER_PATH);
    PSO* triangle_pso = create_graphics_pso(device, {
        .vertex = {.code = {vertex_code.data, vertex_code.size}, .entry_point = "vertexMain"},
        .fragment = {.code = {fragment_code.data, fragment_code.size}, .entry_point = "fragmentMain"},
        .color_targets = { { .format = Format::bgra8_srgb } }
    });
    free(fragment_code.data);
    free(vertex_code.data);
    if (!triangle_pso)
    {
        destroy_device(device);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSetKeyCallback(window, key_callback);

    TimelinePoint latest_completion{ .semaphore = create_timeline_semaphore(device) };
    CommandPool* command_pools[] = {create_command_pool(device), create_command_pool(device)};

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwWindowShouldClose(window))
            break;
        if (latest_completion.value >= 2)
            wait_timeline({.semaphore = latest_completion.semaphore, .value = latest_completion.value - 1});
        CommandPool* command_pool = command_pools[latest_completion.value % 2];
        reset_command_pool(command_pool);
        CommandBuffer* commands = begin_commands(command_pool);
        const SwapchainFrame frame = acquire(commands);
        if (!frame.render_view)
            continue;
        begin_render_pass(commands, {
            .colors = { { .render_view = frame.render_view, .load = LoadOp::clear } },
        });
        bind_pso(commands, triangle_pso);
        draw(commands, {}, 3);
        end_render_pass(commands);
        end_commands(commands);
        latest_completion.value++;
        submit_and_present(device, {.commands = {commands}, .completion = latest_completion});
    }

    wait_idle(device);

    destroy_command_pool(command_pools[1]);
    destroy_command_pool(command_pools[0]);
    destroy_timeline_semaphore(latest_completion.semaphore);
    destroy_pso(triangle_pso);

    destroy_device(device);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
