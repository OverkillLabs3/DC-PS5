#include <cstdint>
#include <cstdio>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"


namespace {
// Online-service call: soft-fail with SCE_NP_ERROR_ABORTED instead of throwing (a thrown
// exception reaches std::terminate and aborts the whole game process).
int SoftFailNp(const char* func) {
    std::fprintf(stderr, "[SOFT-NP] %s -> 0 (unimplemented SCE API - no-op)\n", func);
    std::fflush(stderr);
    return 0;
}
}  // namespace

extern "C" {

int APS5_VABI sceFontAttachDeviceCacheBuffer(FontLibrary library, void* buffer, uint32_t size) {
 (void)library;
 (void)buffer;
 (void)size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontBindRenderer(FontHandle font_handle, FontRenderer renderer) {
 (void)font_handle;
 (void)renderer;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCharacterGetBidiLevel(FontTextCharacter text_character, int* bidi_level) {
 (void)text_character;
 (void)bidi_level;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCharacterGetSyllableStringState(FontTextCharacter text_character, int* syllable_string_state) {
 (void)text_character;
 (void)syllable_string_state;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCharacterGetTextFontCode(FontTextCharacter text_character, FontHandle* font_handle, uint32_t* text_code) {
 (void)text_character;
 (void)font_handle;
 (void)text_code;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCharacterGetTextOrder(FontTextCharacter text_character, void** text_order) {
 (void)text_character;
 (void)text_order;
 return SoftFailNp(__func__);
 return 0;
}
uint32_t APS5_VABI sceFontCharacterLooksFormatCharacters(FontTextCharacter text_character) {
 (void)text_character;
 return SoftFailNp(__func__);
 return 0;
}
uint32_t APS5_VABI sceFontCharacterLooksWhiteSpace(FontTextCharacter text_character) {
 (void)text_character;
 return SoftFailNp(__func__);
 return 0;
}
FontTextCharacter APS5_VABI sceFontCharacterRefersTextBack(FontTextCharacter text_character) {
 (void)text_character;
 SoftFailNp(__func__); 
 return {};
}
FontTextCharacter APS5_VABI sceFontCharacterRefersTextNext(FontTextCharacter text_character) {
 (void)text_character;
 SoftFailNp(__func__); 
 return {};
}
FontTextCodes* APS5_VABI sceFontCharactersRefersTextCodes(FontTextCharacter text_character, FontTextCharacter term_character, FontTextCodes* text_codes) {
 (void)text_character;
 (void)term_character;
 (void)text_codes;
 return nullptr;  // SoftFailNp: pointer-returning
}
int APS5_VABI sceFontCloseFont(FontHandle font_handle) {
 (void)font_handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCreateLibrary(const FontMemory* memory, FontLibrarySelection selection, FontLibrary* library) {
 (void)memory;
 (void)selection;
 (void)library;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCreateLibraryWithEdition(const FontMemory* memory, FontLibrarySelection selection, uint64_t edition, FontLibrary* library) {
 (void)memory;
 (void)selection;
 (void)edition;
 (void)library;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCreateRendererWithEdition(const FontMemory* memory, FontRendererSelection selection, uint64_t edition, FontRenderer* renderer) {
 (void)memory;
 (void)selection;
 (void)edition;
 (void)renderer;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCreateString(const FontMemory* memory, FontTextSource* font_text_source, const FontCreateStringDetail* string_detail, FontString* font_string) {
 (void)memory;
 (void)font_text_source;
 (void)string_detail;
 (void)font_string;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontCreateWritingLine(const FontMemory* memory, int writing_form, const void* writing_line_detail, FontWritingLine* writing_line) {
 (void)memory;
 (void)writing_form;
 (void)writing_line_detail;
 (void)writing_line;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontDefineAttribute(FontHandle font_handle, int attribute, int* old_attribute) {
 (void)font_handle;
 (void)attribute;
 (void)old_attribute;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontDeleteGlyph(const FontMemory* memory, void** font_glyph) {
 (void)memory;
 (void)font_glyph;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontDestroyLibrary(FontLibrary* library) {
 (void)library;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontDestroyRenderer(FontRenderer* renderer) {
 (void)renderer;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontDestroyString(FontString* font_string) {
 (void)font_string;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontDestroyWritingLine(FontWritingLine* writing_line) {
 (void)writing_line;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontGenerateCharGlyph(FontHandle font_handle, uint32_t code, const FontGenerateGlyphDetail* detail, void** font_glyph) {
 (void)font_handle;
 (void)code;
 (void)detail;
 (void)font_glyph;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontGetCharGlyphMetrics(FontHandle font_handle, uint32_t code, FontGlyphMetrics* metrics) {
 (void)font_handle;
 (void)code;
 (void)metrics;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontGetHorizontalLayout(FontHandle font_handle, FontHorizontalLayout* layout) {
 (void)font_handle;
 (void)layout;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontGetRenderCharGlyphMetrics(FontHandle font_handle, uint32_t code, FontGlyphMetrics* metrics) {
 (void)font_handle;
 (void)code;
 (void)metrics;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontGetVerticalLayout(FontHandle font_handle, FontVerticalLayout* layout) {
 (void)font_handle;
 (void)layout;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontGlyphDefineAttribute(void* font_glyph, int attribute, int* old_attribute) {
 (void)font_glyph;
 (void)attribute;
 (void)old_attribute;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontMemoryInit(FontMemory* font_memory, void* address, uint32_t size_byte, const FontMemoryInterface* mem_interface, void* mspace_object, FontMemoryDestroyCallback destroy_callback, void* destroy_object) {
 (void)font_memory;
 (void)address;
 (void)size_byte;
 (void)mem_interface;
 (void)mspace_object;
 (void)destroy_callback;
 (void)destroy_object;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontMemoryTerm(FontMemory* font_memory) {
 (void)font_memory;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontOpenFontInstance(FontHandle font_handle, void* setup_font, FontHandle* out_font_handle) {
 (void)font_handle;
 (void)setup_font;
 (void)out_font_handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontOpenFontMemory(FontLibrary library, const void* font_address, uint32_t font_size, const FontOpenDetail* detail, FontHandle* handle) {
 (void)library;
 (void)font_address;
 (void)font_size;
 (void)detail;
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontOpenFontSet(FontLibrary library, uint32_t font_set_type, uint32_t open_mode, const FontOpenDetail* detail, FontHandle* handle) {
 (void)library;
 (void)font_set_type;
 (void)open_mode;
 (void)detail;
 (void)handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontRebindRenderer(FontHandle font_handle) {
 (void)font_handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontRenderCharGlyphImageHorizontal(FontHandle font_handle, uint32_t code, FontRenderSurface* surf, float x, float y, FontGlyphMetrics* metrics, FontRenderResult* result) {
 (void)font_handle;
 (void)code;
 (void)surf;
 (void)x;
 (void)y;
 (void)metrics;
 (void)result;
 return SoftFailNp(__func__);
 return 0;
}
void APS5_VABI sceFontRenderSurfaceInit(FontRenderSurface* surf, void* buffer, int buf_width_byte, int pixel_size_byte, int width, int height) {
 (void)surf;
 (void)buffer;
 (void)buf_width_byte;
 (void)pixel_size_byte;
 (void)width;
 (void)height;
 SoftFailNp(__func__);
 return;
}
void APS5_VABI sceFontRenderSurfaceSetScissor(FontRenderSurface* surf, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) {
 (void)surf;
 (void)x0;
 (void)y0;
 (void)x1;
 (void)y1;
 SoftFailNp(__func__);
 return;
}
int APS5_VABI sceFontSetEffectSlant(FontHandle font_handle, float slant_ratio) {
 (void)font_handle;
 (void)slant_ratio;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontSetEffectWeight(FontHandle font_handle, float weight_x_scale, float weight_y_scale, uint32_t mode) {
 (void)font_handle;
 (void)weight_x_scale;
 (void)weight_y_scale;
 (void)mode;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontSetScalePixel(FontHandle font_handle, float w, float h) {
 (void)font_handle;
 (void)w;
 (void)h;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontSetupRenderEffectSlant(FontHandle font_handle, float slant_ratio) {
 (void)font_handle;
 (void)slant_ratio;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontSetupRenderEffectWeight(FontHandle font_handle, float weight_x_scale, float weight_y_scale, uint32_t mode) {
 (void)font_handle;
 (void)weight_x_scale;
 (void)weight_y_scale;
 (void)mode;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontSetupRenderScalePixel(FontHandle font_handle, float w, float h) {
 (void)font_handle;
 (void)w;
 (void)h;
 return SoftFailNp(__func__);
 return 0;
}
uint32_t APS5_VABI sceFontStringGetTerminateCode(FontString font_string) {
 (void)font_string;
 return SoftFailNp(__func__);
 return 0;
}
void* APS5_VABI sceFontStringGetTerminateOrder(FontString font_string) {
 (void)font_string;
 SoftFailNp(__func__);

 return nullptr;
}
int APS5_VABI sceFontStringGetWritingForm(FontString font_string) {
 (void)font_string;
 return SoftFailNp(__func__);
 return 0;
}
FontRenderCharacter APS5_VABI sceFontStringRefersRenderCharacters(FontString font_string, FontTextCharacter start_character, FontTextCharacter last_character, uint32_t* character_count) {
 (void)font_string;
 (void)start_character;
 (void)last_character;
 (void)character_count;
 SoftFailNp(__func__); 
 return {};
}
FontTextCharacter APS5_VABI sceFontStringRefersTextCharacters(FontString font_string, uint32_t* character_count) {
 (void)font_string;
 (void)character_count;
 SoftFailNp(__func__); 
 return {};
}
int APS5_VABI sceFontSupportExternalFonts(FontLibrary library, uint32_t font_max, uint32_t formats) {
 (void)library;
 (void)font_max;
 (void)formats;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontSupportSystemFonts(FontLibrary library) {
 (void)library;
 return SoftFailNp(__func__);
 return 0;
}
FontTextCodes* APS5_VABI sceFontTextCodesStepBack(FontTextCodes* text_codes_step) {
 (void)text_codes_step;
 return nullptr;  // SoftFailNp: pointer-returning
}
FontTextCodes* APS5_VABI sceFontTextCodesStepNext(FontTextCodes* text_codes_step) {
 (void)text_codes_step;
 return nullptr;  // SoftFailNp: pointer-returning
}
int APS5_VABI sceFontTextSourceInit(FontTextSource* font_text_source, const void* text_address, uint32_t text_size_byte, FontTextParseFunction text_parser, void* text_object) {
 (void)font_text_source;
 (void)text_address;
 (void)text_size_byte;
 (void)text_parser;
 (void)text_object;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontTextSourceRewind(FontTextSource* font_text_source) {
 (void)font_text_source;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontTextSourceSetDefaultFont(FontTextSource* font_text_source, FontHandle default_font) {
 (void)font_text_source;
 (void)default_font;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontTextSourceSetWritingForm(FontTextSource* font_text_source, int writing_form) {
 (void)font_text_source;
 (void)writing_form;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontUnbindRenderer(FontHandle font_handle) {
 (void)font_handle;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontWritingGetRenderMetrics(FontWriting* font_writing, FontWritingMetrics* writing_metrics) {
 (void)font_writing;
 (void)writing_metrics;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontWritingInit(FontWriting* font_writing, FontString font_string, FontRenderCharacter font_character) {
 (void)font_writing;
 (void)font_string;
 (void)font_character;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontWritingLineClear(FontWritingLine writing_line) {
 (void)writing_line;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontWritingLineGetOrderingSpace(FontWritingLine writing_line, float* head_space, float* inline_space, float* tail_space, float* advance_space) {
 (void)writing_line;
 (void)head_space;
 (void)inline_space;
 (void)tail_space;
 (void)advance_space;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceFontWritingLineGetRenderMetrics(FontWritingLine writing_line, FontWritingMetrics* writing_metrics) {
 (void)writing_line;
 (void)writing_metrics;
 return SoftFailNp(__func__);
 return 0;
}
FontWritingLineStep* APS5_VABI sceFontWritingLineRefersRenderStep(FontWritingLine writing_line) {
 (void)writing_line;
 return nullptr;  // SoftFailNp: pointer-returning
}
int APS5_VABI sceFontWritingLineWritesOrder(FontWritingLine writing_line, uint64_t writing_attribute, const FontWritingMetrics* writing_metrics, void* writing_orderer) {
 (void)writing_line;
 (void)writing_attribute;
 (void)writing_metrics;
 (void)writing_orderer;
 return SoftFailNp(__func__);
 return 0;
}
const FontWritingStep* sceFontWritingRefersRenderStep(FontWriting* font_writing) {
 (void)font_writing;
 return nullptr;  // SoftFailNp: pointer-returning
}
FontTextCharacter APS5_VABI sceFontWritingRefersRenderStepCharacter(FontWriting* font_writing, const void** letter_step) {
 (void)font_writing;
 (void)letter_step;
 SoftFailNp(__func__); 
 return {};
}
int APS5_VABI sceFontWritingSetMaskInvisible(FontWriting* font_writing, int mask) {
 (void)font_writing;
 (void)mask;
 return SoftFailNp(__func__);
 return 0;
}
}
