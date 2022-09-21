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

#include <Guid/FileInfo.h>

#undef OPROM_DEBUG
// https://habr.com/en/company/acronis/blog/535848/
// https://github.com/LongSoft/CrScreenshotDxe/blob/master/CrScreenshotDxe.c

// https://github.com/pbatard/ntfs-3g/releases

// Simple UEFI PCI driver:
// https://github.com/tianocore/edk2/blob/master/OvmfPkg/VirtioPciDeviceDxe/VirtioPciDevice.c

#define BOOTKIT_DRIVER_VERSION 0x01
#define EFI_CONSOLE_CONTROL_PROTOCOL_GUID                                      \
  {                                                                            \
    0x77800382, 0x12e, 0x4c82, {                                               \
      0x55, 0x56, 0x81, 0xf9, 0x43, 0x4, 0xf7, 0xc7                            \
    }                                                                          \
  }

void SetVar();
void DumpCalcExe();

EFI_STATUS
EFIAPI
DriverStart(IN EFI_DRIVER_BINDING_PROTOCOL *This, IN EFI_HANDLE Controller,
            IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath) {
  DumpCalcExe();
  SetVar();
  return EFI_SUCCESS;
}

BOOLEAN Checke1000eNIC(EFI_HANDLE Controller,
                       EFI_DRIVER_BINDING_PROTOCOL **This) {
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_PCI_IO_PROTOCOL *PciIo;

  // Open the PCIIo protocol on this PCI device handle
  PCI_TYPE00 Pci;
  Status = gBS->OpenProtocol(Controller, &gEfiPciIoProtocolGuid,
                             (VOID **)&PciIo, (*This)->DriverBindingHandle,
                             Controller, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR(Status) || PciIo == NULL) {
    return FALSE;
  }
  Status = PciIo->Pci.Read(PciIo,                       // (protocol, device)
                                                        // handle
                           EfiPciIoWidthUint32,         // access width & copy
                                                        // mode
                           0,                           // Offset
                           sizeof Pci / sizeof(UINT32), // Count
                           &Pci                         // target buffer
  );

  gBS->CloseProtocol(Controller, &gEfiPciIoProtocolGuid,
                     (*This)->DriverBindingHandle, Controller);

  if (Status == EFI_SUCCESS) {
#ifdef OPROM_DEBUG
    Print(L"PCI %X %X\n", Pci.Hdr.VendorId, Pci.Hdr.DeviceId);
#endif
    if (Pci.Hdr.VendorId == 0x8086 && Pci.Hdr.DeviceId == 0x10d3) {
      return TRUE;
    } else {
      return FALSE;
    }
  }
  return FALSE;
}

EFI_STATUS
EFIAPI
DriverSupported(IN EFI_DRIVER_BINDING_PROTOCOL *This, IN EFI_HANDLE Controller,
                IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath) {
  EFI_DEVICE_PATH_PROTOCOL *this = DevicePathFromHandle(Controller);
  if (this == NULL) {
    return EFI_UNSUPPORTED;
  }

#ifdef OPROM_DEBUG
  CHAR16 *p = ConvertDevicePathToText(this, TRUE, FALSE);
  Print(L"%s\n", p);
#endif

  if (Checke1000eNIC(Controller, &This)) {
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}

EFI_STATUS
EFIAPI
DriverStop(IN EFI_DRIVER_BINDING_PROTOCOL *This, IN EFI_HANDLE Controller,
           IN UINTN NumberOfChildren, IN EFI_HANDLE *ChildHandleBuffer) {
  return EFI_SUCCESS;
}

void SetVar() {
  EFI_GUID myvar = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
  unsigned char data[] = "hello";
  EFI_RUNTIME_SERVICES *rBS = gST->RuntimeServices;
  rBS->SetVariable(L"ExVar", &myvar,
                   EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                       EFI_VARIABLE_RUNTIME_ACCESS,
                   6, data);
}

EFI_DRIVER_BINDING_PROTOCOL gTestDriverBinding = {
    DriverSupported,        DriverStart, DriverStop,
    BOOTKIT_DRIVER_VERSION, NULL,        NULL};

EFI_STATUS
EFIAPI
OptionRomEntrypoint(IN EFI_HANDLE ImageHandle,
                    IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status; //
  // Entry Point
  // Install driver model protocol(s) onto ImageHandle
  // Can't do much else, not allowed to interact with HW or do print statements,
  // etc...

  gBS = SystemTable->BootServices;
  gST = SystemTable;
  gImageHandle = ImageHandle;

  Status = EfiLibInstallDriverBindingComponentName2(
      ImageHandle,         // ImageHandle
      SystemTable,         // SystemTable
      &gTestDriverBinding, // DriverBinding
      ImageHandle,         // DriverBindingHandle
      NULL, NULL);

  ASSERT_EFI_ERROR(Status);
  return Status;
}