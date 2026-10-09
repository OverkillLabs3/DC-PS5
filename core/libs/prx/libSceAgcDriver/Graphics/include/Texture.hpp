#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_TEXTURE_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_GRAPHICS_INCLUDE_TEXTURE_HPP

#include "prx/libSceAgcDriver/Graphics/include/Context.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Resources.hpp"
#include "prx/libSceAgcDriver/Graphics/include/GuestTextureResource.hpp"
#include "prx/libSceAgcDriver/Graphics/include/TextureDetiler.hpp"

namespace AgcDriver::Graphics {

class ResidentColor;
class CommandBatch;

class Texture {
public:
    Texture(const Context& context, TextureDetiler& detiler, const GuestTextureResource& descriptor, VkComponentMapping components, std::span<const std::byte> snapshot);
    // aliasView: sample the resident render target's own image through a view instead of copying it (requires a matching format).
    Texture(const Context& context, const std::shared_ptr<ResidentColor>& source, const GuestTextureResource& descriptor, VkComponentMapping components, bool aliasView = false);
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    VkImageView View() const;
    VkDeviceSize AllocationBytes() const { return allocationBytes; }
    bool Aliased() const { return aliased; }
    // True while the asynchronous guest upload is still executing; releases the upload resources once it has finished.
    bool UploadPending();
    const std::shared_ptr<ResidentColor>& Source() const { return source; }

private:
    void release() noexcept;

    Context context;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceSize allocationBytes = 0;
    bool aliased = false;
    std::shared_ptr<ResidentColor> source;
    std::unique_ptr<CommandBatch> upload;
    std::unique_ptr<Buffer> stagingKeep;
    std::unique_ptr<Buffer> linearKeep;
};

}

#endif
