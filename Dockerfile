FROM ubuntu:jammy

RUN apt update && apt upgrade -y && apt install git build-essential git uuid-dev iasl nasm python3 python3-distutils python3-apt python2 -y

RUN ln -s /bin/python3 /bin/python

RUN git clone https://github.com/tianocore/edk2

WORKDIR /edk2

RUN git checkout 2c17d676e402d75a3a674499342f7ddaccf387bd && git submodule update --init

COPY ntfs_x64_rw.efi /

COPY OptionRomBootkit OptionRomBootkit/

RUN /bin/bash -c "source edksetup.sh && make -C BaseTools && sed -i -z 's/\[Components\]/\[Components\]\n  OptionRomBootkit\/OptionRomBootkit.inf/g' /edk2/OvmfPkg/OvmfPkgX64.dsc \
	&& build -a X64 -t GCC5 -b RELEASE -p OvmfPkg/OvmfPkgX64.dsc"

RUN BaseTools/Source/C/bin/EfiRom -f 0x8086 -i 0x10d3 -v --debug 9 -o OptionRomBootkit.efirom -e /ntfs_x64_rw.efi Build/OvmfX64/RELEASE_GCC5/X64/OptionRomBootkit.efi
