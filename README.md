## kern_base - Fetch Darwin kernel `__HIB`/text base

`kern_base` is a macOS kernel extension used to fetch Darwin `__HIB` text base and kernel text base

### Rationale

This project mainly inspired by [aidensstuff/kbase](https://github.com/aidensstuff/kbase), yet it currently undocumented. it uses a relative intuitive way to resolve kernel text bases(KALSR):

1. Masking `0xfffffffffff00000` with a kernel symbol address and threat it as a `struct mach_header_64 *`.

2. Check its first 4 bytes if equals to `MH_MAGIC_64`, if it equals, we're done.

3. Otherwise, we subtract the kernel symbol address by `0x100000` and goto step 1.

### Test

```shell
make [release]

sudo rm -rf /tmp/kern_base.kext
sudo cp -r kern_base.kext /tmp
sudo kextload -v 6 /tmp/kern_base.kext
```

After kext loaded, you can use [sysctl(8)](x-man-page://8/sysctl) to check the result:

```shell
sysctl kern.addr
kern.addr._hib: 0xffffff8011900000
kern.addr.kern: 0xffffff8011a00000

# Above is a sample output
```

To unload the kext after you tested:

```shell
sudo kextunload -v 6 /tmp/kern_base.kext
```

### Limitation

* Currently this kext only suitable for non-fat 64-bit Mach-O kernels, it won't be a big issue since modern Darwin kernels is flat 64-bit Mach-O format.

* Only tested on a limited set of kernels, assumably support macOS from 10.8 to 10.14(both inclusive):

	10.14.4(18E226)

	10.13.6(10.13.6)

	--10.12.6--

	10.11.6(15G31)

	10.10.5(14F27)

	10.9.5(13F1911)

* Iterative checking first 4 bytes isn't safe since an address mayn't got paged, thusly triggers a page fault and kernel panicked!

### *References*

[aidensstuff/kbase](https://github.com/aidensstuff/kbase)
