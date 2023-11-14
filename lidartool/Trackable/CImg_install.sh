
#!/bin/bash
git clone --depth=1 https://github.com/GreycLab/CImg.git
cd CImg
git reset --hard 1ba7710
git apply ../cimg_patch.diff

