# Record Packed file format

set variable recordPackedDir to an existing directory

recording is done only in production mode

output can be processed by packedPlayer 
```console
../lidartool/packedPlayer +conf confName +i inFile.pkf
```

You have to build it beforehand
```console
> cd ../lidartool
> make packedPlayer
```

see `packedPlayer -h` for more info

