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

int APS5_VABI sceNetAccept(int s, void* addr, uint32_t* addrlen) {
 (void)s;
 (void)addr;
 (void)addrlen;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetBind_nid_postfix(int s, const void* addr, uint32_t addrlen) {
 (void)s;
 (void)addr;
 (void)addrlen;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetEpollControl(int eid, int op, int id, const NetEpollEvent* event) {
 (void)eid;
 (void)op;
 (void)id;
 (void)event;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetEpollCreate(const char* name, int flags) {
 (void)name;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetEpollDestroy(int eid) {
 (void)eid;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetEpollWait(int eid, NetEpollEvent* events, int maxevents, int timeout) {
 (void)eid;
 (void)events;
 (void)maxevents;
 (void)timeout;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetEtherNtostr(const NetEtherAddr* n, char* str, size_t len) {
 (void)n;
 (void)str;
 (void)len;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetGetMacAddress(NetEtherAddr* addr, int flags) {
 (void)addr;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetGetSockInfo(int s, void* info, int n, int flags) {
 (void)s;
 (void)info;
 (void)n;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetGetsockname(int s, void* addr, uint32_t* addrlen) {
 (void)s;
 (void)addr;
 (void)addrlen;
 return SoftFailNp(__func__);
 return 0;
}
uint32_t APS5_VABI sceNetHtonl_nid_postfix(uint32_t host32) {
 (void)host32;
 return SoftFailNp(__func__);
 return 0;
}
uint16_t APS5_VABI sceNetHtons_nid_postfix(uint16_t host16) {
 (void)host16;
 return SoftFailNp(__func__);
 return 0;
}
const char* sceNetInetNtop(int af, const void* src, char* dst, uint32_t size) {
 (void)af;
 (void)src;
 (void)dst;
 (void)size;
 return nullptr;  // SoftFailNp: pointer-returning
}
int APS5_VABI sceNetInetPton(int af, const char* src, void* dst) {
 (void)af;
 (void)src;
 (void)dst;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetInit_nid_postfix(void) {
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetListen(int s, int backlog) {
 (void)s;
 (void)backlog;
 return SoftFailNp(__func__);
 return 0;
}
uint32_t APS5_VABI sceNetNtohl_nid_postfix(uint32_t net32) {
 (void)net32;
 return SoftFailNp(__func__);
 return 0;
}
uint16_t APS5_VABI sceNetNtohs_nid_postfix(uint16_t net16) {
 (void)net16;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetPoolCreate(const char* name, int size, int flags) {
 (void)name;
 (void)size;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetPoolDestroy(int memid) {
 (void)memid;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetResolverCreate(const char* name, int memid, int flags) {
 (void)name;
 (void)memid;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetResolverStartNtoa(int rid, const char* hostname, void* addr, int timeout, int retry, int flags) {
 (void)rid;
 (void)hostname;
 (void)addr;
 (void)timeout;
 (void)retry;
 (void)flags;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetSetsockopt(int s, int level, int optname, const void* optval, uint32_t optlen) {
 (void)s;
 (void)level;
 (void)optname;
 (void)optval;
 (void)optlen;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetShutdown(int s, int how) {
 (void)s;
 (void)how;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetSocket(const char* name, int family, int type, int protocol) {
 (void)name;
 (void)family;
 (void)type;
 (void)protocol;
 return SoftFailNp(__func__);
 return 0;
}
int APS5_VABI sceNetSocketClose(int s) {
 (void)s;
 return SoftFailNp(__func__);
 return 0;
}
}
