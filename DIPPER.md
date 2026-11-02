# ios-dipper — iOS 16 on Xiaomi Mi 8 (dipper, SDM845)

Port target: Xiaomi Mi 8 (`dipper`), Snapdragon 845 (SDM845), 6 GB RAM, UFS 2.1.
Base: `dipa-studio/ios-sweet` (xnu-8796 + iPhone X iOS 16.7.16 userland).
References: `NWylynko/ios-flame` (SM8150 — closest flagship SoC, ST FTS touch, Metal via Turnip),
`MakrSas/ios-topaz` (Xiaomi ABL quirks, HackiOS GPU/Wi-Fi work).

## Hardware facts (from LineageOS android_kernel_xiaomi_sdm845 dts — authoritative sources in dts/)

| Item | sweet (SM7150) | dipper (SDM845) | Status |
|---|---|---|---|
| GICD / GICR | 0x17a00000 / 0x17a60000 (stride 0x20000) | **same** | DONE in mkadt.py + shim |
| UFS host | ufshc@1d84000 (0x3000) | ufshc@1d84000 (0x2500), IRQ SPI 265, ufsphy qmp-v3 | same base; size updated |
| PSHOLD | 0x0c264000 | **same** (restart@10ac000 → pshold-base 0xC264000) | DONE |
| IMEM restart_reason | 0x146aa65c | **0x146bf65c** (msm-imem@146bf000+65c) | DONE in shim.c |
| vtimer INTID | 19 (GIC_PPI 3) | **same** (armv8-timer <1 3> = PPI3 → INTID 19) | no change |
| Timer freq | 19.2 MHz | **same** | no change |
| CPUs | MPIDR 0x0..0x700 (Aff1=core) 2xA76+6xA55 | **same layout** 4xKryo385-Silver(A55)+4xGold(A75); both clusters support TGran16 (A75/A55) → all 8 cores viable | freqs updated in mkadt.py |
| Framebuffer | cont_splash 0x9c000000 | **0x9d400000** size 0x2400000 | DONE in console.h, mmu.h, mkadt.py |
| Panel | 1080x2400 video | 1080x2248 **Samsung EA8074 AMOLED, CMD mode**, notch; pwr pm8998_l14 + disp_vci_vreg(GPIO23); reset tlmm6, te tlmm10 | FB geometry updated |
| Touch | Goodix GT9896 @ i2c 0x5d, QUP SE7 0xa84000 | **ST FTS @ i2c 0x49, QUPv3 SE14 i2c@a98000** (qupv3_1 @0xac0000, GIC_SPI 359); irq tlmm31, reset tlmm32; vdd=disp_vddio_vreg(GPIO23), avdd=pm8998_l28. Some units: Atmel mxt640u variant | needs driver: adapt flame 0085-fts (SPI→I2C) or minimal FTS I2C |
| Keys | PM6150 PON + PM6150L GPIO2 (SPMI SID4) | PM8998 PON: kpdpwr(116), pon_2/resin(114); vol-up = pm8998_gpio6 | needs pm8998 SPMI addresses |
| Boot image | boot.img v2, --base 0 | dipper stock = TBD (verify header ver in stock boot.img; Android 8.1-era → likely v0/v1, MIUI12 A10 → v2) | TBD: pull stock boot.img |
| RAM map | highmem 0x1_00000000 + 0xfcc00000 | assumed 0x1_00000000 + up to 6G total; **verify live FDT** | TBD |
| PDC | — | pdc@b220000 is interrupt-parent for most IRQs (wakeup) | note for touch/key IRQs |
| SPMI | PM6150/PM6150L | spmi PMIC arbiter addr TBD (pm8998 sid0, pmi8998 sid1?) | TBD |

## File changes done so far

- `tools/mkadt.py`: FB 0x9d400000/2248px, UFS size 0x2500, CPU freqs 1.766/2.803 GHz,
  compatible +xiaomi,dipper +qcom,sdm845, HIGH_MEM 0x1_00000000+0x180000000 (TBD verify),
  ECID synthetic.
- `shim/console.h`, `shim/mmu.h`: FB 0x9d400000, 2248.
- `shim/shim.c`: IMEM_RESTART_REASON 0x146bf65c. SMP probe uses i<<8 MPIDRs — matches SDM845.

## What still needs doing (ordered)

1. **Pull device data from the phone** (adb + recovery): live FDT (/sys/firmware/fdt),
   stock boot.img + dtbo.img header version, partition backups (persist, modemst1/2, fsg, fsc,
   boot, dtbo, vbmeta). Gives real RAM banks, real DTB for shim --dtb, confirms FTS vs Atmel.
2. **CI kernel build**: fork repo → xnu.yml workflow (macos-14, SWEET machine config).
   Needs user's GitHub account. Kernel = `kernel.development.sweet`.
3. **Shim build**: aarch64-linux-gnu-gcc + mkbootimg — run on GitHub Actions ubuntu
   (or local WSL2 if user installs as admin). Adjust build.sh once boot.img header known:
   dipper likely needs `--header_version 0|1|2` matching stock; dtb lives in dtbo.img
   (v0 has no dtb field — may need to pass stock dtbo via --dtb on v2, or patch loader
   to read FDT passed by ABL — shim already reads the live FDT from ABL anyway).
4. **mkadt for dipper**: confirm RAM banks from live FDT, FB_SIZE, then
   `mkadt.py dipper.adt --highmem`.
5. **iOS root image**: OS=087-86698-044.dmg SystemOS=087-86593-047.dmg AppOS=087-86715-047.dmg
   (iPhone X 16.7.16 20H392, IPSW already on disk). mkiosroot.sh + sweet-root-tweaks.py
   need Linux + apfs-fuse + hfsprogs → run on GH Actions ubuntu runner or WSL2.
   Extract DMGs from local IPSW instead of remotezip (offline-friendly).
6. **Boot**: `fastboot flash userdata ios16-root.simg` then `fastboot boot shim-boot.img`.
   First boot ~4-5 min on Apple logo. Expect black screen iterations → kernel console on
   framebuffer (sweetfb driver) is the debug channel; log ring at end of userdata.
7. **Bring-up order**: boot → display console → UFS root → launchd → backboardd/SpringBoard →
   then drivers: FTS touch (SE14 I2C), PM8998 keys (SPMI), battery, (later: MDP direct scanout,
   GPU A630 via freedreno — flame's Turnip path, WCN3990 Wi-Fi, SDM845 modem).

## Known dips for dipper (predicted from other ports)

- Command-mode AMOLED panel (EA8074): framebuffer copy works via cont_splash; vsync/TE
  (tlmm10) differs from video-mode panels — flame (also cmd-mode P-OLED) solved this;
  see flame patches 0084/0087/0121.
- SDM845 has real LSE atomics (ARMv8.2) — no LSE emulation needed (unlike topaz).
- A/B: dipper is A-only, single userdata — simpler than A/B devices.
- Xiaomi ABL on 2018 flagships accepts `fastboot boot` (used by postmarketOS/edk2-sdm845).
- USB: sdm845 dwc3 — flame 0088/0089 USB-ethernet patches likely adaptable → SSH log channel.
