/*
 * Created 190427 lynnl
 */

#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <mach-o/loader.h>

#include <kern/clock.h>
#include <string.h>

#include <sys/sysctl.h>

#ifndef __kext_makefile__
#define KEXTNAME_S          "kern_base"
#endif

#define LOG(fmt, ...)        printf(KEXTNAME_S ": " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)    LOG("[ERR] " fmt, ##__VA_ARGS__)

#define KERN_ADDR_MASK      0xfffffffffff00000u

#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(*a))
#define ARRAY_LAST(a)       (ARRAY_SIZE(a) - 1u)

static uint64_t _hib;
static uint64_t kern;
static uint32_t step;

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
    "" /* sysctl nub: kern.addr.kern */
)

static SYSCTL_UINT(
        _kern_addr,
        OID_AUTO,
        step,
        CTLFLAG_RD,
        &step,
        0,
        ""
);

/*
 * All node entries should place before nub entries
 */
static struct sysctl_oid *sysctl_entries[] = {
    /* sysctl nodes */
    &sysctl__kern_addr,

    /* sysctl nubs */
    &sysctl__kern_addr__hib,
    &sysctl__kern_addr_kern,
    &sysctl__kern_addr_step,
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

#define KERN_ADDR_STEP      (~KERN_ADDR_MASK + 1u)
#ifndef MH_FILESET
#define MH_FILESET          0xc
#endif

kern_return_t kern_base_start(kmod_info_t *ki __unused, void *d __unused)
{
    /* hib should less than base(specifically less KERN_ADDR_STEP) */
    _hib = ((uint64_t) memcpy) & KERN_ADDR_MASK;

    kern = ((uintptr_t) clock_get_calendar_microtime) & KERN_ADDR_MASK;
    while (kern > _hib + KERN_ADDR_STEP) {
        step++;
        kern -= KERN_ADDR_STEP;
    }

    if (kern != _hib + KERN_ADDR_STEP) {
        /* Kernel layout changes, this kext won't work */
        LOG_ERR("bad text base  step: %u __HIB: %#llx kernel: %#llx", step, _hib, kern);
        return KERN_FAILURE;
    }

    struct mach_header_64 *mh = (struct mach_header_64 *) kern;
    /* Only support non-fat 64-bit mach-o kernel */
    if ((mh->magic != MH_MAGIC_64 && mh->magic != MH_CIGAM_64) ||
        (mh->filetype != MH_EXECUTE && mh->filetype != MH_FILESET))
    {
        LOG_ERR("bad Mach-O header  step: %d magic: %#x filetype: %#x", step, mh->magic, mh->filetype);
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

