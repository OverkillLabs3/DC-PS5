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

int APS5_VABI sceHttp2AddRequestHeader(int id, const char* name, const char* value, uint32_t mode) {
 (void)id;
 (void)name;
 (void)value;
 (void)mode;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2CreateRequestWithURL(int tmpl_id, const char* method, const char* url, uint64_t content_length) {
 (void)tmpl_id;
 (void)method;
 (void)url;
 (void)content_length;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2CreateTemplate(int lib_http2_ctx_id, const char* user_agent, int http_ver, int is_auto_proxy_conf) {
 (void)lib_http2_ctx_id;
 (void)user_agent;
 (void)http_ver;

 (void)is_auto_proxy_conf;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2DeleteRequest(int req_id) {
 (void)req_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2DeleteTemplate(int tmpl_id) {
 (void)tmpl_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2GetAllResponseHeaders(int req_id, char** header, size_t* header_size) {
 (void)req_id;
 (void)header;
 (void)header_size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2GetResponseContentLength(int req_id, int* result, uint64_t* content_length) {
 (void)req_id;
 (void)result;
 (void)content_length;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2GetStatusCode(int req_id, int* status_code) {
 (void)req_id;
 (void)status_code;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2Init(int libnet_mem_id, int libssl_ctx_id, size_t pool_size, int max_concurrent_request) {
 (void)libnet_mem_id;
 (void)libssl_ctx_id;
 (void)pool_size;
 (void)max_concurrent_request;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2ReadData(int req_id, void* data, size_t size) {
 (void)req_id;
 (void)data;
 (void)size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2ReadDataAsync(int req_id, void* data, size_t size, void* kqueue_option, void* option) {
 (void)req_id;
 (void)data;
 (void)size;
 (void)kqueue_option;
 (void)option;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SendRequest(int req_id, const void* post_data, size_t size) {
 (void)req_id;
 (void)post_data;
 (void)size;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SendRequestAsync(int req_id, const void* post_data, size_t size, void* kqueue_option, void* option) {
 (void)req_id;
 (void)post_data;
 (void)size;
 (void)kqueue_option;
 (void)option;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetAuthEnabled(int id, int is_enable) {
 (void)id;
 (void)is_enable;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetAutoRedirect(int id, int enable) {
 (void)id;
 (void)enable;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetConnectionWaitTimeOut(int id, uint32_t usec) {
 (void)id;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetConnectTimeOut(int id, uint32_t usec) {
 (void)id;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetInflateGZIPEnabled(int id, int enable) {
 (void)id;
 (void)enable;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetRecvTimeOut(int id, uint32_t usec) {
 (void)id;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetRedirectCallback(int id, void* cb_func, void* user_arg) {
 (void)id;
 (void)cb_func;
 (void)user_arg;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetRequestContentLength(int id, uint64_t content_length) {
 (void)id;
 (void)content_length;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetResolveTimeOut(int id, uint32_t usec) {
 (void)id;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetSendTimeOut(int id, uint32_t usec) {
 (void)id;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetSslCallback(int id, void* cb_func, void* user_arg) {
 (void)id;
 (void)cb_func;
 (void)user_arg;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SetTimeOut(int id, uint32_t usec) {
 (void)id;
 (void)usec;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SslDisableOption(int id, uint32_t ssl_flags) {
 (void)id;
 (void)ssl_flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2SslEnableOption(int id, uint32_t ssl_flags) {
 (void)id;
 (void)ssl_flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2Term(int lib_http2_ctx_id) {
 (void)lib_http2_ctx_id;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceHttp2WaitAsync(int req_id, Http2AsyncResult* result, uint32_t* timeout, void* option) {
 (void)req_id;
 (void)result;
 (void)timeout;
 (void)option;
 return SoftFailNp(__func__);
 return 0;
}
}
