# datagen.py
#
# use:
#   python3 datagen.py > in_data_[x].csv
#

import random
for i in range(9999):
    print(random.randint(0,1000), end="")
    for j in range(9999):
        print(",", end="")
        print(random.randint(0,1000),end="")
    print("")
