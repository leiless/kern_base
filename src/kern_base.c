/*
 * Created 190427 lynnl
 */

#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <mach-o/loader.h>

#include <kern/locks.h>
#include <string.h>

#include <sys/sysctl.h>

#ifndef __kext_makefile__
#define KEXTNAME_S          "kern_base"
#endif

#define LOG(fmt, ...)        printf(KEXTNAME_S ": " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)    LOG("[ERR] " fmt, ##__VA_ARGS__)

#define KERN_ADDR_MASK      0xfffffffffff00000

#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(*a))
#define ARRAY_LAST(a)       (ARRAY_SIZE(a) - 1)

static uint64_t _hib;
static uint64_t kern;

#define ADDR_BUFSZ          19

static char _hib_str[ADDR_BUFSZ];
static char kern_str[ADDR_BUFSZ];

static SYSCTL_NODE(
    _kern,
    OID_AUTO,
    addr,
    CTLFLAG_RD,
    NULL,
    "" /* sysctl node: kern.addr */
)

static SYSCTL_STRING(
    _kern_addr,
    OID_AUTO,
    _hib,
    CTLFLAG_RD,
    _hib_str,
    ARRAY_LAST(_hib_str),
    "" /* sysctl nub: kern.addr.hib */
)

static SYSCTL_STRING(
    _kern_addr,
    OID_AUTO,
    kern,
    CTLFLAG_RD,
    kern_str,
    ARRAY_LAST(kern_str),
    "" /* sysctl nub: kern.addr.slide */
)

/*
 * All node entries should place before nub entries
 */
static struct sysctl_oid *sysctl_entries[] = {
    /* sysctl nodes */
    &sysctl__kern_addr,

    /* sysctl nubs */
    &sysctl__kern_addr__hib,
    &sysctl__kern_addr_kern,
};

static inline void addr_sysctl_register(void)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(sysctl_entries); i++) {
        sysctl_register_oid(sysctl_entries[i]);
    }
}

static inline void addr_sysctl_deregister(void)
{
    ssize_t i;
    /* Deregister should follow inverted order */
    for (i = ARRAY_LAST(sysctl_entries); i >= 0; i--) {
        if (sysctl_entries[i] != NULL) {
            /* Double sysctl_unregister_oid() call causes panic */
            sysctl_unregister_oid(sysctl_entries[i]);
            sysctl_entries[i] = NULL;
        }
    }
}

kern_return_t kern_base_start(kmod_info_t *ki __unused, void *d __unused)
{
    uint32_t t;

    /* hib should less than base(specifically less 0x100000) */
    _hib = ((uint64_t) bcopy) & KERN_ADDR_MASK;

    kern = ((uintptr_t) lck_mtx_assert) & KERN_ADDR_MASK;
    while (1) {
        t = *(uint32_t *) kern;
        /* Only support non-fat 64-bit mach-o kernel */
        if (t == MH_MAGIC_64 || t == MH_CIGAM_64) break;
        kern -= 0x100000;
    }

    if (kern != _hib + 0x100000) {
        /* Kernel layout changes  this kext won't work */
        LOG_ERR("bad text base  __HIB: %#llx kernel: %#llx", _hib, kern);
        return KERN_FAILURE;
    }

    (void) snprintf(_hib_str, ARRAY_SIZE(_hib_str), "%#018llx", _hib);
    (void) snprintf(kern_str, ARRAY_SIZE(_hib_str), "%#018llx", kern);

    addr_sysctl_register();

    return KERN_SUCCESS;
}

kern_return_t kern_base_stop(kmod_info_t *ki __unused, void *d __unused)
{
    addr_sysctl_deregister();
    return KERN_SUCCESS;
}

#ifdef __kext_makefile__
extern kern_return_t _start(kmod_info_t *, void *);
extern kern_return_t _stop(kmod_info_t *, void *);

/* Will expand name if it's a macro */
#define KMOD_EXPLICIT_DECL2(name, ver, start, stop) \
    __attribute__((visibility("default")))          \
        KMOD_EXPLICIT_DECL(name, ver, start, stop)

KMOD_EXPLICIT_DECL2(BUNDLEID, KEXTBUILD_S, _start, _stop)

/* If you intended to write a kext library  NULLify _realmain and _antimain */
__private_extern__ kmod_start_func_t *_realmain = kern_base_start;
__private_extern__ kmod_stop_func_t *_antimain = kern_base_stop;

__private_extern__ int _kext_apple_cc = __APPLE_CC__;
#endif

