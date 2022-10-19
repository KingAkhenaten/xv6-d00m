K=kernel
U=user

OBJS = \
  $K/entry.o \
  $K/start.o \
  $K/console.o \
  $K/printf.o \
  $K/uart.o \
  $K/kalloc.o \
  $K/spinlock.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trampoline.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/sysgpu.o \
  $K/syskbd.o \
  $K/bio.o \
  $K/fs.o \
  $K/log.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/pipe.o \
  $K/exec.o \
  $K/sysfile.o \
  $K/kernelvec.o \
  $K/plic.o \
  $K/virtio_disk.o \
  $K/virtiogpu.o \
  $K/virtiokbd.o

# riscv64-unknown-elf- or riscv64-linux-gnu-
# perhaps in /opt/riscv/bin
#TOOLPREFIX = 

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

QEMU = qemu-system-riscv64

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

LDFLAGS = -z max-page-size=4096

$K/kernel: $(OBJS) $K/kernel.ld $U/initcode
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) 
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.o > $U/initcode.asm

tags: $(OBJS) _init
	etags *.S *.c

ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

$U/_forktest: $U/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

# Doom needs it's own special building, because it is Doom, not a single-file program
# Getting the makefile to just work *normally* is hell on earth
# Doom engine internal
DOOM_OBJS = \
	$U/doom/xv6.o \
	$U/doom/am_map.o \
	$U/doom/d_event.o \
	$U/doom/d_items.o \
	$U/doom/d_iwad.o \
	$U/doom/d_loop.o \
	$U/doom/d_main.o \
	$U/doom/d_mode.o \
	$U/doom/d_net.o \
	$U/doom/doomdef.o \
	$U/doom/doomgeneric.o \
	$U/doom/doomgeneric_sdl.o \
	$U/doom/doomgeneric_soso.o \
	$U/doom/doomgeneric_sosox.o \
	$U/doom/doomgeneric_win.o \
	$U/doom/doomgeneric_xlib.o \
	$U/doom/doomstat.o \
	$U/doom/dstrings.o \
	$U/doom/dummy.o \
	$U/doom/f_finale.o \
	$U/doom/f_wipe.o \
	$U/doom/g_game.o \
	$U/doom/gusconf.o \
	$U/doom/hu_lib.o \
	$U/doom/hu_stuff.o \
	$U/doom/i_cdmus.o \
	$U/doom/icon.o \
	$U/doom/i_endoom.o \
	$U/doom/i_input.o \
	$U/doom/i_joystick.o \
	$U/doom/i_main.o \
	$U/doom/info.o \
	$U/doom/i_scale.o \
	$U/doom/i_sound.o \
	$U/doom/i_system.o \
	$U/doom/i_timer.o \
	$U/doom/i_video.o \
	$U/doom/m_argv.o \
	$U/doom/m_bbox.o \
	$U/doom/m_cheat.o \
	$U/doom/m_config.o \
	$U/doom/m_controls.o \
	$U/doom/memio.o \
	$U/doom/m_fixed.o \
	$U/doom/m_menu.o \
	$U/doom/m_misc.o \
	$U/doom/m_random.o \
	$U/doom/p_ceilng.o \
	$U/doom/p_doors.o \
	$U/doom/p_enemy.o \
	$U/doom/p_floor.o \
	$U/doom/p_inter.o \
	$U/doom/p_lights.o \
	$U/doom/p_map.o \
	$U/doom/p_maputl.o \
	$U/doom/p_mobj.o \
	$U/doom/p_plats.o \
	$U/doom/p_pspr.o \
	$U/doom/p_saveg.o \
	$U/doom/p_setup.o \
	$U/doom/p_sight.o \
	$U/doom/p_spec.o \
	$U/doom/p_switch.o \
	$U/doom/p_telept.o \
	$U/doom/p_tick.o \
	$U/doom/p_user.o \
	$U/doom/r_bsp.o \
	$U/doom/r_data.o \
	$U/doom/r_draw.o \
	$U/doom/r_main.o \
	$U/doom/r_plane.o \
	$U/doom/r_segs.o \
	$U/doom/r_sky.o \
	$U/doom/r_things.o \
	$U/doom/sha1.o \
	$U/doom/sounds.o \
	$U/doom/s_sound.o \
	$U/doom/statdump.o \
	$U/doom/st_lib.o \
	$U/doom/st_stuff.o \
	$U/doom/tables.o \
	$U/doom/v_video.o \
	$U/doom/w_checksum.o \
	$U/doom/w_file.o \
	$U/doom/w_file_stdc.o \
	$U/doom/wi_stuff.o \
	$U/doom/w_main.o \
	$U/doom/w_wad.o \
	$U/doom/z_zone.o \

$U/doom/%.o: $U/doom/%.c
	$(CC) $(CFLAGS) -c $< -o $@

#Doom xv6 launcher
$U/_doom: $U/doom.o $(ULIB) $(DOOM_OBJS)
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $^
	$(OBJDUMP) -S $@ > $U/doom.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $U/doom.sym

mkfs/mkfs: mkfs/mkfs.c $K/fs.h $K/param.h
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\
	$U/_motd\
	$U/_ed\
	$U/_gputest\
	$U/_kbdtest\
	$U/_doom

fs.img: mkfs/mkfs README $(UPROGS) $U/default.cfg $U/DOOM1.WAD
	mkfs/mkfs fs.img README $(UPROGS) $U/default.cfg $U/DOOM1.WAD

-include kernel/*.d user/*.d

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym \
	$U/doom/*.o $U/doom/*.d \
	$U/initcode $U/initcode.out $K/kernel fs.img \
	mkfs/mkfs .gdbinit \
        $U/usys.S \
	$(UPROGS)

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS
CPUS := 3
endif

QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS)
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
DISPLAYOPTS = -device virtio-gpu-device,bus=virtio-mmio-bus.1,xres=320,yres=200
KEYBOARDOPTS = -device virtio-keyboard-device,bus=virtio-mmio-bus.2
SPICEOPTS = -spice port=32666,disable-ticketing=on

qemu: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS) $(KEYBOARDOPTS) $(DISPLAYOPTS) -nographic

.gdbinit: .gdbinit.tmpl-riscv
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) $(KEYBOARDOPTS) $(DISPLAYOPTS) -S $(QEMUGDB) 

qemu-vga: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS) $(KEYBOARDOPTS) $(DISPLAYOPTS) -serial stdio

qemu-vga-nographic: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS) $(KEYBOARDOPTS) $(DISPLAYOPTS) -nographic

qemu-vga-windows: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS) $(KEYBOARDOPTS) $(DISPLAYOPTS) $(SPICEOPTS) -nographic
