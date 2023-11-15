# Record Packed File Format

Set variable recordPackedDir to an existing directory

Recording is done only in production mode

Build the packedPlayer tool
```console
> cd ../lidartool
> make packedPlayer
```

Output in .pfk files can then be processed by packedPlayer 
```console
../lidartool/packedPlayer +conf confName +i inFile.pkf
```

See `packedPlayer -h` output for more info
