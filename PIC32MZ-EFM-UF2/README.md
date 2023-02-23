# [Microsoft UF2 Booloader ( USB MSD )](https://github.com/microsoft/uf2)

![pic32mz](https://raw.githubusercontent.com/Wiz-IO/examples-XC32/main/PIC32MZ-EFM-UF2/UF2.jpg)

## Bootloader ( v1.0 )
```ini
custom_linker = bootloader.ld 
```
Compile and Upload bootloader ( ICSP )<br>
Hold button ( Curioasity S1 ) to enter in boot mode<br>
Upload application.uf2 ....

## Application
```ini
upload_protocol = UF2 ; or copy / paste to usb drive
custom_linker   = application.ld
```

_( note: the configuration bits are set by the bootloader )_
