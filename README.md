Simplest usage, which just opens a file named "eeprom" in the current directory:
```sh
display-tap-eeprom
```

Use a specific EEPROM file:
```sh
display-tap-eeprom [path-to-EEPROM-file-name]
```

Replace initials with a longer name (EEPROM file name is required).
You can have as many replacement specifiers as you want:
```sh
display-tap-eeprom [path-to-EEPROM-file-name] AJK:Kitaru AMN:Amnesia
```

The display format of records is meant to match the tetrisconcept.net TAP records list format.
