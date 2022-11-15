
# sudo apt-get install g++ binutils libc6-dev-i386
# sudo apt-get install VirtualBox grub-legacy xorriso

GCCPARAMS = -m32 -Iinclude -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -Wno-write-strings -fno-stack-protector
ASPARAMS = --32
LDPARAMS = -melf_i386

objects = loader.o gdt.o port.o interrupts.o interruptstubs.o kernel.o



#obj/%.o: src/%.cpp
#	mkdir -p $(@D)
#	gcc $(GCCPARAMS) -o $@ -c $<

#obj/%.o: src/%.s
#	mkdir -p $(@D)
#	as $(ASPARAMS) -o $@ $<

%.o: %.cpp
#	mkdir -p $(@D)
	gcc $(GCCPARAMS) -o $@ -c $<

%.o: %.s
	as $(ASPARAMS) -o $@ $<


.PHONY: mykernel.bin
mykernel.bin: linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)


mykernel.iso: mykernel.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp mykernel.bin iso/boot/mykernel.bin
	echo 'set timeout=0'                      > iso/boot/grub/grub.cfg
	echo 'set default=0'                     >> iso/boot/grub/grub.cfg
	echo ''                                  >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/mykernel.bin'    >> iso/boot/grub/grub.cfg
	echo '  boot'                            >> iso/boot/grub/grub.cfg
	echo '}'                                 >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=mykernel.iso iso
	rm -rf iso

run: mykernel.iso
#	echo 'SUCCESS'
	(killall VirtualBox and sleep 1) || true
	VirtualBox --startvm "Myos" &

install: mykernel.bin
	sudo cp $< /boot/mykernel.bin

.PHONY: clean
clean:
	rm -f $(objects) mykernel.bin mykernel.iso
